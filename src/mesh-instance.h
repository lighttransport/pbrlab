#ifndef PBRLAB_MESH_INSTANCE_H_
#define PBRLAB_MESH_INSTANCE_H_
#include <memory>
#include <vector>

#include "local-scene.h"
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

struct MeshInstance {
  std::shared_ptr<LocalScene> local_scene;
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
