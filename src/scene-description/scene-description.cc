#include "scene-description/scene-description.h"

#include <iostream>
#include <unordered_map>
#include <unordered_set>

#include "image-utils.h"
#include "io/curve-mesh-io.h"
#include "io/image-io.h"
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

namespace pbrlab {

static void FixTextureId(const std::vector<uint32_t>& texture_ids,
                         uint32_t* fixed_texture_id) {
  if (*fixed_texture_id != uint32_t(-1)) {
    *fixed_texture_id = texture_ids.at(*fixed_texture_id);
  }
}

static bool CreateSceneFromObj(
    const scene_description::WavefrontObj& wavefront_obj,
    std::unordered_map<std::string, MeshPtr>* meshes_name_to_mesh_ptr,
    std::unordered_map<std::string, uint32_t>* materials_name_to_id,
    std::unordered_map<std::string, uint32_t>* textures_name_to_id,
    pbrlab::Scene* scene) {
  std::vector<pbrlab::TriangleMesh> triangle_meshes;
  std::vector<pbrlab::MaterialParameter> material_params;
  std::vector<pbrlab::Texture> textures;
  {
    const bool success = pbrlab::io::LoadTriangleMeshFromObj(
        wavefront_obj.filepath, &triangle_meshes, &material_params, &textures);
    if (!success) {
      std::cerr << "Faild loading obj file [" << wavefront_obj.filepath << "]"
                << std::endl;
      return false;
    }
  }
  std::cerr << "Load obj file [" << wavefront_obj.filepath << "]" << std::endl;

  std::vector<uint32_t> material_ids;
  {
    std::unordered_set<std::string> used_name;
    for (auto& material_param : material_params) {
      if (used_name.count(GetMaterialName(material_param)) > 0) {
        const std::string tmp = GetMaterialName(material_param);
        SetMaterialName(tmp, &material_param);
      }
      const uint32_t material_id = scene->AddMaterialParam(material_param);
      material_ids.emplace_back(material_id);
    }
  }

  std::vector<uint32_t> texture_ids;
  {
    std::unordered_set<std::string> used_name;
    for (auto& texture : textures) {
      if (used_name.count(texture.GetName()) > 0) {
        const std::string tmp = texture.GetName() + "_";
        texture.SetName(tmp);
      }
      const uint32_t tex_id = scene->AddTexture(texture);
      texture_ids.emplace_back(tex_id);
    }
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
            << wavefront_obj.filepath << "]" << std::endl;

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

    if (meshes_name_to_mesh_ptr->count(triangle_mesh.GetName()) == 0) {
      meshes_name_to_mesh_ptr->at(triangle_mesh.GetName()) = mesh_ptr;
    } else {
      std::cerr << "warning: conflict triangle mesh name." << std::endl;
    }

    if (wavefront_obj.create_instances_automatically) {
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
  }
  std::cerr << std::endl;

  for (size_t cnt = 0; cnt < material_params.size(); ++cnt) {
    const auto& material_param = material_params[cnt];
    const uint32_t material_id = material_ids[cnt];

    const std::string material_name = GetMaterialName(material_param);

    if (materials_name_to_id->count(material_name) > 0) {
      std::cerr << "warning: conflict material name." << std::endl;
    }
    (*materials_name_to_id)[material_name] = material_id;
  }

  for (size_t cnt = 0; cnt < textures.size(); ++cnt) {
    const auto& texture   = textures[cnt];
    const uint32_t tex_id = texture_ids[cnt];

    const std::string tex_name = texture.GetName();

    if (textures_name_to_id->count(tex_name) > 0) {
      std::cerr << "warning: conflict tex name." << std::endl;
    }
    (*textures_name_to_id)[tex_name] = tex_id;
  }

  return true;
}

static bool CreateSceneFromCubicBezierCurve(
    const scene_description::Cyhair& cyhair,
    std::unordered_map<std::string, MeshPtr>* meshes_name_to_mesh_ptr,
    std::unordered_map<std::string, uint32_t>* materials_name_to_id,
    pbrlab::Scene* scene) {
  pbrlab::CubicBezierCurveMesh curve_mesh;

  const bool ret =
      io::LoadCurveMeshAsCubicBezierCurve(cyhair.filepath, false, &curve_mesh);
  if (!ret) {
    return false;
  }

  std::cerr << "Load curve file [" << cyhair.filepath << "]" << std::endl;

  {
    std::cerr << "  add shape [" << curve_mesh.GetName() << "]" << std::endl;
    std::cerr << "  num segments : " << curve_mesh.GetNumSegments()
              << std::endl;

    MaterialParameter material_param = pbrlab::HairBsdfParameter();

    SetMaterialName(curve_mesh.GetName() + "-mtl", &material_param);

    const uint32_t material_id = scene->AddMaterialParam(material_param);

    const uint32_t num_segments = curve_mesh.GetNumSegments();

    for (uint32_t seg_id = 0; seg_id < num_segments; ++seg_id) {
      curve_mesh.SetMaterialId(material_id, seg_id);
    }

    const pbrlab::MeshPtr mesh_ptr = scene->AddCubicBezierCurveMesh(curve_mesh);

    if (meshes_name_to_mesh_ptr->count(curve_mesh.GetName()) == 0) {
      meshes_name_to_mesh_ptr->at(curve_mesh.GetName()) = mesh_ptr;
    } else {
      std::cerr << "warning: conflict curve mesh name." << std::endl;
    }

    if (cyhair.create_instances_automatically) {
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

    {
      const std::string material_name = GetMaterialName(material_param);

      if (materials_name_to_id->count(material_name) > 0) {
        std::cerr << "warning: conflict material name." << std::endl;
      }
      (*materials_name_to_id)[material_name] = material_id;
    }
  }

  return true;
}

static bool IsHdr(const std::string& filepath) {
  const std::string file_extension = fs::path(filepath).extension();

  std::string tmp = file_extension;
  std::transform(file_extension.begin(), file_extension.end(), tmp.begin(),
                 tolower);

  return (tmp == ".exr" || tmp == ".hdr");
}

static bool LoadTexture(const std::string& filepath,
                        const std::string& base_dir, const std::string& name,
                        const bool degamma, Texture* texture) {
  std::vector<float> pixels;
  size_t width, height, channels;
  const bool success =
      io::LoadImageFromFile(filepath, base_dir, &pixels, &width, &height, &channels);

  if (!success) {
    return false;
  }

  if (degamma) {
    std::vector<float> tmp;
    SrgbToLiner(pixels, width, height, channels, &tmp);
    *texture = Texture(tmp, uint32_t(width), uint32_t(height),
                       uint32_t(channels), name);
  } else {
    *texture = Texture(pixels, uint32_t(width), uint32_t(height),
                       uint32_t(channels), name);
  }
  return true;
}

static bool LoadTexture(
    const scene_description::Texture& texture,
    std::unordered_map<std::string, uint32_t>* textures_name_to_id,
    Scene* scene) {
  const bool degamma = IsHdr(texture.filepath);

  Texture tex;
  const bool ret =
      LoadTexture(texture.filepath, "./", texture.name, degamma, &tex);

  if (!ret) {
    return false;
  }

  const uint32_t tex_id = scene->AddTexture(tex);

  const std::string tex_name = tex.GetName();

  if (textures_name_to_id->count(tex_name) > 0) {
    std::cerr << "warning: conflict tex name." << std::endl;
  }
  (*textures_name_to_id)[tex_name] = tex_id;

  return true;
}

static bool AddMaterialParam(
    const scene_description::Material& material,
    std::unordered_map<std::string, uint32_t>* materials_name_to_id,
    std::unordered_map<std::string, uint32_t>* textures_name_to_id,
    Scene* scene) {
  MaterialParameter mp = {};
  if (material.type == scene_description::Material::kCyclesPrincipledBsdf) {
    const auto& in = material.cycles_principled_bsdf;

    CyclesPrincipledBsdfParameter m_param = {};

    m_param.base_color = float3(in.base_color.data());

    m_param.subsurface        = in.subsurface;
    m_param.subsurface_radius = float3(in.subsurface_radius.data());
    m_param.subsurface_color  = float3(in.subsurface_color.data());

    m_param.metallic             = in.metallic;
    m_param.specular             = in.specular;
    m_param.specular_tint        = in.specular_tint;
    m_param.roughness            = in.roughness;
    m_param.anisotropic          = in.anisotropic;
    m_param.anisotropic_rotation = in.anisotropic_rotation;
    m_param.sheen                = in.sheen;
    m_param.sheen_tint           = in.sheen_tint;

    m_param.clearcoat              = in.clearcoat;
    m_param.clearcoat_roughness    = in.clearcoat_roughness;
    m_param.ior                    = in.ior;
    m_param.transmission           = in.transmission;
    m_param.transmission_roughness = in.transmission_roughness;

    m_param.base_color_tex_id       = uint32_t(-1);
    m_param.subsurface_color_tex_id = uint32_t(-1);

    if (!in.base_color_tex_name.empty() &&
        textures_name_to_id->count(in.base_color_tex_name) > 0) {
      m_param.base_color_tex_id =
          (*textures_name_to_id)[in.base_color_tex_name];
    }

    if (!in.subsurface_color_tex_name.empty() &&
        textures_name_to_id->count(in.subsurface_color_tex_name) > 0) {
      m_param.subsurface_color_tex_id =
          (*textures_name_to_id)[in.subsurface_color_tex_name];
    }

    mp = m_param;
  } else if (material.type == scene_description::Material::kHairBsdf) {
    const auto& in = material.hair_bsdf;

    HairBsdfParameter m_param = {};

    m_param.coloring_hair = HairBsdfParameter::ColoringHair(in.coloring_hair);

    m_param.base_color = float3(in.base_color.data());

    m_param.melanin           = in.melanin;
    m_param.melanin_redness   = in.melanin_redness;
    m_param.melanin_randomize = in.melanin_randomize;

    m_param.roughness           = in.roughness;
    m_param.azimuthal_roughness = in.azimuthal_roughness;

    m_param.ior   = in.ior;
    m_param.shift = in.shift;

    m_param.specular_tint = float3(in.specular_tint.data());

    m_param.second_specular_tint = float3(in.second_specular_tint.data());

    m_param.transmission_tint = float3(in.transmission_tint.data());

    mp = m_param;
  } else {
    assert(false);
    return false;
  }

  SetMaterialName(material.name, &mp);

  const uint32_t material_id = scene->AddMaterialParam(mp);

  const std::string material_name = GetMaterialName(mp);

  if (materials_name_to_id->count(material_name) > 0) {
    std::cerr << "warning: conflict material name." << std::endl;
  }
  (*materials_name_to_id)[material_name] = material_id;

  return true;
}

static bool AddLight(
    const scene_description::Light& light,
    std::unordered_map<std::string, uint32_t>* light_name_to_id, Scene* scene) {
  pbrlab::LightParameter light_param = {};

  if (light.type == scene_description::Light::kArea) {
    pbrlab::AreaLightParameter area_light_param = {};

    area_light_param.name     = light.name;
    area_light_param.emission = float3(light.emission.data());

    light_param = area_light_param;
  }

  const uint32_t light_id = scene->AddLightParam(light_param);

  const std::string light_name = GetLightName(light_param);

  if (light_name_to_id->count(light_name) > 0) {
    std::cerr << "warning: conflict light name." << std::endl;
  }
  (*light_name_to_id)[light_name] = light_id;

  return true;
}

static bool CreateLocalScene(
    const scene_description::LocalScene& local_scene,
    std::unordered_map<std::string, MeshPtr>* meshes_name_to_mesh_ptr,
    std::unordered_map<std::string, uint32_t /*local scene id*/>*
        local_scene_name_to_id,
    Scene* scene) {
  const uint32_t local_scene_id = scene->CreateLocalScene();

  if (local_scene_name_to_id->count(local_scene.name) > 0) {
    std::cerr << "warning: conflict local scene." << std::endl;
  }

  for (const std::string& mesh_name : local_scene.meshes) {
    if (meshes_name_to_mesh_ptr->count(mesh_name) == 0) {
      std::cerr << "error: not found mesh [" << mesh_name << "]." << std::endl;
      return false;
    }
    scene->AddMeshToLocalScene(local_scene_id,
                               (*meshes_name_to_mesh_ptr)[mesh_name]);
  }

  (*local_scene_name_to_id)[local_scene.name] = local_scene_id;
  return true;
}

static bool CreateInstance(
    const scene_description::Instance& instance,
    std::unordered_map<std::string, uint32_t>* local_scene_name_to_id,
    std::unordered_map<std::string, uint32_t>* materials_name_to_id,
    std::unordered_map<std::string, uint32_t>* lights_name_to_id,
    Scene* scene) {
  if (local_scene_name_to_id->count(instance.local_scene) == 0) {
    std::cerr << "error: local_scene name not found" << std::endl;
    return false;
  }

  const uint32_t local_scene_id =
      (*local_scene_name_to_id)[instance.local_scene];

  // Transform TODO
  const float transform[4][4] = {{1.0f, 0.0f, 0.0f, 0.0f},
                                 {0.0f, 1.0f, 0.0f, 0.0f},
                                 {0.0f, 0.0f, 1.0f, 0.0f},
                                 {0.0f, 0.0f, 0.0f, 1.0f}};

  const uint32_t instance_id = scene->CreateInstance(local_scene_id, transform);

  const ::pbrlab::MeshInstance& instance_ = scene->GetMeshInstance(instance_id);

  const ::pbrlab::LocalScene* local_scene = instance_.local_scene.get();

  const uint32_t num_meshes = uint32_t(local_scene->meshes.size());

  std::vector<std::vector<uint32_t>> material_ids = instance_.material_ids;
  assert(material_ids.size() == num_meshes);
  for (uint32_t mesh_id = 0; mesh_id < num_meshes; ++mesh_id) {
    if (mesh_id >= instance.materials.size()) {
      continue;
    }
    if (instance.materials[mesh_id] == "") {
      continue;
    }

    if (materials_name_to_id->count(instance.materials[mesh_id]) == 0) {
      std::cerr << "error: not found material name" << std::endl;
      return false;
    }

    const uint32_t material_id =
        (*materials_name_to_id)[instance.materials[mesh_id]];

    const MeshPtr& mesh_ptr = (*local_scene).meshes[mesh_id];

    const uint32_t num_primitive = GetNumPrimitive(mesh_ptr);

    assert(num_primitive == instance_.material_ids[mesh_id].size());

    material_ids[mesh_id] = std::vector<uint32_t>(num_primitive, material_id);
  }
  scene->AttachMaterialParamIdsToInstance(instance_id, material_ids);

  std::vector<std::vector<uint32_t>> light_param_ids(num_meshes);
  for (uint32_t mesh_id = 0; mesh_id < num_meshes; ++mesh_id) {
    if (mesh_id >= instance.lights.size()) {
      continue;
    }
    if (instance.lights[mesh_id] == "") {
      continue;
    }

    if (lights_name_to_id->count(instance.lights[mesh_id]) == 0) {
      std::cerr << "error: not found light name" << std::endl;
      return false;
    }

    const uint32_t light_id = (*lights_name_to_id)[instance.lights[mesh_id]];

    const MeshPtr& mesh_ptr = (*local_scene).meshes[mesh_id];

    const uint32_t num_primitive = GetNumPrimitive(mesh_ptr);

    light_param_ids[mesh_id] = std::vector<uint32_t>(num_primitive, light_id);
  }

  scene->AttachLightParamIdsToInstance(instance_id, light_param_ids);

  return true;
}

bool CreateSceneFromSceneDescription(
    const scene_description::Root& scene_description_root, Scene* scene) {
  std::unordered_map<std::string, MeshPtr> meshes_name_to_mesh_ptr;
  std::unordered_map<std::string, uint32_t> materials_name_to_id;
  std::unordered_map<std::string, uint32_t> textures_name_to_id;
  std::unordered_map<std::string, uint32_t> lights_name_to_id;
  std::unordered_map<std::string, uint32_t> local_scene_name_to_id;

  for (const auto& wavefront_obj : scene_description_root.wavefront_objs) {
    if (!CreateSceneFromObj(wavefront_obj, &meshes_name_to_mesh_ptr,
                            &materials_name_to_id, &textures_name_to_id,
                            scene)) {
      return false;
    }
  }

  for (const auto& cyhair : scene_description_root.cyhairs) {
    if (!CreateSceneFromCubicBezierCurve(cyhair, &meshes_name_to_mesh_ptr,
                                         &materials_name_to_id, scene)) {
      return false;
    }
  }

  for (const auto& texture : scene_description_root.textures) {
    if (!LoadTexture(texture, &textures_name_to_id, scene)) {
      return false;
    }
  }

  for (const auto& material : scene_description_root.materials) {
    if (!AddMaterialParam(material, &materials_name_to_id, &textures_name_to_id,
                          scene)) {
      return false;
    }
  }

  for (const auto& light : scene_description_root.lights) {
    if (!AddLight(light, &lights_name_to_id, scene)) {
      return false;
    }
  }

  for (const auto& local_scene : scene_description_root.local_scenes) {
    if (!CreateLocalScene(local_scene, &meshes_name_to_mesh_ptr,
                          &local_scene_name_to_id, scene)) {
      return false;
    }
  }

  for (const auto& instance : scene_description_root.instances) {
    if (!CreateInstance(instance, &local_scene_name_to_id,
                        &materials_name_to_id, &lights_name_to_id, scene)) {
      return false;
    }
  }

  return true;
}

}  // namespace pbrlab
