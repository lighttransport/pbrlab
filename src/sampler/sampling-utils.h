#ifndef PBRLAB_SAMPLING_UTILES_H_
#define PBRLAB_SAMPLING_UTILES_H_
#include <utility>

#include "pbrlab_math.h"
#include "type.h"
namespace pbrlab {

inline float3 CosineSampleHemisphere(const float u1, const float u2);
inline std::pair<float, float> TriangleUniformSampler(const float u1,
                                                      const float u2);

inline float3 CosineSampleHemisphere(const float u1, const float u2) {
  const float u1_ = u1 * 2.0f * kPi, u3 = std::sqrt(u2);
  return float3(std::cos(u1_) * u3, std::sin(u1_) * u3,
                std::sqrt(std::max(1.0f - u2, 0.0f)));
}

inline std::pair<float, float> TriangleUniformSampler(const float u1,
                                                      const float u2) {
  const bool flag = (u1 > u2);
  const float M   = flag ? u1 : u2;
  const float m   = (!flag) ? u1 : u2;

  return std::make_pair(1.0f - M, M - m);
}

}  // namespace pbrlab
#endif  // PBRLAB_SAMPLING_UTILES_H_
