#include "pc-common.h"

#include "io/curve-mesh-io.h"
#include "io/triangle-mesh-io.h"

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

static bool CreateSceneFromObj(const std::string& obj_filename,
                               pbrlab::Scene* scene) {
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
  return true;
}

static bool CreateSceneFromCubicBezierCurve(const std::string& filepath,
                                            pbrlab::Scene* scene) {
  pbrlab::CubicBezierCurveMesh curve_mesh;

  const bool ret =
      pbrlab::io::LoadCurveMeshAsCubicBezierCurve(filepath, false, &curve_mesh);
  if (!ret) {
    return false;
  }

  std::cerr << "Load curve file [" << filepath << "]" << std::endl;

  {
    std::cerr << "  add shape [" << curve_mesh.GetName() << "]" << std::endl;
    std::cerr << "  num segments : " << curve_mesh.GetNumSegments()
              << std::endl;

    const uint32_t material_id =
        scene->AddMaterialParam(pbrlab::HairBsdfParameter());

    const uint32_t num_segments = curve_mesh.GetNumSegments();

    for (uint32_t seg_id = 0; seg_id < num_segments; ++seg_id) {
      curve_mesh.SetMaterialId(material_id, seg_id);
    }

    const pbrlab::MeshPtr mesh_ptr = scene->AddCubicBezierCurveMesh(curve_mesh);

    const uint32_t instance_id = scene->CreateInstance(mesh_ptr);
    (void)instance_id;
  }

  return true;
}

bool CreateScene(int argc, char** argv, pbrlab::Scene* scene) {
  if (argc < 2) {
    return false;
  }

  for (size_t i = 1; i < size_t(argc); ++i) {
    const std::string filepath(argv[i]);

    const std::string file_extension = fs::path(filepath).extension();

    if (file_extension == ".obj") {
      const bool ret = CreateSceneFromObj(filepath, scene);
      if (!ret) {
        return false;
      }
    } else if (file_extension == ".hair") {
      const bool ret = CreateSceneFromCubicBezierCurve(filepath, scene);
      if (!ret) {
        return false;
      }
    }
  }

  scene->CommitScene();

  float bmin[3], bmax[3];
  scene->FetchSceneAABB(bmin, bmax);
  printf("bmin: %f %f %f\n  bmax: %f %f %f\n", double(bmin[0]), double(bmin[1]),
         double(bmin[2]), double(bmax[0]), double(bmax[1]), double(bmax[2]));

  return true;
}
