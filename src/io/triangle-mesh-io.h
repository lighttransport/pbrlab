#ifndef PBRLAB_TRIANGLE_MESH_IO_H_
#define PBRLAB_TRIANGLE_MESH_IO_H_

#include <string>
#include <vector>

#include "material-param.h"
#include "mesh/triangle-mesh.h"

namespace pbrlab {
namespace io {

bool LoadTriangleMeshFromObj(const std::string& filename,
                             std::vector<TriangleMesh>* meshes,
                             std::vector<MaterialParameter>* material_params);
}  // namespace io
}  // namespace pbrlab
#endif  // PBRLAB_TRIANGLE_MESH_IO_H
