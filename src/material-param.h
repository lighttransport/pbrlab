#ifndef PBRLAB_MATERIAL_PARAM_H_
#define PBRLAB_MATERIAL_PARAM_H_

#include <stdint.h>

#include "type.h"

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#endif

#include "mpark/variant.hpp"

#ifdef __clang__
#pragma clang diagnostic pop
#endif

namespace pbrlab {
enum MaterialParameterType { kCyclesPrincipledBsdfParameter = 0 };

struct CyclesPrincipledBsdfParameter {
  float3 base_color        = {static_cast<float>(0.8), static_cast<float>(0.8),
                       static_cast<float>(0.8)};
  float3 subsurface_radius = {static_cast<float>(1.0), static_cast<float>(1.0),
                              static_cast<float>(1.0)};
  float3 subsurface_color  = {static_cast<float>(0.7), static_cast<float>(0.1),
                             static_cast<float>(0.1)};
  float metallic           = static_cast<float>(0.0);
  float specular           = static_cast<float>(0.5);
  float roughness          = static_cast<float>(0.5);
  float anisotropic        = static_cast<float>(0.0);
  float anisotropic_rotation   = static_cast<float>(0.0);
  float sheen                  = static_cast<float>(0.0);
  float sheen_tint             = static_cast<float>(0.5);
  float clearcoat              = static_cast<float>(0.0);
  float clearcoat_roughness    = static_cast<float>(0.03);
  float ior                    = static_cast<float>(1.45);
  float transmission           = static_cast<float>(0.0);
  float transmission_roughness = static_cast<float>(0.0);

  uint32_t base_color_tex_id       = static_cast<uint32_t>(-1);
  uint32_t subsurface_color_tex_id = static_cast<uint32_t>(-1);
};

using MaterialParameter = mpark::variant<CyclesPrincipledBsdfParameter>;
}  // namespace pbrlab
#endif  // PBRLAB_MAfloatERIAL_PARAM_H_
