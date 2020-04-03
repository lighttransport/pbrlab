#include "mesh/triangle-mesh.h"

#include <assert.h>
#include <math.h>

#include <algorithm>
#include <iostream>

#include "pbrlab_math.h"
#include "type.h"

namespace pbrlab {

float3 CalcGeometryNormal(const float3& p0, const float3& p1, const float3& p2);

TriangleMesh::TriangleMesh() = default;

TriangleMesh::TriangleMesh(const std::string name,
                           const std::shared_ptr<Attribute>& attribute,
                           const std::vector<uint32_t> vertex_ids,
                           const std::vector<uint32_t> normal_ids,
                           const std::vector<uint32_t> texcoord_ids,
                           const std::vector<uint32_t> material_ids)
    : pAttribute_(attribute), name_(name) {
  if (vertex_ids.size() % 3 != 0) {
    // TODO use logger
    std::cerr << "error! wrong vertex ids size" << std::endl;
  }
  assert(vertex_ids.size() % 3 == 0);
  num_faces_ = static_cast<uint32_t>(vertex_ids.size() / 3);

  vertex_ids_ = vertex_ids;

  if (normal_ids.size() == num_faces_ * 3) {
    normal_ids_ = normal_ids;
  } else {
    normal_ids_.resize(num_faces_ * 3);
    std::fill(normal_ids_.begin(), normal_ids_.end(), uint32_t(-1));
  }

  if (texcoord_ids.size() == num_faces_ * 3) {
    texcoord_ids_ = texcoord_ids;
  } else {
    texcoord_ids_.resize(num_faces_ * 3);
    std::fill(texcoord_ids_.begin(), texcoord_ids_.end(), uint32_t(-1));
  }

  if (material_ids.size() == num_faces_) {
    material_ids_ = material_ids;
  } else {
    material_ids_.resize(num_faces_ * 3);
    std::fill(material_ids_.begin(), material_ids_.end(), uint32_t(-1));
  }
}

const std::vector<uint32_t>& TriangleMesh::GetMaterials(void) const {
  return material_ids_;
}

std::string TriangleMesh::GetName(void) const { return name_; }

float3 TriangleMesh::FetchGeometryNormal(const uint32_t prim_id) const {
  assert(prim_id < num_faces_);
#ifdef NDEBUG
  return CalcGeometryNormal(
      float3(pAttribute_->vertices.data() + vertex_ids_[prim_id * 3 + 0] * 4),
      float3(pAttribute_->vertices.data() + vertex_ids_[prim_id * 3 + 1] * 4),
      float3(pAttribute_->vertices.data() + vertex_ids_[prim_id * 3 + 2] * 4));
#else
  return CalcGeometryNormal(
      float3(&(pAttribute_->vertices.at(vertex_ids_[prim_id * 3 + 0] * 4))),
      float3(&(pAttribute_->vertices.at(vertex_ids_[prim_id * 3 + 1] * 4))),
      float3(&(pAttribute_->vertices.at(vertex_ids_[prim_id * 3 + 2] * 4))));
#endif
}

float3 TriangleMesh::FetchShadingNormal(const uint32_t prim_id, const float u,
                                        const float v) const {
  assert(prim_id < num_faces_);
  float3 ret;
  if (normal_ids_[prim_id * 3 + 0] == static_cast<uint32_t>(-1) ||
      normal_ids_[prim_id * 3 + 1] == static_cast<uint32_t>(-1) ||
      normal_ids_[prim_id * 3 + 2] == static_cast<uint32_t>(-1)) {
    ret = FetchGeometryNormal(prim_id);
  } else {
#ifdef NDEBUG
    ret = vnormalized(Lerp3(
        float3(pAttribute_->normals.data() + normal_ids_[prim_id * 3 + 0] * 4),
        float3(pAttribute_->normals.data() + normal_ids_[prim_id * 3 + 1] * 4),
        float3(pAttribute_->normals.data() + normal_ids_[prim_id * 3 + 2] * 4),
        u, v));
#else
    ret = vnormalized(Lerp3(
        float3(&(pAttribute_->normals.at(normal_ids_[prim_id * 3 + 0] * 4))),
        float3(&(pAttribute_->normals.at(normal_ids_[prim_id * 3 + 1] * 4))),
        float3(&(pAttribute_->normals.at(normal_ids_[prim_id * 3 + 2] * 4))), u,
        v));
#endif
  }
  return ret;
}
float3 TriangleMesh::FetchLocalPosition(const uint32_t prim_id, const float u,
                                        const float v) const {
  assert(prim_id < num_faces_);
  const float3& p0 =
      float3(pAttribute_->vertices.data() + vertex_ids_[prim_id * 3 + 0] * 4);
  const float3& p1 =
      float3(pAttribute_->vertices.data() + vertex_ids_[prim_id * 3 + 1] * 4);
  const float3 p2 =
      float3(pAttribute_->vertices.data() + vertex_ids_[prim_id * 3 + 2] * 4);
  return Lerp3(p0, p1, p2, u, v);
}

float TriangleMesh::FetchFaceArea(const uint32_t prim_id) const {
  const float3& p0 =
      float3(pAttribute_->vertices.data() + vertex_ids_[prim_id * 3 + 0] * 4);
  const float3& p1 =
      float3(pAttribute_->vertices.data() + vertex_ids_[prim_id * 3 + 1] * 4);
  const float3& p2 =
      float3(pAttribute_->vertices.data() + vertex_ids_[prim_id * 3 + 2] * 4);

  // return (p1 - p0).cross(p2 - p0).norm() * 0.5f;
  return vlength(vcross(p1 - p0, p2 - p0)) * 0.5f;
}

uint32_t TriangleMesh::GetNumFaces(void) const { return num_faces_; }
uint32_t TriangleMesh::GetNumVertices(void) const {
  return uint32_t(pAttribute_->vertices.size() / 3);
}

const std::vector<uint32_t>& TriangleMesh::GetVertexIds(void) const {
  return vertex_ids_;
}
const std::vector<float>& TriangleMesh::GetVertices(void) const {
  return pAttribute_->vertices;
}

void TriangleMesh::SetMaterialId(const uint32_t material_id,
                                 const uint32_t prim_id) {
#ifdef NDEBUG
  material_ids_[prim_id] = material_id;
#else
  material_ids_.at(prim_id) = material_id;
#endif
}

float3 CalcGeometryNormal(const float3& p0, const float3& p1,
                          const float3& p2) {
  return vnormalized(vcross(p1 - p0, p2 - p1));
}

}  // namespace pbrlab
