#ifndef PBRLAB_LIGHT_MANAGER_H_
#define PBRLAB_LIGHT_MANAGER_H_

#include <memory>
#include <unordered_map>

#include "light-param.h"
#include "mesh-instance.h"
#include "mesh/triangle-mesh.h"
#include "random/rng.h"
#include "sampler/sampling-utils.h"
#include "type.h"

namespace pbrlab {

class LightManager {
public:
  LightManager();

  template <class... Args>
  uint32_t AddLightParam(Args&&... args) {
    const uint32_t ret = uint32_t(light_params_.size());
    light_params_.emplace_back(args...);

    return ret;
  }

  void Clear(void);
  void Commit(void);

  inline auto ImplicitAreaLight(const uint32_t instance_id,
                                const uint32_t local_geom_id,
                                const uint32_t prim_id) const {
#ifdef NDEBUG
    const std::unique_ptr<AreaLight>& area_light =
        area_lights_[instance_id][local_geom_id];
#else
    const std::unique_ptr<AreaLight>& area_light =
        area_lights_.at(instance_id).at(local_geom_id);
#endif

    struct {
      float3 emission;
      float pdf;
    } ret;

    if (!area_light ||
        area_light->light_param_ids[prim_id] == static_cast<uint32_t>(-1)) {
      ret.emission = float3(0.f);
      ret.pdf      = 0.0f;
      return ret;
    }

#ifdef NDEBUG
    const AreaLightParameter& area_light_param =
        FetchAsAreaLightParam(area_light->light_param_ids[prim_id]);
#else
    const AreaLightParameter& area_light_param =
        FetchAsAreaLightParam(area_light->light_param_ids.at(prim_id));
#endif

#ifdef NDEBUG
    ret.emission = area_light_param.emission;
    ret.pdf      = lights_[area_light->global_id].choose_light_probability *
              area_light->choose_primitive_probability[prim_id] *
              area_light->prim_area_measure_pdf[prim_id];
    return ret;
#else
    ret.emission = area_light_param.emission;
    ret.pdf      = lights_.at(area_light->global_id).choose_light_probability *
              area_light->choose_primitive_probability.at(prim_id) *
              area_light->prim_area_measure_pdf.at(prim_id);
    return ret;
#endif
  }

  void RegisterInstanceMesh(const MeshInstance& instance,
                            const uint32_t instance_id);

  auto SampleAllLight(const RNG& rng) const {
    struct {
      LightType light_type;
      float3 v1, v2, emission;
      float pdf;
    } ret;

    if (cumulative_probability_.empty()) {
      // there are no lights.
      ret.light_type = kLightNone;
      ret.v1         = float3(0.f);
      ret.v2         = float3(0.f);
      ret.emission   = float3(0.f);
      ret.pdf        = 0.f;
      return ret;
    }

    const float u0 = rng.Draw();
    const uint32_t choosed_light_id =
        uint32_t(std::lower_bound(cumulative_probability_.begin(),
                                  cumulative_probability_.end(), u0) -
                 cumulative_probability_.begin());

    assert(choosed_light_id < cumulative_probability_.size());

    const Light& light            = lights_[choosed_light_id];
    const float& choose_light_pdf = light.choose_light_probability;

    if (light.light_type == kAreaLight) {
      assert(light.local_id.size() == 2);
      const uint32_t instance_id = light.local_id[0];
      const uint32_t local_id    = light.local_id[1];
#ifdef NDEBUG
      const AreaLight* area_light = area_lights_[instance_id][local_id].get();
#else
      const AreaLight* area_light =
          area_lights_.at(instance_id).at(local_id).get();
#endif
      const float u1                 = rng.Draw();
      const uint32_t choosed_prim_id = uint32_t(
          std::lower_bound(area_light->cumulative_probability_.begin(),
                           area_light->cumulative_probability_.end(), u1) -
          area_light->cumulative_probability_.begin());

      assert(choosed_prim_id < area_light->cumulative_probability_.size());

      const float& choose_prim_pdf =
          area_light->choose_primitive_probability[choosed_prim_id];

      const float& prim_area_measure_pdf =
          area_light->prim_area_measure_pdf[choosed_prim_id];

      const LightParameter& light_param =
          light_params_[area_light->light_param_ids[choosed_prim_id]];

      const AreaLightParameter area_light_param =
          mpark::get<kAreaLight>(light_param);

      if (area_light->mesh_ptr.index() == kTriangleMesh) {
        const float u2 = rng.Draw(), u3 = rng.Draw();
        const auto uv = TriangleUniformSampler(u2, u3);
        const std::shared_ptr<TriangleMesh>& triangle_mesh =
            mpark::get<kTriangleMesh>(area_light->mesh_ptr);
        const float3 local_position = triangle_mesh->FetchLocalPosition(
            choosed_prim_id, uv.first, uv.second);
        // TODO transform
        const float3 global_position = local_position;

        const float3 local_normal =
            triangle_mesh->FetchGeometryNormal(choosed_prim_id);
        // TODO transform
        const float3 global_normal = local_normal;

        const float pdf =
            choose_light_pdf * choose_prim_pdf * prim_area_measure_pdf;

        ret.light_type = kAreaLight;
        ret.v1         = global_position;
        ret.v2         = global_normal;
        ret.emission   = area_light_param.emission;
        ret.pdf        = pdf;

        return ret;
      } else {
        assert(false);
      }
    }
    assert(false);
    // there are no lights.
    ret.light_type = kLightNone;
    ret.v1         = float3(0.f);
    ret.v2         = float3(0.f);
    ret.emission   = float3(0.f);
    ret.pdf        = 0.f;
    return ret;
  }

private:
  AreaLightParameter FetchAsAreaLightParam(const uint32_t id) const;

  struct AreaLight {
    MeshPtr mesh_ptr;
    std::vector<uint32_t> light_param_ids;            // num_prim
    std::vector<float> choose_primitive_probability;  // num_prim
    std::vector<float> cumulative_probability_;
    std::vector<float> prim_area_measure_pdf;  // num_prim
    float intensity_sum;
    uint32_t global_id = static_cast<uint32_t>(-1);
  };

  struct Light {
    LightType light_type;
    float choose_light_probability = 0.0f;
    std::vector<uint32_t> local_id;
  };
  std::vector<Light> lights_;
  std::vector<float> cumulative_probability_;
  std::vector<std::vector<std::unique_ptr<AreaLight>>> area_lights_;
  std::vector<LightParameter> light_params_;
};
}  // namespace pbrlab
#endif  // PBRLAB_LIGHT_MANAGER_H_
