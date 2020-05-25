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

EditQueue::EditQueue(void) = default;
EditQueue::~EditQueue(void) {
  std::lock_guard<std::mutex> lock(mtx_);

  for (auto& v : queue_) {
    void* dst                 = v.first;
    void* src                 = v.second.first;
    const FileType& file_type = v.second.second;
    if (dst != nullptr && src != nullptr) {
      if (file_type == kFloat) {
        float* src_ = reinterpret_cast<float*>(src);
        delete src_;
      } else {
        assert(false);
      }
    }
  }

  queue_.clear();
}

template <typename T>
void CopyAndDeleteSrc(T* src, T* dst) {
  (*dst) = (*src);
  if (src != nullptr) {
    delete src;
    src = nullptr;
  }
}

void EditQueue::EditAndPopAll(void) {
  std::lock_guard<std::mutex> lock(mtx_);

  for (auto& v : queue_) {
    void* dst                 = v.first;
    void* src                 = v.second.first;
    const FileType& file_type = v.second.second;
    if (dst != nullptr && src != nullptr) {
      if (file_type == kFloat) {
        float* dst_ = reinterpret_cast<float*>(dst);
        float* src_ = reinterpret_cast<float*>(src);

        CopyAndDeleteSrc(src_, dst_);
      } else if (file_type == kPbrlabMaterialParameter) {
        pbrlab::MaterialParameter* dst_ =
            reinterpret_cast<pbrlab::MaterialParameter*>(dst);
        pbrlab::MaterialParameter* src_ =
            reinterpret_cast<pbrlab::MaterialParameter*>(src);

        CopyAndDeleteSrc(src_, dst_);
      } else {
        assert(false);
      }
    }
  }

  queue_.clear();
}

RenderItem::RenderItem(void)
    : cancel_render_flag(false),
      finish_frag(false),
      last_render_pass(0),
      finish_pass(0),
      max_pass(512) {}

static void FixTextureId(const std::vector<uint32_t>& texture_ids,
                         uint32_t* fixed_texture_id) {
  if (*fixed_texture_id != uint32_t(-1)) {
    *fixed_texture_id = texture_ids.at(*fixed_texture_id);
  }
}

static bool CreateSceneFromObj(const std::string& obj_filename,
                               pbrlab::Scene* scene) {
  std::vector<pbrlab::TriangleMesh> triangle_meshes;
  std::vector<pbrlab::MaterialParameter> material_params;
  std::vector<pbrlab::Texture> textures;
  {
    const bool success = pbrlab::io::LoadTriangleMeshFromObj(
        obj_filename, &triangle_meshes, &material_params, &textures);
    if (!success) {
      std::cerr << "Faild loading obj file [" << obj_filename << "]"
                << std::endl;
      return false;
    }
  }
  std::cerr << "Load obj file [" << obj_filename << "]" << std::endl;

  std::vector<uint32_t> material_ids;
  for (const auto& material_param : material_params) {
    const uint32_t material_id = scene->AddMaterialParam(material_param);
    material_ids.emplace_back(material_id);
  }

  std::vector<uint32_t> texture_ids;
  for (const auto& texture : textures) {
    const uint32_t tex_id = scene->AddTexture(texture);
    texture_ids.emplace_back(tex_id);
  }

  // Fix tex id
  for (auto& material_param : material_params) {
    if (material_param.index() == pbrlab::kCyclesPrincipledBsdfParameter) {
      pbrlab::CyclesPrincipledBsdfParameter& cycles_material_param =
          mpark::get<pbrlab::kCyclesPrincipledBsdfParameter>(material_param);

      FixTextureId(texture_ids, &(cycles_material_param.base_color_tex_id));
      FixTextureId(texture_ids,
                   &(cycles_material_param.subsurface_color_tex_id));
    }
  }

  const size_t num_shape = triangle_meshes.size();

  std::cerr << "The Number of shapes is " << num_shape << " in ["
            << obj_filename << "]" << std::endl;

  for (auto& triangle_mesh : triangle_meshes) {
    std::cerr << "  add shape [" << triangle_mesh.GetName() << "]" << std::endl;
    std::cerr << "    num face : " << triangle_mesh.GetNumFaces() << std::endl;

    const uint32_t num_face = triangle_mesh.GetNumFaces();

    const std::vector<uint32_t>& no_fix_material_ids =
        triangle_mesh.GetMaterials();
    assert(no_fix_material_ids.size() == num_face);
    for (uint32_t f_id = 0; f_id < num_face; ++f_id) {
      triangle_mesh.SetMaterialId(material_ids.at(no_fix_material_ids.at(f_id)),
                                  f_id);
    }

    const pbrlab::MeshPtr mesh_ptr = scene->AddTriangleMesh(triangle_mesh);

    const uint32_t local_scene_id = scene->CreateLocalScene();
    scene->AddMeshToLocalScene(local_scene_id, mesh_ptr);
    // Transform TODO
    const float transform[4][4] = {{1.0f, 0.0f, 0.0f, 0.0f},
                                   {0.0f, 1.0f, 0.0f, 0.0f},
                                   {0.0f, 0.0f, 1.0f, 0.0f},
                                   {0.0f, 0.0f, 0.0f, 1.0f}};

    const uint32_t instance_id =
        scene->CreateInstance(local_scene_id, transform);

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

    const uint32_t local_scene_id = scene->CreateLocalScene();
    scene->AddMeshToLocalScene(local_scene_id, mesh_ptr);
    // Transform TODO
    const float transform[4][4] = {{1.0f, 0.0f, 0.0f, 0.0f},
                                   {0.0f, 1.0f, 0.0f, 0.0f},
                                   {0.0f, 0.0f, 1.0f, 0.0f},
                                   {0.0f, 0.0f, 0.0f, 1.0f}};

    const uint32_t instance_id =
        scene->CreateInstance(local_scene_id, transform);

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
