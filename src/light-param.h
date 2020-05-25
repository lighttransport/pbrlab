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
  std::string name;
};
enum LightType { kAreaLight = 0, kLightNone };
using LightParameter = mpark::variant<AreaLightParameter>;

inline void SetLightName(const std::string& name, LightParameter* light_param) {
  if (light_param->index() == kAreaLight) {
    mpark::get<kAreaLight>(*light_param).name = name;
  } else {
    assert(false);
  }
}

inline std::string GetLightName(const LightParameter& light_param) {
  if (light_param.index() == kAreaLight) {
    return mpark::get<kAreaLight>(light_param).name;
  } else {
    assert(false);
  }
  return "";
}
}  // namespace pbrlab

#endif  // PBRLAB_LIGHT_PARAM_H_
