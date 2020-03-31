#include "io/curve-mesh-io.h"

#include <iostream>

#include "curve-util.h"
#include "io/cyhair.h"

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

#ifdef __clang__
#pragma clang diagnostic pop
#endif

namespace pbrlab {
namespace io {

bool LoadCurveMeshAsCubicBezierCurve(
    const std::string& filepath, const bool memory_saving_mode,
    std::vector<float>* vertices_thickness /*4(xyz+thickness) * num_vertices*/,
    std::vector<uint32_t>* indices) {
  const fs::path filepath_ = filepath;
  const std::string ext    = filepath_.extension();

  if (ext == ".hair") {
    std::vector<std::vector<float>> vertices_;
    std::vector<std::vector<float>> thicknesses_;
    LoadCyHair(filepath, true, &vertices_, &thicknesses_);

    const size_t num_strands = vertices_.size();
    if (thicknesses_.size() != num_strands) {
      return false;
    }

    uint32_t indices_offset = 0;
    for (size_t strand_id = 0; strand_id < num_strands; ++strand_id) {
      const auto& vertices    = vertices_[strand_id];
      const auto& thicknesses = thicknesses_[strand_id];

      std::vector<float> bezier_vertices;
      std::vector<float> bezier_thicknesses;

      bool ret = ToCubicBezierCurve(vertices, thicknesses, &bezier_vertices,
                                    &bezier_thicknesses);
      const size_t num_vertices = bezier_thicknesses.size();
      if (!ret || bezier_vertices.size() != num_vertices * 3 ||
          num_vertices % 4 != 0) {
        return false;
      }

      if (memory_saving_mode) {
        const size_t num_segments = num_vertices / 4;

        for (size_t seg_id = 0; seg_id < num_segments; ++seg_id) {
          indices->emplace_back(indices_offset + seg_id * 3);

          for (size_t c_id = 0; c_id < 3; ++c_id) {
            const size_t v_id = seg_id * 4 + c_id;
            vertices_thickness->emplace_back(bezier_vertices[3 * v_id + 0]);
            vertices_thickness->emplace_back(bezier_vertices[3 * v_id + 1]);
            vertices_thickness->emplace_back(bezier_vertices[3 * v_id + 2]);
            vertices_thickness->emplace_back(bezier_thicknesses[v_id]);
          }

          if (seg_id > 0) {
            const bool x_ok = fabsf(bezier_vertices[3 * seg_id * 4 + 0] -
                                    bezier_vertices[3 * seg_id * 4 + 0 - 3]) <
                              std::numeric_limits<float>::epsilon();
            const bool y_ok = fabsf(bezier_vertices[3 * seg_id * 4 + 1] -
                                    bezier_vertices[3 * seg_id * 4 + 1 - 3]) <
                              std::numeric_limits<float>::epsilon();
            const bool z_ok = fabsf(bezier_vertices[3 * seg_id * 4 + 2] -
                                    bezier_vertices[3 * seg_id * 4 + 2 - 3]) <
                              std::numeric_limits<float>::epsilon();
            if (!(x_ok && y_ok && z_ok)) {
              return false;
            }
          }
        }

        vertices_thickness->emplace_back(
            bezier_vertices[3 * (num_vertices - 1) + 0]);
        vertices_thickness->emplace_back(
            bezier_vertices[3 * (num_vertices - 1) + 1]);
        vertices_thickness->emplace_back(
            bezier_vertices[3 * (num_vertices - 1) + 2]);
        vertices_thickness->emplace_back(bezier_thicknesses[num_vertices - 1]);
      } else {
        for (size_t v_id = 0; v_id < num_vertices; ++v_id) {
          if (v_id % 4 == 0) {
            indices->emplace_back(indices_offset + v_id);
          }
          vertices_thickness->emplace_back(bezier_vertices[3 * v_id + 0]);
          vertices_thickness->emplace_back(bezier_vertices[3 * v_id + 1]);
          vertices_thickness->emplace_back(bezier_vertices[3 * v_id + 2]);
          vertices_thickness->emplace_back(bezier_thicknesses[v_id]);
        }
      }
      indices_offset += vertices_thickness->size() / 4;
    }

    return true;
  } else {
    std::cerr << "unknown data type" << std::endl;
    return false;
  }
}

bool LoadCurveMeshAsCubicBezierCurve(const std::string& filepath,
                                     const bool memory_saving_mode,
                                     CubicBezierCurveMesh* curve_mesh) {
  std::shared_ptr<CurveAttribute> curve_attribute(new CurveAttribute());
  std::vector<uint32_t> indices;

  LoadCurveMeshAsCubicBezierCurve(filepath, memory_saving_mode,
                                  &(curve_attribute->vertices), &indices);

  const std::vector<uint32_t> material_ids(indices.size(), uint32_t(-1));

  *curve_mesh = CubicBezierCurveMesh(filepath /*TODO: name*/, curve_attribute,
                                     indices, material_ids);

  return curve_mesh != nullptr;
}

}  // namespace io
}  // namespace pbrlab
