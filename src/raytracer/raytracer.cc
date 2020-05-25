#include "raytracer/raytracer.h"

#include <embree3/rtcore.h>

#include "raytracer/raytracer_impl.h"

namespace pbrlab {
namespace raytracer {

Raytracer::Raytracer(void) : impl_(new Impl()) {}
Raytracer::~Raytracer(void) = default;

uint32_t Raytracer::AddTriangleMeshToLocalScene(const uint32_t local_scene_id,
                                                const float* vertices,
                                                const uint32_t num_vertices,
                                                const uint32_t* faces,
                                                const uint32_t num_faces) {
  return impl_->AddTriangleMeshToLocalScene(local_scene_id, vertices,
                                            num_vertices, faces, num_faces);
}

uint32_t Raytracer::AddCubicBezierCurveMeshToLocalScene(
    const uint32_t local_scene_id, const float* vertices_thickness,
    const uint32_t num_vertices, const uint32_t* indices,
    const uint32_t num_segments) {
  return impl_->AddCubicBezierCurveMeshToLocalScene(
      local_scene_id, vertices_thickness, num_vertices, indices, num_segments);
}

uint32_t Raytracer::CreateLocalScene(void) { return impl_->CreateLocalScene(); }

uint32_t Raytracer::CreateInstanceFromLocalScene(uint32_t local_scene_id,
                                                 const float transform[4][4]) {
  return impl_->CreateInstanceFromLocalScene(local_scene_id, transform);
}

bool Raytracer::GetSceneAABB(float* bmin, float* bmax) const {
  return impl_->GetSceneAABB(bmin, bmax);
}

TraceResult Raytracer::FirstHitTrace1(const float* ray_org,
                                      const float* ray_dir, const float min_t,
                                      const float max_t) const {
  return impl_->FirstHitTrace1(ray_org, ray_dir, min_t, max_t);
}

bool Raytracer::AnyHit1(const float* ray_org, const float* ray_dir,
                        const float min_t, const float max_t) const {
  return impl_->AnyHit1(ray_org, ray_dir, min_t, max_t);
}

}  // namespace raytracer
}  // namespace pbrlab
