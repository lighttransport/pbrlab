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

inline auto LambertBrdfPdf(const float3& omega_in, const float3& omega_out) {
  (void)omega_out;
  struct {
    float brdf_f, pdf;
  } ret = {kPiInv, LambertPdf(omega_in, omega_out)};
  return ret;
}

inline auto LambertBrdfSample(const float3& omega_out,
                              const std::array<float, 2>& u) {
  const float3 omega_in = CosineSampleHemisphere(u[0], u[1]);
  const auto tmp        = LambertBrdfPdf(omega_in, omega_out);
  struct {
    float3 omega_in;
    float brdf_f, pdf;
  } ret = {omega_in, tmp.brdf_f, tmp.pdf};
  return ret;
}

}  // namespace pbrlab
#endif  // PBRLAB_LAMBERT_H_
