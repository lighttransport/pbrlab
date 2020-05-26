#include "scene.h"

#include <exception>
#include <memory>
#include <variant>

#include "matrix.h"

namespace pbrlab {

Scene::Scene(void) : light_manager_(new LightManager()) {}
Scene::~Scene(void) = default;

uint32_t Scene::AddMeshToLocalScene(const uint32_t local_scene_id,
                                    const MeshPtr& mesh_ptr) {
  assert(local_scene_id <= local_scenes_.size());

  uint32_t local_geom_id = uint32_t(-1);
  if (mesh_ptr.index() == kTriangleMesh) {
    // Triangle Mesh
    const std::shared_ptr<TriangleMesh> triangle_mesh =
        mpark::get<kTriangleMesh>(mesh_ptr);

    // BVH
    const std::vector<float>& vertices = triangle_mesh.get()->GetVertices();
    const std::vector<uint32_t>& vertex_ids =
        triangle_mesh.get()->GetVertexIds();

    const uint32_t num_vertices = triangle_mesh.get()->GetNumVertices();
    const uint32_t num_faces    = triangle_mesh.get()->GetNumFaces();

    local_geom_id = raytracer_.AddTriangleMeshToLocalScene(
        local_scene_id, vertices.data(), num_vertices, vertex_ids.data(),
        num_faces);
  } else if (mesh_ptr.index() == kCubicBezierCurveMesh) {
    // Cubic Bezier Curve Mesh
    const std::shared_ptr<CubicBezierCurveMesh> curve_mesh =
        mpark::get<kCubicBezierCurveMesh>(mesh_ptr);

    // BVH
    const std::vector<float>& vertices   = curve_mesh->GetVertices();
    const std::vector<uint32_t>& indices = curve_mesh->GetIndices();

    const uint32_t num_vertices = curve_mesh->GetNumVertices();
    const uint32_t num_indices  = curve_mesh->GetNumSegments();

    local_geom_id = raytracer_.AddCubicBezierCurveMeshToLocalScene(
        local_scene_id, vertices.data(), num_vertices, indices.data(),
        num_indices);
  } else {
    (void)local_scene_id;
  }

  LocalScene* local_scene = local_scenes_.at(local_scene_id).get();

  if (local_geom_id >= local_scene->meshes.size()) {
    local_scene->meshes.resize(local_geom_id + 1);
  }
  local_scene->meshes[local_geom_id] = mesh_ptr;

  return local_geom_id;
}

void Scene::AttachLightParamIdsToInstance(
    const uint32_t instance_id,
    const std::vector<std::vector<uint32_t>>& light_param_ids) {
  MeshInstance& instance = instances_.at(instance_id);
  if (instance.light_param_ids.size() != light_param_ids.size())
    throw std::runtime_error("light param error");

  instance.light_param_ids = light_param_ids;
}

void Scene::AttachMaterialParamIdsToInstance(
    const uint32_t instance_id,
    const std::vector<std::vector<uint32_t>>& material_ids) {
  MeshInstance& instance = instances_.at(instance_id);

  if (instance.local_scene.get()->meshes.size() != material_ids.size()) {
    throw std::runtime_error("material param error");
  }

  for (uint32_t mesh_id = 0; mesh_id < material_ids.size(); ++mesh_id) {
    const MeshPtr& mesh_ptr = instance.local_scene->meshes[mesh_id];

    const uint32_t num_primitive = GetNumPrimitive(mesh_ptr);

    if (material_ids[mesh_id].size() != num_primitive) {
      throw std::runtime_error("material param error");
    }
  }

  instance.material_ids = material_ids;
}

void Scene::CommitScene(void) {
  for (uint32_t i = 0; i < instances_.size(); ++i) {
    light_manager_->RegisterInstanceMesh(instances_[i], i);
  }

  light_manager_->Commit();

  raytracer_.GetSceneAABB(bmin_, bmax_);
}

uint32_t Scene::CreateInstance(const uint32_t local_scene_id,
                               const float transform[4][4]) {
  assert(local_scene_id < local_scenes_.size());

  MeshInstance instance;

  Matrix::Copy(transform, instance.transform_lg);  // [NOTE] local to global
  Matrix::Copy(transform, instance.transform_gl);
  Matrix::Inverse(instance.transform_gl);  // [NOTE] global to local

  LocalScene& local_scene = *(local_scenes_[local_scene_id]);

  for (MeshPtr& mesh_ptr : local_scene.meshes) {
    if (mesh_ptr.index() == kTriangleMesh) {
      const std::shared_ptr<TriangleMesh> triangle_mesh =
          mpark::get<kTriangleMesh>(mesh_ptr);

      // Material
      // TODO specify material ids
      instance.material_ids.emplace_back(triangle_mesh.get()->GetMaterials());

      // Light
      { instance.light_param_ids.emplace_back(); }
    } else if (mesh_ptr.index() == kCubicBezierCurveMesh) {
      // Cubic Bezier Curve Mesh
      const std::shared_ptr<CubicBezierCurveMesh> curve_mesh =
          mpark::get<kCubicBezierCurveMesh>(mesh_ptr);

      // Materilal
      // TODO specify material ids
      instance.material_ids.emplace_back(curve_mesh->GetMaterials());

      // Light
      { instance.light_param_ids.emplace_back(); }
    }
  }

  instance.local_scene = local_scenes_[local_scene_id];

  const uint32_t instance_id =
      raytracer_.CreateInstanceFromLocalScene(local_scene_id, transform);

  // Register Instance
  if (instance_id >= instances_.size()) {
    instances_.resize(instance_id + 1);
  }
  instances_[instance_id] = instance;

  return instance_id;
}

uint32_t Scene::CreateLocalScene(void) {
  const uint32_t local_scene_id = raytracer_.CreateLocalScene();
  if (local_scene_id >= local_scenes_.size()) {
    local_scenes_.resize(local_scene_id + 1);
  }
  local_scenes_[local_scene_id].reset(new LocalScene);
  return local_scene_id;
}

const MeshInstance& Scene::GetMeshInstance(const uint32_t instance_id) const {
#ifdef NDEBUG
  return instances_[instance_id];
#else
  return instances_.at(instance_id);
#endif
}

const LightManager* Scene::GetLightManager(void) const {
  return light_manager_.get();
}

const Texture* Scene::GetTexture(const uint32_t tex_id) const {
#ifdef NDEBUG
  return textures_[tex_id].get();
#else
  return textures_.at(tex_id).get();
#endif
}

const MaterialParameter* Scene::FetchMeshMaterialParameter(
    const TraceResult& trace_result) const {
  assert(trace_result.instance_id < instances_.size());
  const MeshInstance& instance = instances_[trace_result.instance_id];

  assert(trace_result.geom_id < instance.material_ids.size());
  const std::vector<uint32_t>& material_ids =
      instance.material_ids[trace_result.geom_id];

  if (material_ids[trace_result.prim_id] == uint32_t(-1)) {
    return nullptr;
  }

#ifdef NDEBUG
  return &(material_params_[material_ids[trace_result.prim_id]]);
#else
  return &(material_params_.at(material_ids.at(trace_result.prim_id)));
#endif
}

std::vector<MaterialParameter>* Scene::FetchMeshMaterialParameters(void) {
  return &material_params_;
}

float3 Scene::FetchMeshShadingNormal(const TraceResult& trace_result) const {
  assert(trace_result.instance_id < instances_.size());
  const MeshInstance& instance = instances_[trace_result.instance_id];

  assert(trace_result.geom_id < instance.local_scene->meshes.size());
  const MeshPtr& mesh_p = instance.local_scene->meshes[trace_result.geom_id];

  float3 ret;
  if (mesh_p.index() == kTriangleMesh) {
    // TODO transform
    ret = mpark::get<kTriangleMesh>(mesh_p)->FetchShadingNormal(
        trace_result.prim_id, trace_result.u, trace_result.v);
  } else if (mesh_p.index() == kCubicBezierCurveMesh) {
    ret = float3(trace_result.normal_g);
  } else {
    assert(false);
  }
  return ret;
}

float2 Scene::FetchMeshTexcoord(const TraceResult& trace_result) const {
  assert(trace_result.instance_id < instances_.size());
  const MeshInstance& instance = instances_[trace_result.instance_id];

  assert(trace_result.geom_id < instance.local_scene->meshes.size());
  const MeshPtr& mesh_p = instance.local_scene->meshes[trace_result.geom_id];

  float2 ret;
  if (mesh_p.index() == kTriangleMesh) {
    // TODO transform
    ret = mpark::get<kTriangleMesh>(mesh_p)->FetchTexcoord(
        trace_result.prim_id, trace_result.u, trace_result.v);
  } else if (mesh_p.index() == kCubicBezierCurveMesh) {
    // TODO
    ret = float2(0.f, 0.f);
  } else {
    assert(false);
  }
  return ret;
}

void Scene::FetchSceneAABB(float* bmin, float* bmax) const {
  bmin[0] = bmin_[0];
  bmin[1] = bmin_[1];
  bmin[2] = bmin_[2];

  bmax[0] = bmax_[0];
  bmax[1] = bmax_[1];
  bmax[2] = bmax_[2];
}

TraceResult Scene::TraceFirstHit1(const Ray& ray) const {
  return raytracer_.FirstHitTrace1(ray.ray_org.v, ray.ray_dir.v, ray.min_t,
                                   ray.max_t);
}

bool Scene::AnyHit1(const Ray& ray) const {
  return raytracer_.AnyHit1(ray.ray_org.v, ray.ray_dir.v, ray.min_t, ray.max_t);
}

}  // namespace pbrlab
