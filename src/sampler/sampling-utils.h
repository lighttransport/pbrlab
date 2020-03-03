#ifndef PBRLAB_SAMPLING_UTILES_H_
#define PBRLAB_SAMPLING_UTILES_H_
#include <algorithm>
#include <utility>

#include "pbrlab_math.h"
#include "type.h"
namespace pbrlab {

inline float3 CosineSampleHemisphere(const float u1, const float u2) {
  const float u1_ = u1 * 2.0f * kPi, u3 = std::sqrt(u2);
  return float3(std::cos(u1_) * u3, std::sin(u1_) * u3,
                std::sqrt(std::max(1.0f - u2, 0.0f)));
}

inline float3 UniformSampleSphere(const float u1, const float u2) {
  const float u     = 2.0f * u2 - 1.0f;
  const float norm  = std::sqrt(std::max(0.0f, 1.0f - u * u));
  const float theta = 2.0f * kPi * u1;

  // Y up
  return float3(norm * std::cos(theta), u, norm * std::sin(theta));
}

inline float UniformSampleSpherePdf() { return 1.0f / (4.0f * kPi); }

inline float PowerHeuristicWeight(const float sampled_pdf,
                                  const float other_pdf) {
  float r = 0.f, mis = 0.f;
  if (sampled_pdf > other_pdf) {
    r   = other_pdf / sampled_pdf;
    mis = 1 / (1 + r * r);
  } else if (sampled_pdf < other_pdf) {
    r   = sampled_pdf / other_pdf;
    mis = 1 - 1 / (1 + r * r);
  } else {
    // avoid (possible, but extremely rare) inf/inf cases
    assert(std::fabs(sampled_pdf - other_pdf) <
           std::numeric_limits<float>::epsilon());
    mis = 0.5f;
  }

#if !defined(NDEBUG)
  float invr, invmis;
  if (other_pdf > sampled_pdf) {
    invr   = sampled_pdf / other_pdf;
    invmis = 1 / (1 + r * r);
  } else {
    invr   = other_pdf / sampled_pdf;
    invmis = 1 - 1 / (1 + r * r);
  }

  assert(std::abs(mis + invmis - 1.0f) < 0.001f);
#endif

  return mis;
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
