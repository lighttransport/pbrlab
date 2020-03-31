#ifndef PBRLAB_MESH_INSTANCE_H_
#define PBRLAB_MESH_INSTANCE_H_
#include <memory>
#include <vector>

#include "mesh/cubic-bezier-curve-mesh.h"
#include "mesh/triangle-mesh.h"

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#endif

#include "mpark/variant.hpp"

#ifdef __clang__
#pragma clang diagnostic pop
#endif

namespace pbrlab {

enum MeshType { kTriangleMesh = 0, kCubicBezierCurveMesh, kMeshNone };
using MeshPtr = mpark ::variant<std::shared_ptr<TriangleMesh>,
                                std::shared_ptr<CubicBezierCurveMesh>>;

struct MeshInstance {
  std::vector<MeshPtr> meshes;
  std::vector<std::vector<uint32_t>> material_ids;
  std::vector<std::vector<uint32_t>>
      light_param_ids;  // If light_param_ids[i].size() is
                        // 0, the mesh whose local geom
                        // id is i don't have emission

  float transform_lg[4][4];  // [NOTE] local to global
  float transform_gl[4][4];  // [NOTE] global to local
  // sR 0
  //  t 1
  //  v' = v * M
};

}  // namespace pbrlab
#endif  //  PBRLAB_MESH_INSTANCE_H_
