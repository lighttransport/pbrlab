#ifndef PBRLAB_LOCAL_SCENE_H_
#define PBRLAB_LOCAL_SCENE_H_

#include <vector>

#include "mesh/mesh.h"

namespace pbrlab {

struct LocalScene {
  std::vector<MeshPtr> meshes;
};

}  // namespace pbrlab

#endif  // PBRLAB_LOCAL_SCENE_H_
