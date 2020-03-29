#ifndef PBRLAB_RAYTRACER_MESH_H_
#define PBRLAB_RAYTRACER_MESH_H_

#include <stdint.h>

namespace pbrlab {
namespace raytracer {

struct Mesh {
  uint32_t embree_local_scene_id = static_cast<uint32_t>(-1);
  uint32_t embree_local_geom_id  = static_cast<uint32_t>(-1);
};

struct TriangleMesh : Mesh {
  const float* vertices = nullptr;  // num_vertices * 4 (xyz+padding)
  const uint32_t* faces = nullptr;  // num_faces * 3
  uint32_t num_vertices = 0;
  uint32_t num_faces    = 0;
};

struct CubicBezierCurveMesh : Mesh {
  const float* vertices_thickness = nullptr;  // 4(xyz+thickness) * num vertices
  const uint32_t* indices         = nullptr;  // num_segments

  uint32_t num_vertices = 0;
  uint32_t num_segments = 0;
};

}  // namespace raytracer
}  // namespace pbrlab
#endif  // PBRLAB_RAYTRACER_MESH_H_
