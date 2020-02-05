#ifndef PBRLAB_LIGHT_PARAM_H_
#define PBRLAB_LIGHT_PARAM_H_

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

struct AreaLightParameter {
  float3 emission = float3(0.8f);
};
enum LightType { kAreaLight = 0, kLightNone };
using LightParameter = mpark::variant<AreaLightParameter>;
}  // namespace pbrlab

#endif  // PBRLAB_LIGHT_PARAM_H_
