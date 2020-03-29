#include "mesh/cubic-bezier-curve-mesh.h"

namespace pbrlab {

CubicBezierCurveMesh::CubicBezierCurveMesh() = default;

CubicBezierCurveMesh::CubicBezierCurveMesh(
    const std::string& name, const std::shared_ptr<CurveAttribute> attribute,
    const std::vector<uint32_t>& indices)
    : pAttribute_(attribute), indices_(indices), name_(name) {}
}  // namespace pbrlab
