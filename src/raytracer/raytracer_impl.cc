#include <cmath>
#include <cassert>

#include <memory>
#include <vector>
#include <limits>
#include <cstring>

#include "raytracer/mesh.h"
#include "raytracer/raytracer_impl.h"

namespace pbrlab {
namespace raytracer {

//const constexpr float kMaxRayDistance = 1.844E18f;
const constexpr float kMaxRayDistance = std::numeric_limits<float>::infinity();

// Custom intersection context
struct IntersectContext {
  RTCIntersectArguments args;
  // TODO
  IntersectContext() { rtcInitIntersectArguments(&args); }
};

Raytracer::Impl::Impl(void) {
  embree_device_       = rtcNewDevice(nullptr);
  embree_global_scene_ = rtcNewScene(embree_device_);
};

Raytracer::Impl::~Impl() {
  for (auto& local_scene : local_scenes_) {
    rtcReleaseScene(local_scene.embree_scene);
  }
  if (embree_global_scene_ != nullptr) {
    rtcReleaseScene(embree_global_scene_);
  }
  if (embree_device_ != nullptr) {
    rtcReleaseDevice(embree_device_);
  }
}

static void IntersectionFilter(const RTCFilterFunctionNArguments* args) {
  if (args->context == nullptr) return;
  // TODO
}

uint32_t Raytracer::Impl::CreateInstanceFromLocalScene(
    uint32_t local_scene_id, const float transform[4][4]) {
  float transform_[4][4];
  for (size_t i = 0; i < 4; i++) {
    for (size_t j = 0; j < 4; j++) {
      if (transform == nullptr) {
        transform_[i][j] = (i == j) ? 1.0f : 0.0f;
      } else {
        transform_[i][j] = transform[i][j];
      }
    }
  }
  RTCGeometry instance =
      rtcNewGeometry(embree_device_, RTC_GEOMETRY_TYPE_INSTANCE);
  // TODO error message
  rtcSetGeometryInstancedScene(instance,
                               local_scenes_.at(local_scene_id).embree_scene);
  rtcSetGeometryTimeStepCount(instance, 1);

  // transform
  rtcSetGeometryTransform(instance, /* timeStep */ 0,
                          RTC_FORMAT_FLOAT4X4_COLUMN_MAJOR,
                          reinterpret_cast<const float*>(transform_));

  rtcCommitGeometry(instance);

  const uint32_t instance_id = create_instance_counter_;
  rtcAttachGeometryByID(embree_global_scene_, instance, instance_id);
  create_instance_counter_++;

  rtcReleaseGeometry(instance);

  rtcCommitScene(embree_global_scene_);

  return instance_id;
}

uint32_t Raytracer::Impl::CreateLocalScene(void) {
  const uint32_t local_scene_id = uint32_t(local_scenes_.size());
  local_scenes_.emplace_back();
  local_scenes_.back().embree_scene = rtcNewScene(embree_device_);
  return local_scene_id;
}

uint32_t Raytracer::Impl::AddTriangleMeshToLocalScene(
    const uint32_t local_scene_id, const float* vertices,
    const uint32_t num_vertices, const uint32_t* faces,
    const uint32_t num_faces) {
  RTCGeometry geom = rtcNewGeometry(embree_device_, RTC_GEOMETRY_TYPE_TRIANGLE);

  const uint32_t triangle_mesh_id =
      static_cast<uint32_t>(triangle_meshes_.size());

  triangle_meshes_.emplace_back();
  TriangleMesh& triangle_mesh = triangle_meshes_.back();
  triangle_mesh.num_vertices  = num_vertices;
  triangle_mesh.num_faces     = num_faces;

  // FIXME(LTE): vertices are flattened across scene objects
  // So create single shared buffer for Vertex, or create vertex buffer per scene object.

  // vertices = 4 floats(xyzw)
 #if 0
  float *embree_vertices = reinterpret_cast<float *>(rtcSetNewGeometryBuffer(geom, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, /* stride */sizeof(float) * 4, num_vertices));
  assert(embree_vertices);
  memcpy(embree_vertices, vertices, sizeof(float) * 4 * num_vertices);
  triangle_mesh.vertices = embree_vertices;

  uint32_t *embree_faces = reinterpret_cast<uint32_t *>(rtcSetNewGeometryBuffer(geom, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, /* stride */sizeof(uint32_t) * 3, num_faces));
  assert(embree_faces);
  memcpy(embree_faces, faces, sizeof(uint32_t) * 3 * num_faces);
  triangle_mesh.faces = embree_faces;
#else

  triangle_mesh.vertices = vertices;
  rtcSetSharedGeometryBuffer(geom, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3,
                             triangle_mesh.vertices, 0, sizeof(float) * 4,
                             num_vertices);


  triangle_mesh.faces = faces;
  rtcSetSharedGeometryBuffer(geom, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3,
                             triangle_mesh.faces, 0, sizeof(uint32_t) * 3,
                             num_faces);
#endif

  //rtcSetGeometryIntersectFilterFunction(geom, IntersectionFilter);
  rtcCommitGeometry(geom);

  LocalScene& local_scene           = local_scenes_.at(local_scene_id);
  const RTCScene local_scene_embree = local_scene.embree_scene;

  const uint32_t local_geom_id = uint32_t(local_scene.mesh_data_ids.size());

  local_scene.mesh_data_ids.emplace_back(kTriangleMesh, triangle_mesh_id);

  rtcAttachGeometryByID(local_scene_embree, geom, local_geom_id);

  rtcCommitScene(local_scene_embree);

  rtcReleaseGeometry(geom);

  return local_geom_id;
}

uint32_t Raytracer::Impl::AddCubicBezierCurveMeshToLocalScene(
    const uint32_t local_scene_id, const float* vertices_thickness,
    const uint32_t num_vertices, const uint32_t* indices,
    const uint32_t num_segments) {
  RTCGeometry geom =
      rtcNewGeometry(embree_device_, RTC_GEOMETRY_TYPE_FLAT_BEZIER_CURVE);

  const uint32_t cubic_bezier_curve_mesh_id =
      uint32_t(cubic_bezier_curve_meshes_.size());

  cubic_bezier_curve_meshes_.emplace_back();
  CubicBezierCurveMesh& cubic_bezier_curve_mesh =
      cubic_bezier_curve_meshes_.back();
  cubic_bezier_curve_mesh.num_vertices = num_vertices;
  cubic_bezier_curve_mesh.num_segments = num_segments;

  cubic_bezier_curve_mesh.vertices_thickness = vertices_thickness;

  rtcSetSharedGeometryBuffer(geom, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT4,
                             vertices_thickness, 0, sizeof(float) * 4,
                             num_vertices);

  cubic_bezier_curve_mesh.indices = indices;
  rtcSetSharedGeometryBuffer(geom, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT,
                             indices, 0, sizeof(uint32_t), num_segments);

  //rtcSetGeometryIntersectFilterFunction(geom, IntersectionFilter);
  rtcCommitGeometry(geom);

  LocalScene& local_scene           = local_scenes_[local_scene_id];
  const RTCScene local_scene_embree = local_scene.embree_scene;

  const uint32_t local_geom_id = uint32_t(local_scene.mesh_data_ids.size());
  local_scene.mesh_data_ids.emplace_back(kCubicBezierCurveMesh,
                                         cubic_bezier_curve_mesh_id);

  rtcAttachGeometryByID(local_scene_embree, geom, local_geom_id);

  rtcCommitScene(local_scene_embree);

  rtcReleaseGeometry(geom);

  return local_geom_id;
}

bool Raytracer::Impl::GetSceneAABB(float* bmin, float* bmax) const {
  if (embree_global_scene_ == nullptr) return false;
  RTCBounds rtc_bounds;
  rtcGetSceneBounds(embree_global_scene_, &rtc_bounds);
  bmin[0] = rtc_bounds.lower_x;
  bmin[1] = rtc_bounds.lower_y;
  bmin[2] = rtc_bounds.lower_z;

  bmax[0] = rtc_bounds.upper_x;
  bmax[1] = rtc_bounds.upper_y;
  bmax[2] = rtc_bounds.upper_z;
  return true;
}

static void Normalize(float* v) {
  // TODO : increase efficiency
  const float inv_norm =
      1.0f / std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
  v[0] *= inv_norm;
  v[1] *= inv_norm;
  v[2] *= inv_norm;
}
static TraceResult EmbreeRayToTraceResult(const RTCRayHit& rayhit) {

  TraceResult tr;
  if (rayhit.hit.instID[0] != RTC_INVALID_GEOMETRY_ID &&
      rayhit.hit.geomID != RTC_INVALID_GEOMETRY_ID &&
      rayhit.hit.primID != RTC_INVALID_GEOMETRY_ID) {

    tr.normal_g[0] = rayhit.hit.Ng_x;
    tr.normal_g[1] = rayhit.hit.Ng_y;
    tr.normal_g[2] = rayhit.hit.Ng_z;
    // NOTE : embree do not normalize Ng
    Normalize(tr.normal_g);

    tr.t = rayhit.ray.tfar;
    tr.u = rayhit.hit.u;
    tr.v = rayhit.hit.v;

    tr.instance_id = rayhit.hit.instID[0];
    tr.geom_id     = rayhit.hit.geomID;
    tr.prim_id     = rayhit.hit.primID;
  }
  return tr;
}

static RTCRayHit SetRayHit(const float* ray_org, const float* ray_dir,
                           const float min_t, const float max_t) {
  RTCRayHit rayhit;
  rayhit.ray.org_x = ray_org[0];
  rayhit.ray.org_y = ray_org[1];
  rayhit.ray.org_z = ray_org[2];

  rayhit.ray.dir_x = ray_dir[0];
  rayhit.ray.dir_y = ray_dir[1];
  rayhit.ray.dir_z = ray_dir[2];

  rayhit.ray.tnear = min_t;
  rayhit.ray.tfar  = std::min(max_t, kMaxRayDistance);
  rayhit.ray.mask = 0xffffffff;
  rayhit.ray.flags = 0;

  rayhit.hit.geomID    = static_cast<uint32_t>(RTC_INVALID_GEOMETRY_ID);
  //rayhit.hit.primID    = static_cast<uint32_t>(RTC_INVALID_GEOMETRY_ID);
  rayhit.hit.instID[0] = static_cast<uint32_t>(RTC_INVALID_GEOMETRY_ID);

  return rayhit;
}

TraceResult Raytracer::Impl::FirstHitTrace1(const float* ray_org,
                                            const float* ray_dir,
                                            const float min_t,
                                            const float max_t) const {
  RTCRayHit rayhit = SetRayHit(ray_org, ray_dir, min_t, max_t);

  //IntersectContext context;
  rtcIntersect1(embree_global_scene_, &rayhit); //,&(context.args), &rayhit);

  return EmbreeRayToTraceResult(rayhit);
}

bool Raytracer::Impl::AnyHit1(const float* ray_org, const float* ray_dir,
                              const float min_t, const float max_t) const {
  RTCRayHit rayhit = SetRayHit(ray_org, ray_dir, min_t, max_t);
  //IntersectContext context;
  const float tmp = rayhit.ray.tfar;
  rtcOccluded1(embree_global_scene_, &(rayhit.ray)); //&(context).context, &(rayhit.ray));
  return (std::fabs(tmp - rayhit.ray.tfar) > 1e-6f);
}

}  // namespace raytracer
}  // namespace pbrlab
