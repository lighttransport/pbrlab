#include "mesh/cubic-bezier-curve-mesh.h"

namespace pbrlab {

CubicBezierCurveMesh::CubicBezierCurveMesh() = default;

CubicBezierCurveMesh::CubicBezierCurveMesh(
    const std::string& name, const std::shared_ptr<CurveAttribute> attribute,
    const std::vector<uint32_t>& indices,
    const std::vector<uint32_t>& material_ids)
    : pAttribute_(attribute),
      indices_(indices),
      material_ids_(material_ids),
      name_(name) {}

const std::vector<uint32_t>& CubicBezierCurveMesh::GetIndices(void) const {
  return indices_;
}
const std::vector<uint32_t>& CubicBezierCurveMesh::GetMaterials(void) const {
  return material_ids_;
}
std::string CubicBezierCurveMesh::GetName(void) const { return name_; }
uint32_t CubicBezierCurveMesh::GetNumSegments(void) const {
  return uint32_t(indices_.size());
}
uint32_t CubicBezierCurveMesh::GetNumVertices(void) const {
  return uint32_t(pAttribute_->vertices.size() / 4);
}
const std::vector<float>& CubicBezierCurveMesh::GetVertices(void) const {
  return pAttribute_->vertices;
}

void CubicBezierCurveMesh::SetMaterialId(const uint32_t material_id,
                                         const uint32_t segment_id) {
#ifdef NDEBUG
  material_ids_[segment_id] = material_id;
#else
  material_ids_.at(segment_id) = material_id;
#endif
}

}  // namespace pbrlab
