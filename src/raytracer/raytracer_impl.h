#ifndef PBRLAB_RAYTRACER_IMPL_H_
#define PBRLAB_RAYTRACER_IMPL_H_

#include <tuple>
#include <vector>

#include "embree3/rtcore.h"
#include "raytracer/mesh.h"
#include "raytracer/raytracer.h"

namespace pbrlab {
namespace raytracer {

struct LocalScene {
  RTCScene embree_scene;
  std::vector<std::tuple<GeometryType, uint32_t>> mesh_data_ids;
};

class Raytracer::Impl {
public:
  Impl(void);
  ~Impl();

  uint32_t AddTriangleMeshToLocalScene(const uint32_t local_scene_id,
                                       const float* vertices,
                                       const uint32_t num_vertices,
                                       const uint32_t* faces,
                                       const uint32_t num_faces);

  uint32_t AddCubicBezierCurveMeshToLocalScene(const uint32_t local_scene_id,
                                               const float* vertices_thickness,
                                               const uint32_t num_vertices,
                                               const uint32_t* indices,
                                               const uint32_t num_segments);

  uint32_t CreateLocalScene(void);

  // return new instance id
  uint32_t CreateInstanceFromLocalScene(uint32_t local_scene_id,
                                        const float transform[4][4]);

  bool GetSceneAABB(float* bmin, float* bmax) const;

  TraceResult FirstHitTrace1(const float* ray_org, const float* ray_dir,
                             const float min_t, const float max_t) const;

  bool AnyHit1(const float* ray_org, const float* ray_dir, const float min_t,
               const float max_t) const;

private:
  RTCDevice embree_device_      = nullptr;
  RTCScene embree_global_scene_ = nullptr;
  std::vector<LocalScene> local_scenes_;

  std::vector<TriangleMesh> triangle_meshes_;
  std::vector<CubicBezierCurveMesh> cubic_bezier_curve_meshes_;

  uint32_t create_instance_counter_ = 0;
};

}  // namespace raytracer
}  // namespace pbrlab

#endif  // PBRLAB_RAYTRACER_IMPL_H_
