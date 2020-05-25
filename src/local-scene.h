#ifndef PBRLAB_LOCAL_SCENE_H_
#define PBRLAB_LOCAL_SCENE_H_

#include <vector>

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#endif

#include "mpark/variant.hpp"

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#include "mesh/cubic-bezier-curve-mesh.h"
#include "mesh/triangle-mesh.h"

namespace pbrlab {

enum MeshType { kTriangleMesh = 0, kCubicBezierCurveMesh, kMeshNone };

using MeshPtr = mpark ::variant<std::shared_ptr<TriangleMesh>,
                                std::shared_ptr<CubicBezierCurveMesh>>;

struct LocalScene {
  std::vector<MeshPtr> meshes;
};

}  // namespace pbrlab

#endif  // PBRLAB_LOCAL_SCENE_H_
