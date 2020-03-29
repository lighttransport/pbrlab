#ifndef PBRLAB_CUBIC_BEZIER_CURVE_MESH_H_
#define PBRLAB_CUBIC_BEZIER_CURVE_MESH_H_
#include <stdint.h>

#include <memory>
#include <vector>

#include "mesh/attribute.h"
namespace pbrlab {

class CubicBezierCurveMesh {
public:
  explicit CubicBezierCurveMesh();
  explicit CubicBezierCurveMesh(const std::string& name,
                                const std::shared_ptr<CurveAttribute> attribute,
                                const std::vector<uint32_t>& indices);

private:
  std::shared_ptr<CurveAttribute> pAttribute_;
  std::vector<uint32_t> indices_;
  std::string name_;
};

}  // namespace pbrlab

#endif  // PBRLAB_CUBIC_BEZIER_CURVE_MESH_H_
