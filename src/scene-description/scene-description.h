#ifndef PBRLAB_SCENE_DESCRIPTION_H_
#define PBRLAB_SCENE_DESCRIPTION_H_

#include <array>
#include <vector>

#include "scene.h"

namespace pbrlab {
namespace scene_description {

using vec3 = std::array<float, 3>;

struct WavefrontObj {
  std::string filepath         = "";
  std::string default_material = "";

  bool create_instances_automatically = false;
};

struct Cyhair {
  std::string filepath         = "";
  std::string name             = "";
  std::string default_material = "";

  bool create_instances_automatically = false;
};

struct Transform {
  enum Type { kTranslate, kScale, kAxisAngle, kLookAt };
  Type type;

  // type = translate
  vec3 translate = {0.f, 0.f, 0.f};

  // type = "scale"
  vec3 scale = {1.f, 1.f, 1.f};

  // type = "axis_angle"
  vec3 axis   = {1.f, 0.f, 0.f};
  float angle = 0.f;  // deg

  // type = "look_at"
  vec3 origin = {0.f, 0.f, 0.f};
  vec3 target = {0.f, 0.f, 1.f};
  vec3 up     = {0.f, 1.f, 0.f};
};

struct CyclesPrincipledBsdf {
  vec3 base_color        = {static_cast<float>(0.8), static_cast<float>(0.8),
                     static_cast<float>(0.8)};
  float subsurface       = static_cast<float>(0.0);
  vec3 subsurface_radius = {static_cast<float>(1.0), static_cast<float>(1.0),
                            static_cast<float>(1.0)};
  vec3 subsurface_color  = {static_cast<float>(0.7), static_cast<float>(0.1),
                           static_cast<float>(0.1)};
  float metallic         = static_cast<float>(0.0);
  float specular         = static_cast<float>(0.5);
  float specular_tint    = static_cast<float>(0.0);
  float roughness        = static_cast<float>(0.5);
  float anisotropic      = static_cast<float>(0.0);
  float anisotropic_rotation   = static_cast<float>(0.0);
  float sheen                  = static_cast<float>(0.0);
  float sheen_tint             = static_cast<float>(0.5);
  float clearcoat              = static_cast<float>(0.0);
  float clearcoat_roughness    = static_cast<float>(0.03);
  float ior                    = static_cast<float>(1.45);
  float transmission           = static_cast<float>(0.0);
  float transmission_roughness = static_cast<float>(0.0);

  std::string base_color_tex_name       = "";
  std::string subsurface_color_tex_name = "";
};

struct HairBsdf {
  enum ColoringHair { kRGB = 0, kMelanin };
  ColoringHair coloring_hair = kMelanin;
  vec3 base_color            = {0.18f, 0.06f, 0.02f};

  float melanin           = 0.5f;
  float melanin_redness   = 0.8f;
  float melanin_randomize = 0.f;

  float roughness           = 0.2f;
  float azimuthal_roughness = 0.3f;

  float ior = 1.55f;

  float shift = 2.f;

  vec3 specular_tint        = {1.f, 1.f, 1.f};
  vec3 second_specular_tint = {1.f, 1.f, 1.f};
  vec3 transmission_tint    = {1.f, 1.f, 1.f};
};

struct Material {
  enum Type { kCyclesPrincipledBsdf, kHairBsdf };
  Type type = kCyclesPrincipledBsdf;

  std::string name = "";

  // type = “cycles_principled_bsdf"
  CyclesPrincipledBsdf cycles_principled_bsdf;

  // type = “hair_bsdf”
  HairBsdf hair_bsdf;
};

struct Light {
  enum Type { kArea, kDirectional, kPoint };
  Type type = kArea;

  std::string name = "";
  vec3 emission    = {1.f, 1.f, 1.f};

  // type = area

  // type = directional or point
  std::vector<Transform> transform;
};

struct Texture {
  std::string name;
  std::string filepath;
};

struct LocalScene {
  std::string name;
  std::vector<std::string> meshes;
};

struct Instance {
  std::string local_scene;
  std::vector<std::string> materials;
  std::vector<std::string> lights;
  std::vector<Transform> transform;
};

struct World {};

struct Setting {};

struct Root {
  std::vector<WavefrontObj> wavefront_objs;
  std::vector<Cyhair> cyhairs;
  std::vector<Material> materials;
  std::vector<Light> lights;
  std::vector<Texture> textures;
  std::vector<LocalScene> local_scenes;
  std::vector<Instance> instances;
  World world;
  Setting setting;
};

}  // namespace scene_description

bool CreateSceneFromSceneDescription(
    const scene_description::Root &scene_description_root, Scene *scene);

}  // namespace pbrlab

#endif  // PBRLAB_SCENE_DESCRIPTION_H_
