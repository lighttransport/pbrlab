#ifndef PBRLAB_MATH_H_
#define PBRLAB_MATH_H_
#include "nanort.h"
#include "type.h"

namespace pbrlab {
constexpr const float kPi    = 3.141592653589793f;
constexpr const float kPiInv = 0.318309886183f;

constexpr const float kEps = 1e-3f;      // TODO
constexpr const float kInf = 1.844E18f;  // for embree

inline float SafeSqrtf(float f) { return std::sqrt(std::max(f, 0.0f)); }

// TODO better way
inline float3 vcross(const float3& a, const float3& b) {
  return nanort::vcross(a, b);
}
inline float vdot(const float3& a, const float3& b) {
  return nanort::vdot(a, b);
}
inline float vlength(const float3& v) { return nanort::vlength(v); }
inline float3 vnormalized(const float3& v) { return nanort::vnormalize(v); }

template <typename T>
auto Lerp3(const T& v0, const T& v1, const T& v2, const float u,
           const float v) {
  return (1.0f - u - v) * v0 + u * v1 + v * v2;
}
}  // namespace pbrlab
#endif  // PBRLAB_MATH_H_
