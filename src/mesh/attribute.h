#ifndef PBRLAB_ATTRIBUTE_H_
#define PBRLAB_ATTRIBUTE_H_

#include <vector>

namespace pbrlab {
struct Attribute {
  std::vector<float> vertices;   // 4(xyzw) * num vertices_  (w = 1.0f)
  std::vector<float> normals;    // 4(xyzw) * num vertices_  or 0 (w = 1.0f)
  std::vector<float> texcoords;  // 2(uv) * num_vertices or 0
};                               // namespace mesh

struct CurveAttribute {
  std::vector<float> vertices;  // 4(xyz+thickness) * num vertices_
};

}  // namespace pbrlab

#endif  // PBRLAB_ATTRIBUTE_H_
