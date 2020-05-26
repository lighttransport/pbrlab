#include "io/scene-description-io.h"

#include <iostream>

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#endif

#include "staticjson/staticjson.hpp"

STATICJSON_DECLARE_ENUM(
    pbrlab::scene_description::Transform::Type,
    {"translate", pbrlab::scene_description::Transform::kTranslate},
    {"scale", pbrlab::scene_description::Transform::kScale},
    {"axis_angle", pbrlab::scene_description::Transform::kAxisAngle},
    {"look_at", pbrlab::scene_description::Transform::kLookAt})

STATICJSON_DECLARE_ENUM(
    pbrlab::scene_description::Material::Type,
    {"cycles_principled_bsdf",
     pbrlab::scene_description::Material::kCyclesPrincipledBsdf},
    {"hair_bsdf", pbrlab::scene_description::Material::kHairBsdf})

STATICJSON_DECLARE_ENUM(pbrlab::scene_description::HairBsdf::ColoringHair,
                        {"rgb", pbrlab::scene_description::HairBsdf::kRGB},
                        {"melanin",
                         pbrlab::scene_description::HairBsdf::kMelanin})

STATICJSON_DECLARE_ENUM(pbrlab::scene_description::Light::Type,
                        {"area", pbrlab::scene_description::Light::kArea},
                        {"directional",
                         pbrlab::scene_description::Light::kDirectional},
                        {"point", pbrlab::scene_description::Light::kPoint})

#ifdef __clang__
#pragma clang diagnostic pop
#endif

namespace staticjson {

template <>
void init(pbrlab::scene_description::WavefrontObj* wavefront_obj,
          ObjectHandler* h) {
  h->add_property("filepath", &(wavefront_obj->filepath));

  h->add_property("default_material", &(wavefront_obj->default_material),
                  staticjson::Flags::Optional);

  h->add_property("create_instances_automatically",
                  &(wavefront_obj->create_instances_automatically),
                  staticjson::Flags::Optional);
}

template <>
void init(pbrlab::scene_description::Cyhair* cyhair, ObjectHandler* h) {
  h->add_property("filepath", &(cyhair->filepath));

  h->add_property("name", &(cyhair->name));

  h->add_property("default_material", &(cyhair->default_material),
                  staticjson::Flags::Optional);

  h->add_property("create_instances_automatically",
                  &(cyhair->create_instances_automatically),
                  staticjson::Flags::Optional);
}

template <>
void init(pbrlab::scene_description::Transform* transform, ObjectHandler* h) {
  h->add_property("type", &(transform->type));

  h->add_property("translate", &(transform->translate),
                  staticjson::Flags::Optional);

  h->add_property("scale", &(transform->scale), staticjson::Flags::Optional);

  h->add_property("axis", &(transform->axis), staticjson::Flags::Optional);

  h->add_property("angle", &(transform->angle), staticjson::Flags::Optional);

  h->add_property("origin", &(transform->origin), staticjson::Flags::Optional);

  h->add_property("target", &(transform->target), staticjson::Flags::Optional);

  h->add_property("up", &(transform->up), staticjson::Flags::Optional);
}

template <>
void init(
    pbrlab::scene_description::CyclesPrincipledBsdf* cycles_principled_bsdf,
    ObjectHandler* h) {
  h->add_property("base_color", &(cycles_principled_bsdf->base_color),
                  staticjson::Flags::Optional);

  h->add_property("subsurface", &(cycles_principled_bsdf->subsurface),
                  staticjson::Flags::Optional);

  h->add_property("subsurface_radius",
                  &(cycles_principled_bsdf->subsurface_radius),
                  staticjson::Flags::Optional);

  h->add_property("subsurface_color",
                  &(cycles_principled_bsdf->subsurface_color),
                  staticjson::Flags::Optional);

  h->add_property("metallic", &(cycles_principled_bsdf->metallic),
                  staticjson::Flags::Optional);

  h->add_property("specular", &(cycles_principled_bsdf->specular),
                  staticjson::Flags::Optional);

  h->add_property("specular_tint", &(cycles_principled_bsdf->specular_tint),
                  staticjson::Flags::Optional);

  h->add_property("roughness", &(cycles_principled_bsdf->roughness),
                  staticjson::Flags::Optional);

  h->add_property("anisotropic", &(cycles_principled_bsdf->anisotropic),
                  staticjson::Flags::Optional);

  h->add_property("anisotropic_rotation",
                  &(cycles_principled_bsdf->anisotropic_rotation),
                  staticjson::Flags::Optional);

  h->add_property("sheen", &(cycles_principled_bsdf->sheen),
                  staticjson::Flags::Optional);

  h->add_property("sheen_tint", &(cycles_principled_bsdf->sheen_tint),
                  staticjson::Flags::Optional);

  h->add_property("clearcoat", &(cycles_principled_bsdf->clearcoat),
                  staticjson::Flags::Optional);

  h->add_property("clearcoat_roughness",
                  &(cycles_principled_bsdf->clearcoat_roughness),
                  staticjson::Flags::Optional);

  h->add_property("ior", &(cycles_principled_bsdf->ior),
                  staticjson::Flags::Optional);

  h->add_property("transmission", &(cycles_principled_bsdf->transmission),
                  staticjson::Flags::Optional);

  h->add_property("transmission_roughness",
                  &(cycles_principled_bsdf->transmission_roughness),
                  staticjson::Flags::Optional);

  h->add_property("base_color_tex_name",
                  &(cycles_principled_bsdf->base_color_tex_name),
                  staticjson::Flags::Optional);

  h->add_property("subsurface_color_tex_name",
                  &(cycles_principled_bsdf->subsurface_color_tex_name),
                  staticjson::Flags::Optional);
}

template <>
void init(pbrlab::scene_description::HairBsdf* hair_bsdf, ObjectHandler* h) {
  h->add_property("coloring_hair", &(hair_bsdf->coloring_hair),
                  staticjson::Flags::Optional);

  h->add_property("base_color", &(hair_bsdf->base_color),
                  staticjson::Flags::Optional);

  h->add_property("melanin", &(hair_bsdf->melanin),
                  staticjson::Flags::Optional);

  h->add_property("melanin_redness", &(hair_bsdf->melanin_redness),
                  staticjson::Flags::Optional);

  h->add_property("roughness", &(hair_bsdf->roughness),
                  staticjson::Flags::Optional);

  h->add_property("azimuthal_roughness", &(hair_bsdf->azimuthal_roughness),
                  staticjson::Flags::Optional);

  h->add_property("ior", &(hair_bsdf->ior), staticjson::Flags::Optional);

  h->add_property("shift", &(hair_bsdf->shift), staticjson::Flags::Optional);

  h->add_property("specular_tint", &(hair_bsdf->specular_tint),
                  staticjson::Flags::Optional);

  h->add_property("second_specular_tint", &(hair_bsdf->second_specular_tint),
                  staticjson::Flags::Optional);

  h->add_property("transmission_tint", &(hair_bsdf->transmission_tint),
                  staticjson::Flags::Optional);
}

template <>
void init(pbrlab::scene_description::Material* material, ObjectHandler* h) {
  h->add_property("type", &(material->type));

  h->add_property("name", &(material->name));

  h->add_property("cycles_principled_bsdf",
                  &(material->cycles_principled_bsdf));

  h->add_property("hair_bsdf", &(material->hair_bsdf));
}

template <>
void init(pbrlab::scene_description::Light* light, ObjectHandler* h) {
  h->add_property("type", &(light->type));

  h->add_property("name", &(light->name));

  h->add_property("emission", &(light->emission));

  h->add_property("transform", &(light->transform),
                  staticjson::Flags::Optional);
}

template <>
void init(pbrlab::scene_description::Texture* texture, ObjectHandler* h) {
  h->add_property("name", &(texture->name));

  h->add_property("filepath", &(texture->filepath));
}

template <>
void init(pbrlab::scene_description::LocalScene* local_scene,
          ObjectHandler* h) {
  h->add_property("name", &(local_scene->name));

  h->add_property("meshes", &(local_scene->meshes),
                  staticjson::Flags::Optional);
}

template <>
void init(pbrlab::scene_description::Instance* instance, ObjectHandler* h) {
  h->add_property("local_scene", &(instance->local_scene));

  h->add_property("materials", &(instance->materials));

  h->add_property("lights", &(instance->lights));

  h->add_property("transform", &(instance->transform),
                  staticjson::Flags::Optional);
}

template <>
void init(pbrlab::scene_description::World* world, ObjectHandler* h) {
  (void)world;
  (void)h;
  // TODO
}

template <>
void init(pbrlab::scene_description::Setting* setting, ObjectHandler* h) {
  (void)setting;
  (void)h;
  // TODO
}

template <>
void init(pbrlab::scene_description::Root* scene_description_root,
          ObjectHandler* h) {
  h->add_property("wavefront_objs", &(scene_description_root->wavefront_objs));

  h->add_property("cyhairs", &(scene_description_root->cyhairs));

  h->add_property("materials", &(scene_description_root->materials));

  h->add_property("lights", &(scene_description_root->lights));

  h->add_property("local_scenes", &(scene_description_root->local_scenes));

  h->add_property("instances", &(scene_description_root->instances));

  h->add_property("textures", &(scene_description_root->textures));

  h->add_property("world", &(scene_description_root->world));

  h->add_property("setting", &(scene_description_root->setting));
}

}  // namespace staticjson

namespace pbrlab {
namespace io {

bool LoadSceneDescriptionFromJson(
    const std::string& filepath,
    scene_description::Root* scene_description_root) {
  staticjson::ParseStatus res;
  if (!staticjson::from_json_file(filepath, scene_description_root, &res)) {
    // TOOD use logger
    std::cerr << "Failed to parse JSON.\n";
    std::cerr << res.description() << "\n";
    return false;
  }
  return true;
}

}  // namespace io
}  // namespace pbrlab
