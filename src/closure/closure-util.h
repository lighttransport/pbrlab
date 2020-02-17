#ifndef PBRLAB_CLOSURE_UTIL_H_
#define PBRLAB_CLOSURE_UTIL_H_

#include <math.h>

#include <limits>

namespace pbrlab {

inline float FresnelDielectricCos(const float cos_, float eta) {
  if (std::fabs(eta) < std::numeric_limits<float>::epsilon()) {
    // ignore fresnel when eta is zero.
    return 1.0f;
  }

  // compute fresnel reflectance without explicitly computing
  // the refracted direction
  if (cos_ < 0.0f) eta = 1.0f / eta;

  const float c = std::fabs(cos_);
  float g       = eta * eta - 1 + c * c;
  if (g > 0) {
    g             = std::sqrt(g);
    const float A = (g - c) / (g + c);
    const float B = (c * (g + c) - 1) / (c * (g - c) + 1);
    return 0.5f * A * A * (1 + B * B);
  }
  return 1.0f;  // TIR(no refracted component)
}

}  // namespace pbrlab

#endif  // PBRLAB_CLOSURE_UTIL_H_
