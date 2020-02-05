#include "raytracer/raytracer.h"

#include <embree3/rtcore.h>

#include "raytracer/raytracer_impl.h"

namespace pbrlab {
namespace raytracer {

Raytracer::Raytracer(void) : impl_(new Impl()) {}
Raytracer::~Raytracer(void) = default;

void Raytracer::RegisterNewTriangleMesh(
    const float* vertices, const uint32_t num_vertices, const uint32_t* faces,
    const uint32_t num_faces, const float transform[4][4],
    uint32_t* instance_id, uint32_t* local_scene_id, uint32_t* local_geom_id) {
  return impl_->RegisterNewTriangleMesh(vertices, num_vertices, faces,
                                        num_faces, transform, instance_id,
                                        local_scene_id, local_geom_id);
}

bool Raytracer::GetSceneAABB(float* bmin, float* bmax) const {
  return impl_->GetSceneAABB(bmin, bmax);
}

TraceResult Raytracer::FirstHitTrace1(const float* ray_org,
                                      const float* ray_dir, const float min_t,
                                      const float max_t) {
  return impl_->FirstHitTrace1(ray_org, ray_dir, min_t, max_t);
}

bool Raytracer::AnyHit1(const float* ray_org, const float* ray_dir,
                        const float min_t, const float max_t) {
  return impl_->AnyHit1(ray_org, ray_dir, min_t, max_t);
}

}  // namespace raytracer
}  // namespace pbrlab
