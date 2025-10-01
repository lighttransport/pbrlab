#ifndef PBRLAB_RAYTRACER_TINYBVH_IMPL_H_
#define PBRLAB_RAYTRACER_TINYBVH_IMPL_H_

#include <tuple>
#include <vector>

#include "raytracer/mesh.h"
#include "raytracer/raytracer.h"

// Forward declaration for tinybvh types
namespace tinybvh {
  class BVH;
  struct bvhvec3;
  struct bvhvec4;
  struct Ray;
}

namespace pbrlab {
namespace raytracer {

struct TinyBVHLocalScene {
  std::unique_ptr<tinybvh::BVH> bvh;
  std::vector<std::tuple<GeometryType, uint32_t>> mesh_data_ids;
  std::vector<tinybvh::bvhvec4> triangle_vertices;  // Flattened triangle vertices
  bool needs_rebuild = true;
};

class TinyBVHRaytracer {
public:
  TinyBVHRaytracer(void);
  ~TinyBVHRaytracer();

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
  void RebuildScene();
  
  std::vector<TinyBVHLocalScene> local_scenes_;
  std::vector<TriangleMesh> triangle_meshes_;
  std::vector<CubicBezierCurveMesh> cubic_bezier_curve_meshes_;

  // Global scene data
  std::unique_ptr<tinybvh::BVH> global_bvh_;
  std::vector<tinybvh::bvhvec4> global_triangle_vertices_;
  std::vector<uint32_t> triangle_to_instance_map_;  // Maps global triangle ID to instance
  std::vector<uint32_t> triangle_to_local_geom_map_;  // Maps global triangle ID to local geom
  std::vector<uint32_t> triangle_to_local_prim_map_;  // Maps global triangle ID to local primitive
  
  uint32_t create_instance_counter_ = 0;
  bool global_scene_needs_rebuild_ = true;
  
  struct InstanceData {
    uint32_t local_scene_id;
    float transform[4][4];
  };
  std::vector<InstanceData> instances_;
};

}  // namespace raytracer
}  // namespace pbrlab

#endif  // PBRLAB_RAYTRACER_TINYBVH_IMPL_H_