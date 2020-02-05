#include "scene.h"

#include <exception>
#include <memory>
#include <variant>

#include "matrix.h"

namespace pbrlab {

Scene::Scene(void) : light_manager_(new LightManager()) {}
Scene::~Scene(void) = default;

void Scene::AttachLightParamIdsToInstance(
    const uint32_t instance_id,
    std::vector<std::vector<uint32_t>> light_param_ids) {
  MeshInstance& instance = instances_.at(instance_id);
  if (instance.light_param_ids.size() != light_param_ids.size())
    throw std::runtime_error("light param error");

  instance.light_param_ids = light_param_ids;
}

void Scene::CommitScene(void) {
  for (uint32_t i = 0; i < instances_.size(); ++i) {
    light_manager_->RegisterInstanceMesh(instances_[i], i);
  }

  light_manager_->Commit();
}

uint32_t Scene::CreateInstance(const MeshPtr& mesh_ptr) {
  MeshInstance instance;

  // Mesh
  const std::shared_ptr<TriangleMesh> triangle_mesh =
      mpark::get<kTriangleMesh>(mesh_ptr);
  instance.meshes.emplace_back(mesh_ptr);

  // Material
  // TODO specify material ids
  instance.material_ids.emplace_back(triangle_mesh.get()->GetMaterials());

  // Light
  { instance.light_param_ids.emplace_back(); }

  // BVH
  const std::vector<float>& vertices      = triangle_mesh.get()->GetVertices();
  const std::vector<uint32_t>& vertex_ids = triangle_mesh.get()->GetVertexIds();

  const uint32_t num_vertices = triangle_mesh.get()->GetNumVertices();
  const uint32_t num_faces    = triangle_mesh.get()->GetNumFaces();

  // TODO transform
  const float tranform[4][4] = {{1.0f, 0.0f, 0.0f, 0.0f},
                                {0.0f, 1.0f, 0.0f, 0.0f},
                                {0.0f, 0.0f, 1.0f, 0.0f},
                                {0.0f, 0.0f, 0.0f, 1.0f}};

  Matrix::Copy(tranform, instance.transform_gl);  // [NOTE] local to global
  Matrix::Copy(tranform, instance.transform_lg);
  Matrix::Inverse(instance.transform_lg);  // [NOTE] global to local

  uint32_t instance_id, local_scene_id, local_geom_id;
  raytracer_.RegisterNewTriangleMesh(
      vertices.data(), num_vertices, vertex_ids.data(), num_faces, tranform,
      &instance_id, &local_scene_id, &local_geom_id);

  // Register Instance
  // TODO : local_scene_id, local_geom_id
  assert(local_geom_id == 0);
  if (instance_id >= instances_.size()) {
    instances_.resize(instance_id + 1);
  }
  instances_[instance_id] = instance;
  return instance_id;
}

const std::shared_ptr<LightManager> Scene::GetLightManager(void) const {
  return light_manager_;
}

const MaterialParameter* Scene::FetchMeshMaterialParamPtr(
    const TraceResult& trace_result) const {
  assert(trace_result.instance_id < instances_.size());
  const MeshInstance& instance = instances_[trace_result.instance_id];

  assert(trace_result.geom_id < instance.material_ids.size());
  const std::vector<uint32_t> material_ids =
      instance.material_ids[trace_result.geom_id];

#ifdef NDEBUG
  return &(material_params_[material_ids[trace_result.prim_id]]);
#else
  return &(material_params_.at(material_ids.at(trace_result.prim_id)));
#endif
}

float3 Scene::FetchMeshShadingNormal(const TraceResult& trace_result) const {
  assert(trace_result.instance_id < instances_.size());
  const MeshInstance& instance = instances_[trace_result.instance_id];

  assert(trace_result.geom_id < instance.meshes.size());
  const MeshPtr& mesh_p = instance.meshes[trace_result.geom_id];

  float3 ret;
  if (mesh_p.index() == kTriangleMesh) {
    // TODO transform
    ret = mpark::get<kTriangleMesh>(mesh_p)->FetchShadingNormal(
        trace_result.prim_id, trace_result.u, trace_result.v);
  } else {
    assert(false);
  }
  return ret;
}

void Scene::FetchSceneAABB(float* bmin, float* bmax) const {
  raytracer_.GetSceneAABB(bmin, bmax);
}

TraceResult Scene::TraceFirstHit1(const Ray& ray) const {
  return raytracer_.FirstHitTrace1(ray.ray_org.v, ray.ray_dir.v, ray.min_t,
                                   ray.max_t);
}

bool Scene::AnyHit1(const Ray& ray) const {
  return raytracer_.AnyHit1(ray.ray_org.v, ray.ray_dir.v, ray.min_t, ray.max_t);
}

}  // namespace pbrlab
