#ifndef PBRLAB_SCENE_H_
#define PBRLAB_SCENE_H_
#include <memory>

#include "light-manager.h"
#include "material-param.h"
#include "mesh-instance.h"
#include "ray.h"
#include "raytracer/raytracer.h"

namespace pbrlab {

class Scene {
public:
  Scene(void);
  ~Scene(void);

  template <class... Args>
  MeshPtr AddTriangleMesh(Args&&... args) {
    triangle_meshes_.emplace_back(std::make_shared<TriangleMesh>(args...));
    MeshPtr ret = triangle_meshes_.back();
    return ret;
  }

  template <class... Args>
  MeshPtr AddCubicBezierCurveMesh(Args&&... args) {
    cubic_bezier_curve_meshes_.emplace_back(
        std::make_shared<CubicBezierCurveMesh>(args...));
    MeshPtr ret = cubic_bezier_curve_meshes_.back();
    return ret;
  }

  template <class... Args>
  uint32_t AddLightParam(Args&&... args) {
    return light_manager_->AddLightParam(args...);
  }

  template <class... Args>
  uint32_t AddMaterialParam(Args&&... args) {
    const uint32_t id = static_cast<uint32_t>(material_params_.size());
    material_params_.emplace_back(args...);
    return id;
  }

  void AttachLightParamIdsToInstance(
      const uint32_t instance_id,
      const std::vector<std::vector<uint32_t>>& light_param_ids);

  void CommitScene(void);

  uint32_t CreateInstance(const MeshPtr& mesh_ptr);

  const LightManager* GetLightManager(void) const;

  inline LightParameter* FetchMeshLightParamPtr(
      const TraceResult& trace_result) const;

  const MaterialParameter* FetchMeshMaterialParameter(
      const TraceResult& trace_result) const;

  std::vector<MaterialParameter>* FetchMeshMaterialParameters(void);

  float3 FetchMeshShadingNormal(const TraceResult& trace_result) const;

  void FetchSceneAABB(float* bmax, float* bmin) const;

  TraceResult TraceFirstHit1(const Ray& ray) const;

  bool AnyHit1(const Ray& ray) const;

private:
  std::vector<MeshInstance> instances_;

  std::vector<std::shared_ptr<TriangleMesh>> triangle_meshes_;

  std::vector<std::shared_ptr<CubicBezierCurveMesh>> cubic_bezier_curve_meshes_;

  std::vector<MaterialParameter> material_params_;

  raytracer::Raytracer raytracer_;

  std::shared_ptr<LightManager> light_manager_;

  float bmax_[3], bmin_[3];
};
}  // namespace pbrlab
#endif  // PBRLAB_SCENE_H_
