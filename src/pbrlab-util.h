#ifndef PBRLAB_UTIL_H_
#define PBRLAB_UTIL_H_

#include <algorithm>

#include "type.h"

namespace pbrlab {
inline float SpectrumNorm(const float3& c) {
  return std::max({c[0], c[1], c[2]});
}

}  // namespace pbrlab

#endif  // PBRLAB_UTIL_H_
