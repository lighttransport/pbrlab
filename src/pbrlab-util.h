#ifndef PBRLAB_UTIL_H_
#define PBRLAB_UTIL_H_

#include <algorithm>

#include "type.h"

namespace pbrlab {
template <typename T>
inline T Clamp(const T x, const T a, const T b) {
  return std::max(a, std::min(b, x));
}

template <typename T>
inline T Saturate(const T x) {
  return Clamp(x, T(0), T(1));
}

inline float Average(const float3& c) { return (c[0] + c[1] + c[2]) / 3.f; }

inline float SpectrumNorm(const float3& c) {
  return std::max({c[0], c[1], c[2]});
}

inline float RgbToY(const float3& c) {
  const float y_weight[3] = {0.212671f, 0.715160f, 0.072169f};
  return y_weight[0] * c[0] + y_weight[1] * c[1] + y_weight[2] * c[2];
}

static inline bool IsBlack(const float3& v) {
  return ((std::fabs(v[0]) + std::fabs(v[1]) + std::fabs(v[2])) <
          std::numeric_limits<float>::epsilon());
}

static inline bool IsFinite(const float3& v) {
  return std::isfinite(v[0]) && std::isfinite(v[1]) && std::isfinite(v[2]);
}

}  // namespace pbrlab

#endif  // PBRLAB_UTIL_H_
