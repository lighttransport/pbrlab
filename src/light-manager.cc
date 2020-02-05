#include "light-manager.h"

#include <numeric>

#include "pbrlab-util.h"
#include "sampler/sampling-utils.h"

namespace pbrlab {

AreaLightParameter LightManager::FetchAsAreaLightParam(
    const uint32_t id) const {
#ifdef NDEBUG
  return mpark::get<kAreaLight>(light_params_[id]);
#else
  return mpark::get<kAreaLight>(light_params_.at(id));
#endif
}

LightManager::LightManager() = default;

void LightManager::Clear() {
  lights_.clear();
  cumulative_probability_.clear();
  area_lights_.clear();
  light_params_.clear();
}

void LightManager::Commit() {
  lights_.clear();
  cumulative_probability_.clear();
  double intensity_sum = 0.0;

  // Area Light
  for (size_t instance_id = 0; instance_id < area_lights_.size();
       ++instance_id) {
    auto& area_lights = area_lights_[instance_id];

    for (size_t local_id = 0; local_id < area_lights.size(); ++local_id) {
      auto& area_light = area_lights[local_id];
      if (area_light) {
        area_light->global_id = uint32_t(lights_.size());

        Light light = {};

        light.light_type               = kAreaLight;
        light.choose_light_probability = area_light->intensity_sum;

        light.local_id.emplace_back(instance_id);
        light.local_id.emplace_back(local_id);

        intensity_sum += double(light.choose_light_probability);

        lights_.emplace_back(light);
      }
    }
  }

  // Normalize Intensity
  std::transform(lights_.begin(), lights_.end(), lights_.begin(),
                 [intensity_sum](const Light& light) {
                   Light tmp                    = light;
                   tmp.choose_light_probability = float(
                       double(tmp.choose_light_probability) / intensity_sum);
                   return tmp;
                 });

  cumulative_probability_.resize(lights_.size());
  std::transform(
      lights_.begin(), lights_.end(), cumulative_probability_.begin(),
      [](const Light& light) { return light.choose_light_probability; });
  for (uint32_t light_id = 0; !cumulative_probability_.empty() &&
                              light_id < cumulative_probability_.size() - 1u;
       ++light_id) {
    cumulative_probability_[light_id + 1u] += cumulative_probability_[light_id];
  }
}

void LightManager::RegisterInstanceMesh(const MeshInstance& instance,
                                        const uint32_t instance_id) {
  if (area_lights_.size() <= instance_id) area_lights_.resize(instance_id + 1);

  auto& area_lights = area_lights_[instance_id];

  if (area_lights.size() <= instance.meshes.size())
    area_lights.resize(instance.meshes.size());

  for (uint32_t local_geom_id = 0; local_geom_id < instance.meshes.size();
       ++local_geom_id) {
    const MeshPtr mesh_ptr = instance.meshes[local_geom_id];
    const std::vector<uint32_t>& light_param_ids =
        instance.light_param_ids[local_geom_id];

    if (mesh_ptr.index() == kTriangleMesh) {
      const std::shared_ptr<TriangleMesh>& triangle_mesh =
          mpark::get<kTriangleMesh>(mesh_ptr);
      const uint32_t num_face = triangle_mesh->GetNumFaces();

      bool have_emission = false;
      for (uint32_t face_id = 0; face_id < num_face; ++face_id) {
        if (light_param_ids[face_id] != static_cast<uint32_t>(-1)) {
          have_emission = true;
          break;
        }
      }
      if (!have_emission) continue;

      auto& area_light = area_lights[local_geom_id];
      area_light.reset(new AreaLight);
      area_light->mesh_ptr        = mesh_ptr;
      area_light->light_param_ids = light_param_ids;

      area_light->choose_primitive_probability.resize(num_face);
      for (uint32_t face_id = 0; face_id < num_face; ++face_id) {
        float intensity = 0.0f;
        if (light_param_ids[face_id] != static_cast<uint32_t>(-1)) {
          const AreaLightParameter& light_param =
              FetchAsAreaLightParam(light_param_ids[face_id]);

          intensity = SpectrumNorm(light_param.emission);
        }

        // TODO transform
        area_light->choose_primitive_probability[face_id] =
            intensity * triangle_mesh->FetchFaceArea(face_id);
      }

      // calculate intensity sum
      area_light->intensity_sum =
          std::accumulate(area_light->choose_primitive_probability.begin(),
                          area_light->choose_primitive_probability.end(), 0.0f);

      const float& intensity_sum = area_light->intensity_sum;

      // Normalize Intensity
      std::transform(
          area_light->choose_primitive_probability.begin(),
          area_light->choose_primitive_probability.end(),
          area_light->choose_primitive_probability.begin(),
          [&intensity_sum](const float v) { return v / intensity_sum; });

      // Calc Cumulative Probability
      area_light->cumulative_probability_.resize(
          area_light->choose_primitive_probability.size());
      std::copy(area_light->choose_primitive_probability.begin(),
                area_light->choose_primitive_probability.end(),
                area_light->cumulative_probability_.begin());

      for (uint32_t face_id = 0;
           !area_light->cumulative_probability_.empty() &&
           face_id < area_light->cumulative_probability_.size() - 1u;
           ++face_id) {
        area_light->cumulative_probability_[face_id + 1u] +=
            area_light->cumulative_probability_[face_id];
      }

      area_light->intensity_sum = intensity_sum;

      // Calc primitive area measure pdf
      area_light->prim_area_measure_pdf.resize(num_face);

      for (uint32_t face_id = 0; face_id < num_face; ++face_id) {
        if (light_param_ids[face_id] != static_cast<uint32_t>(-1)) {
          // TODO transform
          area_light->prim_area_measure_pdf[face_id] =
              1.0f / triangle_mesh->FetchFaceArea(face_id);
        }
      }

    } else {
      continue;
    }
  }
}

}  // namespace pbrlab
