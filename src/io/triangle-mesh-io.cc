#include "io/triangle-mesh-io.h"

#include <iostream>

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#endif

#if defined(__cplusplus) && __cplusplus >= 201703L && defined(__has_include)
#if __has_include(<filesystem>)
#define GHC_USE_STD_FS
#include <filesystem>
namespace fs = std::filesystem;
#endif
#endif
#ifndef GHC_USE_STD_FS
#include <ghc/filesystem.hpp>
namespace fs = ghc::filesystem;
#endif

#include "io/tiny_obj_loader.h"

#ifdef __clang__
#pragma clang diagnostic pop
#endif

namespace pbrlab {
namespace io {

static float ParseTinyObjMaterialFloat(const std::string& param_name,
                                       const tinyobj::material_t& material) {
  std::atof(material.unknown_parameter.find(param_name)->second.c_str());
}

static void ParseTinyObjMaterialFloat3(const std::string& param_name,
                                       const tinyobj::material_t& material,
                                       float* x, float* y, float* z) {
  double x_ = 0.0;
  double y_ = 0.0;
  double z_ = 0.0;
  // TODO : Use c++ function and use T type variable
  sscanf(material.unknown_parameter.find(param_name)->second.c_str(),
         "%lf %lf %lf", &x_, &y_, &z_);
  *x = static_cast<float>(x_);
  *y = static_cast<float>(y_);
  *z = static_cast<float>(z_);
}

bool ConvertTinyObjMaterialFloat(const std::string& param_name,
                                 const tinyobj::material_t material,
                                 float* out) {
  const bool find =
      (material.unknown_parameter.count(param_name) > static_cast<size_t>(0));
  if (find) {
    *out = ParseTinyObjMaterialFloat(param_name, material);
  }
  return find;
}

bool ConvertTinyObjMaterialFloat3(const std::string& param_name,
                                  const tinyobj::material_t& material,
                                  float* out) {
  const bool find =
      (material.unknown_parameter.count(param_name) > static_cast<size_t>(0));
  if (material.unknown_parameter.count(param_name) > static_cast<size_t>(0)) {
    float x, y, z;
    ParseTinyObjMaterialFloat3(param_name, material, &x, &y, &z);
    out[0] = x;
    out[1] = y;
    out[2] = z;
  }

  return find;
}

static MaterialParameter ParseTinyObjMaterial(
    const tinyobj::material_t& material) {
  MaterialParameter ret;

  CyclesPrincipledBsdfParameter cycles_principled_bsdf_parameter;

  ConvertTinyObjMaterialFloat3("base_color", material,
                               cycles_principled_bsdf_parameter.base_color.v);

  ConvertTinyObjMaterialFloat3(
      "subsurface_radius", material,
      cycles_principled_bsdf_parameter.subsurface_radius.v);
  ConvertTinyObjMaterialFloat3(
      "subsurface_color", material,
      cycles_principled_bsdf_parameter.subsurface_color.v);

  ConvertTinyObjMaterialFloat("metallic", material,
                              &(cycles_principled_bsdf_parameter.metallic));

  ConvertTinyObjMaterialFloat("specular", material,
                              &(cycles_principled_bsdf_parameter.specular));

  ConvertTinyObjMaterialFloat("roughness", material,
                              &(cycles_principled_bsdf_parameter.roughness));

  ConvertTinyObjMaterialFloat("anisotropic", material,
                              &(cycles_principled_bsdf_parameter.anisotropic));
  ConvertTinyObjMaterialFloat(
      "anisotropic_rotation", material,
      &(cycles_principled_bsdf_parameter.anisotropic_rotation));

  ConvertTinyObjMaterialFloat("sheen", material,
                              &(cycles_principled_bsdf_parameter.sheen));
  ConvertTinyObjMaterialFloat("sheen_tint", material,
                              &(cycles_principled_bsdf_parameter.sheen_tint));

  ConvertTinyObjMaterialFloat("clearcoat", material,
                              &(cycles_principled_bsdf_parameter.clearcoat));
  ConvertTinyObjMaterialFloat(
      "clearcoat_roughness", material,
      &(cycles_principled_bsdf_parameter.clearcoat_roughness));

  ConvertTinyObjMaterialFloat("ior", material,
                              &(cycles_principled_bsdf_parameter.ior));

  ConvertTinyObjMaterialFloat("transmission", material,
                              &(cycles_principled_bsdf_parameter.transmission));
  ConvertTinyObjMaterialFloat(
      "transmission_roughness", material,
      &(cycles_principled_bsdf_parameter.transmission_roughness));

  ret = cycles_principled_bsdf_parameter;
  return ret;
}

bool LoadTriangleMeshFromObj(const std::string& filename,
                             std::vector<TriangleMesh>* meshes,
                             std::vector<MaterialParameter>* material_params) {
  tinyobj::attrib_t attrib;
  std::vector<tinyobj::shape_t> shapes;
  std::vector<tinyobj::material_t> materials;
  std::string warn;
  std::string err;

  const fs::path filepath = filename;

  const std::string base_dir = filepath.parent_path();

  // TODO logger
  std::cerr << "base dir : " << base_dir << std::endl;

  const char* base_path =
      (base_dir.compare("/") == 0) ? nullptr : base_dir.c_str();

  bool ret =
      tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err,
                       filename.c_str(), base_path, /*trianglulate*/ true);

  if (!warn.empty()) {
    std::cerr << "warning : " << warn << std::endl;
  }

  if (!ret || !err.empty()) {
    std::cerr << "error";
    if (!err.empty()) {
      std::cerr << " : " << err;
    }
    std::cerr << std::endl;
  }

  std::shared_ptr<Attribute> out_attrib(new Attribute());

  if (ret) {
    assert(attrib.vertices.size() % 3 == 0);
    const size_t num_vertices = attrib.vertices.size() / 3;
    out_attrib->vertices.resize(num_vertices * 4);
    for (size_t i = 0; i < num_vertices; ++i) {
      out_attrib->vertices[i * 4 + 0] = attrib.vertices[i * 3 + 0];
      out_attrib->vertices[i * 4 + 1] = attrib.vertices[i * 3 + 1];
      out_attrib->vertices[i * 4 + 2] = attrib.vertices[i * 3 + 2];
      out_attrib->vertices[i * 4 + 3] = 1.0f;
    }

    assert(attrib.normals.size() % 3 == 0);
    const size_t num_normals = attrib.normals.size() / 3;
    out_attrib->normals.resize(num_normals * 4);
    for (size_t i = 0; i < num_normals; ++i) {
      out_attrib->normals[i * 4 + 0] = attrib.normals[i * 3 + 0];
      out_attrib->normals[i * 4 + 1] = attrib.normals[i * 3 + 1];
      out_attrib->normals[i * 4 + 2] = attrib.normals[i * 3 + 2];
      out_attrib->normals[i * 4 + 3] = 1.0f;
    }

    assert(attrib.texcoords.size() % 2 == 0);
    out_attrib->texcoords = attrib.texcoords;

    meshes->clear();
    for (const auto& shape : shapes) {
      const std::string& triangle_mesh_name = shape.name;

      std::vector<uint32_t> face;
      std::vector<uint32_t> normal_index;
      std::vector<uint32_t> texcoords;

      face.reserve(shape.mesh.indices.size());
      normal_index.reserve(shape.mesh.indices.size());
      texcoords.reserve(shape.mesh.indices.size());

      for (const auto& indices : shape.mesh.indices) {
        face.emplace_back(indices.vertex_index);
        normal_index.emplace_back(indices.normal_index);
        texcoords.emplace_back(indices.texcoord_index);
      }

      assert(face.size() % 3 == 0);

      std::vector<uint32_t> material_ids;
      material_ids.reserve(shape.mesh.material_ids.size());
      for (const auto& material_id_ : shape.mesh.material_ids) {
        material_ids.emplace_back(material_id_);
      }

      meshes->emplace_back(triangle_mesh_name, out_attrib, face, normal_index,
                           texcoords, material_ids);
    }
    std::vector<MaterialParameter> material_params;
    for (const auto& material : materials) {
      material_params.emplace_back(ParseTinyObjMaterial(material));
    }
  }

  return ret;
}

}  // namespace io
}  // namespace pbrlab
