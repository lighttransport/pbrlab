#include <stdlib.h>

#include <iostream>
#include <vector>

#include "io/image-io.h"
#include "io/triangle-mesh-io.h"
#include "render.h"

#ifdef PBRLAB_USE_STACK_TRACE_LOGGER
#include <glog/logging.h>
#endif

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;
#ifdef PBRLAB_USE_STACK_TRACE_LOGGER
  google::InitGoogleLogging(argv[0]);
  google::InstallFailureSignalHandler();
#endif

  if (argc < 2) {
    std::cerr << "not specified obj filename" << std::endl;
    return EXIT_FAILURE;
  }

  const std::string obj_filename = std::string(argv[1]);

  std::vector<pbrlab::TriangleMesh> triangle_meshes;
  std::vector<pbrlab::MaterialParameter> material_params;
  {
    const bool success = pbrlab::io::LoadTriangleMeshFromObj(
        obj_filename, &triangle_meshes, &material_params);
    if (!success) {
      std::cerr << "Faild loading obj file [" << argv[1] << "]" << std::endl;
      return EXIT_FAILURE;
    }
  }
  std::cerr << "Load obj file [" << argv[1] << "]" << std::endl;

  pbrlab::Scene scene;
  const size_t num_shape = triangle_meshes.size();

  std::cerr << "The Number of shapes is " << num_shape << " in [" << argv[1]
            << "]" << std::endl;

  for (const auto& triangle_mesh : triangle_meshes) {
    std::cerr << "  add shape [" << triangle_mesh.GetName() << "]" << std::endl;
    std::cerr << "    num face : " << triangle_mesh.GetNumFaces() << std::endl;

    const pbrlab::MeshPtr mesh_ptr = scene.AddTriangleMesh(triangle_mesh);

    const uint32_t instance_id = scene.CreateInstance(mesh_ptr);

    if (triangle_mesh.GetName().substr(0, 5) == "light") {
      pbrlab::AreaLightParameter area_light_param = {};
      area_light_param.emission                   = pbrlab::float3(3.0f);
      const uint32_t light_id = scene.AddLightParam(area_light_param);

      std::vector<std::vector<uint32_t>> light_param_ids;
      light_param_ids.emplace_back(
          std::vector<uint32_t>(triangle_mesh.GetNumFaces()));

      for (uint32_t f = 0; f < triangle_mesh.GetNumFaces(); ++f) {
        light_param_ids[0][f] = light_id;
      }

      scene.AttachLightParamIdsToInstance(instance_id, light_param_ids);
    }
  }
  std::cerr << std::endl;

  for (const auto& material_param : material_params) {
    scene.AddMaterialParam(material_param);
  }

  scene.CommitScene();

  const size_t width   = 512;
  const size_t height  = 512;
  const size_t samples = 32;

  pbrlab::RenderLayer layer;
  pbrlab::Render(scene, width, height, samples, &layer);

  std::vector<float> color(width * height * 4);

  for (size_t i = 0; i < layer.count.size(); ++i) {
    color[i * 4 + 0] = layer.rgba[i * 4 + 0] / layer.count[i];
    color[i * 4 + 1] = layer.rgba[i * 4 + 1] / layer.count[i];
    color[i * 4 + 2] = layer.rgba[i * 4 + 2] / layer.count[i];
    color[i * 4 + 3] = layer.rgba[i * 4 + 3] / layer.count[i];
  }

  pbrlab::io::WritePNG("rgba.png", "./", color, width, height, 4);

  return EXIT_SUCCESS;
}
