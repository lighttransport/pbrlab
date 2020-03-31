#ifndef PBRLAB_CURVE_MESH_IO_H
#define PBRLAB_CURVE_MESH_IO_H

#include <stdint.h>

#include <mesh/cubic-bezier-curve-mesh.h>
#include <string>
#include <vector>
namespace pbrlab {
namespace io {

bool LoadCurveMeshAsCubicBezierCurve(
    const std::string& filepath, const bool memory_saving_mode,
    std::vector<float>* vertices_thickness /*4(xyz+thick_ness) * num_vertices*/,
    std::vector<uint32_t>* indices);

bool LoadCurveMeshAsCubicBezierCurve(const std::string& filepath,
                                     const bool memory_saving_mode,
                                     CubicBezierCurveMesh* curve_mesh);
}  // namespace io
}  // namespace pbrlab

#endif  // PBRLAB_CURVE_MESH_IO_H
