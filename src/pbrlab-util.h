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

inline float3 SafeDivideSpectrum(const float3& a, const float3& b) {
  float3 c;
  if (std::fabs(b[0]) < std::numeric_limits<float>::epsilon()) {
    c[0] = 0.0f;
  } else {
    c[0] = a[0] / b[0];
  }

  if (std::fabs(b[1]) < std::numeric_limits<float>::epsilon()) {
    c[1] = 0.0f;
  } else {
    c[1] = a[1] / b[1];
  }

  if (std::fabs(b[2]) < std::numeric_limits<float>::epsilon()) {
    c[2] = 0.0f;
  } else {
    c[2] = a[2] / b[2];
  }

  return c;
}

inline float RgbToY(const float3& c) {
  const float y_weight[3] = {0.212671f, 0.715160f, 0.072169f};
  return y_weight[0] * c[0] + y_weight[1] * c[1] + y_weight[2] * c[2];
}

inline bool IsBlack(const float3& v) {
  return ((std::fabs(v[0]) + std::fabs(v[1]) + std::fabs(v[2])) <
          std::numeric_limits<float>::epsilon());
}

inline bool IsFinite(const float3& v) {
  return std::isfinite(v[0]) && std::isfinite(v[1]) && std::isfinite(v[2]);
}

}  // namespace pbrlab

#endif  // PBRLAB_UTIL_H_
