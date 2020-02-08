#ifndef PBRLAB_TRIANGLE_MESH_H_
#define PBRLAB_TRIANGLE_MESH_H_

#include <stdint.h>

#include <memory>
#include <string>
#include <vector>

#include "mesh/attribute.h"
#include "type.h"

namespace pbrlab {

class TriangleMesh {
public:
  TriangleMesh();
  TriangleMesh(const std::string name,
               const std::shared_ptr<Attribute>& attribute,
               const std::vector<uint32_t> vertex_ids,
               const std::vector<uint32_t> normal_ids,
               const std::vector<uint32_t> texcoord_ids,
               const std::vector<uint32_t> material_ids);

  float3 FetchGeometryNormal(const uint32_t prim_id) const;

  float3 FetchShadingNormal(const uint32_t prim_id, const float u,
                            const float v) const;

  float3 FetchLocalPosition(const uint32_t prim_id, const float u,
                            const float v) const;

  // TODO transform
  float FetchFaceArea(const uint32_t prim_id) const;

  const std::vector<uint32_t>& GetMaterials(void) const;
  uint32_t GetNumFaces(void) const;
  uint32_t GetNumVertices(void) const;
  std::string GetName(void) const;
  const std::vector<uint32_t>& GetVertexIds(void) const;
  const std::vector<float>& GetVertices(void) const;

private:
  uint32_t num_faces_;

  std::vector<uint32_t> vertex_ids_;    // 3(v1v2v3) * num_faces_
  std::vector<uint32_t> normal_ids_;    // 3(v1v2v3) * num_faces_
  std::vector<uint32_t> texcoord_ids_;  // 3(v1v2v3) * num_faces_

  std::vector<uint32_t> material_ids_;  // num_faces_ or 0

  std::shared_ptr<Attribute> pAttribute_;

  std::string name_;
};

}  // namespace pbrlab

#endif  // PBRLAB_TRIANGLE_MESH_H_
