#ifndef PBRLAB_CUBIC_BEZIER_CURVE_MESH_H_
#define PBRLAB_CUBIC_BEZIER_CURVE_MESH_H_
#include <stdint.h>

#include <memory>
#include <string>
#include <vector>

#include "mesh/attribute.h"
namespace pbrlab {

class CubicBezierCurveMesh {
public:
  explicit CubicBezierCurveMesh();
  explicit CubicBezierCurveMesh(const std::string& name,
                                const std::shared_ptr<CurveAttribute> attribute,
                                const std::vector<uint32_t>& indices,
                                const std::vector<uint32_t>& materila_ids);

  const std::vector<uint32_t>& GetIndices(void) const;
  uint32_t GetNumSegments(void) const;
  uint32_t GetNumVertices(void) const;
  const std::vector<uint32_t>& GetMaterials(void) const;
  std::string GetName(void) const;
  const std::vector<float>& GetVertices(void) const;

  void SetMaterialId(const uint32_t material_id, const uint32_t segment_id);

private:
  std::shared_ptr<CurveAttribute> pAttribute_;
  std::vector<uint32_t> indices_;
  std::vector<uint32_t> material_ids_;
  std::string name_;
};

}  // namespace pbrlab

#endif  // PBRLAB_CUBIC_BEZIER_CURVE_MESH_H_
