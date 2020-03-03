#ifndef PBRLAB_PC_COMMON_H_
#define PBRLAB_PC_COMMON_H_
#include <iostream>
#include <string>
#include "io/triangle-mesh-io.h"
#include "scene.h"

inline bool CreateScene(const std::string& obj_filename, pbrlab::Scene* scene) {
  std::vector<pbrlab::TriangleMesh> triangle_meshes;
  std::vector<pbrlab::MaterialParameter> material_params;
  {
    const bool success = pbrlab::io::LoadTriangleMeshFromObj(
        obj_filename, &triangle_meshes, &material_params);
    if (!success) {
      std::cerr << "Faild loading obj file [" << obj_filename << "]"
                << std::endl;
      return false;
    }
  }
  std::cerr << "Load obj file [" << obj_filename << "]" << std::endl;

  const size_t num_shape = triangle_meshes.size();

  std::cerr << "The Number of shapes is " << num_shape << " in ["
            << obj_filename << "]" << std::endl;

  for (const auto& triangle_mesh : triangle_meshes) {
    std::cerr << "  add shape [" << triangle_mesh.GetName() << "]" << std::endl;
    std::cerr << "    num face : " << triangle_mesh.GetNumFaces() << std::endl;

    const pbrlab::MeshPtr mesh_ptr = scene->AddTriangleMesh(triangle_mesh);

    const uint32_t instance_id = scene->CreateInstance(mesh_ptr);

    if (triangle_mesh.GetName().substr(0, 5) == "light") {
      pbrlab::AreaLightParameter area_light_param = {};
      area_light_param.emission                   = pbrlab::float3(3.0f);
      const uint32_t light_id = scene->AddLightParam(area_light_param);

      std::vector<std::vector<uint32_t>> light_param_ids;
      light_param_ids.emplace_back(
          std::vector<uint32_t>(triangle_mesh.GetNumFaces()));

      for (uint32_t f = 0; f < triangle_mesh.GetNumFaces(); ++f) {
        light_param_ids[0][f] = light_id;
      }

      scene->AttachLightParamIdsToInstance(instance_id, light_param_ids);
    }
  }
  std::cerr << std::endl;

  for (const auto& material_param : material_params) {
    scene->AddMaterialParam(material_param);
  }

  scene->CommitScene();
  return true;
}
#endif  // PBRLAB_PC_COMMON_H_
