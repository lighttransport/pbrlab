#ifndef PBRLAB_MESH_H_
#define PBRLAB_MESH_H_

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

inline std::string GetName(const MeshPtr& mesh_ptr) {
  if (mesh_ptr.index() == kTriangleMesh) {
    return mpark::get<kTriangleMesh>(mesh_ptr).get()->GetName();
  } else if (mesh_ptr.index() == kCubicBezierCurveMesh) {
    return mpark::get<kCubicBezierCurveMesh>(mesh_ptr).get()->GetName();
  }
  assert(false);

  return "";
}

inline uint32_t GetNumPrimitive(const MeshPtr& mesh_ptr) {
  if (mesh_ptr.index() == kTriangleMesh) {
    return mpark::get<kTriangleMesh>(mesh_ptr).get()->GetNumFaces();
  } else if (mesh_ptr.index() == kCubicBezierCurveMesh) {
    return mpark::get<kCubicBezierCurveMesh>(mesh_ptr).get()->GetNumSegments();
  }
  assert(false);
  return uint32_t(-1);
}

}  // namespace pbrlab
#endif  // PBRLAB_MESH_H_
