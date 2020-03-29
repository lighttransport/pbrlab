#ifndef PBRLAB_CURVE_MESH_IO_H
#define PBRLAB_CURVE_MESH_IO_H

#include <stdint.h>

#include <string>
#include <vector>
namespace pbrlab {
namespace io {

bool LoadCurveMeshAsBezierCurve(
    const std::string& filepath, const bool memory_saving_mode,
    std::vector<float>* vertices_thickness /*4(xyz+thick_ness) * num_vertices*/,
    std::vector<uint32_t>* indices);

}
}  // namespace pbrlab

#endif  // PBRLAB_CURVE_MESH_IO_H
