#ifndef PBRLAB_LAMBERT_H_
#define PBRLAB_LAMBERT_H_
#include <array>
#include <tuple>

#include "sampler/sampling-utils.h"
#include "type.h"

namespace pbrlab {

inline float LambertPdf(const float3& omega_in, const float3& omega_out) {
  (void)omega_out;
  return omega_in[2] * kPiInv;
}

inline float LambertBrdfPdf(const float3& omega_in, const float3& omega_out,
                            float* pdf) {
  *pdf = LambertPdf(omega_in, omega_out);
  return kPiInv;
}

inline float LambertBrdfSample(const float3& omega_out,
                               const std::array<float, 2>& u, float3* omega_in,
                               float* pdf) {
  *omega_in = CosineSampleHemisphere(u[0], u[1]);
  return LambertBrdfPdf(*omega_in, omega_out, pdf);
}

}  // namespace pbrlab
#endif  // PBRLAB_LAMBERT_H_
