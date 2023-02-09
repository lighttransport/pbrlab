#ifndef PBRLAB_MATH_H_
#define PBRLAB_MATH_H_
#include "nanort.h"
#include "type.h"

namespace pbrlab {
constexpr const float kPi    = 3.141592653589793f;
constexpr const float kPiInv = 0.318309886183f;

constexpr const float kEps = 1e-3f;      // TODO
constexpr const float kInf = 1.844E18f;  // for embree

template <typename T>
inline T Sqr(const T& v) {
  return v * v;
}
inline float SafeSqrtf(float f) { return std::sqrt((std::max)(f, 0.0f)); }

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
inline T Lerp(const T& v0, const T& v1, const float u) {
  return (1.0f - u) * v0 + u * v1;
}

template <typename T>
inline T Lerp3(const T& v0, const T& v1, const T& v2, const float u,
               const float v) {
  return (1.0f - u - v) * v0 + u * v1 + v * v2;
}

template <int n>
inline float Pow(const float v) {
  static_assert(n > 0, "Power can’t be negative");
  const float n2 = Pow<n / 2>(v);
  return n2 * n2 * Pow<n & 1>(v);
}
template <>
inline float Pow<1>(const float v) {
  return v;
}
template <>
inline float Pow<0>(const float v) {
  (void)v;
  return 1.f;
}

namespace fast_math {

// --------------------------------------------------------------
// From OpenImageIO fmath

/***

BSD 3-Clause License

Copyright (c) 2008-present by Contributors to the OpenImageIO project.
All Rights Reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

***/

template <typename T>
inline T Clamp(const T x, const T a, const T b) {
  return (std::max)(a, (std::min)(b, x));
}

template <typename IN_TYPE, typename OUT_TYPE>
inline OUT_TYPE BitCast(const IN_TYPE in) {
  // NOTE: this is the only standards compliant way of doing this type of
  // casting, luckily the compilers we care about know how to optimize away this
  // idiom.
  OUT_TYPE out;
  memcpy(reinterpret_cast<void*>(&out), &in, sizeof(IN_TYPE));
  return out;
}

const double kM_1_PI = 0.31830988618379067154;
const double kM_2_PI = 0.63661977236758134308;
const double kM_PI_2 = 1.57079632679489661923;
const double kM_LN2  = 0.69314718055994530942;

/// Fused multiply and add: (a*b + c)
inline float Madd(float a, float b, float c) {
  // C++11 defines std::fma, which we assume is implemented using an
  // intrinsic.
  return std::fma(a, b, c);
  // // NOTE: GCC/ICC will turn this (for float) into a FMA unless
  // // explicitly asked not to, clang will do so if -ffp-contract=fast.
  // return a * b + c;
}

/// Round to nearest integer, returning as an int.
inline int FastRint(float x) {
  // used by sin/cos/tan range reduction
  // single roundps instruction on SSE4.1+ (for gcc/clang at least)
  return static_cast<int>(std::rint(x));

  // emulate rounding by adding/substracting 0.5
  // return static_cast<int>(x + std::copysign(0.5f, x));
}

inline float FastSin(float x) {
  // very accurate argument reduction from SLEEF
  // starts failing around x=262000
  // Results on: [-2pi,2pi]
  // Examined 2173837240 values of sin: 0.00662760244 avg ulp diff, 2 max
  // ulp, 1.19209e-07 max error
  int q    = FastRint(x * float(kM_1_PI));
  float qf = float(q);
  x        = Madd(qf, -0.78515625f * 4, x);
  x        = Madd(qf, -0.00024187564849853515625f * 4, x);
  x        = Madd(qf, -3.7747668102383613586e-08f * 4, x);
  x        = Madd(qf, -1.2816720341285448015e-12f * 4, x);
  x        = float(kM_PI_2) - (float(kM_PI_2) - x);  // crush denormals
  float s  = x * x;
  if ((q & 1) != 0) x = -x;
  // this polynomial approximation has very low error on [-pi/2,+pi/2]
  // 1.19209e-07 max error in total over [-2pi,+2pi]
  float u = 2.6083159809786593541503e-06f;
  u       = Madd(u, s, -0.0001981069071916863322258f);
  u       = Madd(u, s, +0.00833307858556509017944336f);
  u       = Madd(u, s, -0.166666597127914428710938f);
  u       = Madd(s, u * x, x);
  // For large x, the argument reduction can fail and the polynomial can be
  // evaluated with arguments outside the valid internal. Just clamp the bad
  // values away (setting to 0.0f means no branches need to be generated).
  if (std::fabs(u) > 1.0f) u = 0.0f;
  return u;
}

inline float FastCos(float x) {
  // same argument reduction as FastSin
  int q    = FastRint(x * float(kM_1_PI));
  float qf = float(q);
  x        = Madd(qf, -0.78515625f * 4, x);
  x        = Madd(qf, -0.00024187564849853515625f * 4, x);
  x        = Madd(qf, -3.7747668102383613586e-08f * 4, x);
  x        = Madd(qf, -1.2816720341285448015e-12f * 4, x);
  x        = float(kM_PI_2) - (float(kM_PI_2) - x);  // crush denormals
  float s  = x * x;
  // polynomial from SLEEF's sincosf, max error is
  // 4.33127e-07 over [-2pi,2pi] (98% of values are "exact")
  float u = -2.71811842367242206819355e-07f;
  u       = Madd(u, s, +2.47990446951007470488548e-05f);
  u       = Madd(u, s, -0.00138888787478208541870117f);
  u       = Madd(u, s, +0.0416666641831398010253906f);
  u       = Madd(u, s, -0.5f);
  u       = Madd(u, s, +1.0f);
  if ((q & 1) != 0) u = -u;
  if (std::fabs(u) > 1.0f) u = 0.0f;
  return u;
}

inline void FastSincos(float x, float* sine, float* cosine) {
  // same argument reduction as FastSin
  int q    = FastRint(x * float(kM_1_PI));
  float qf = float(q);
  x        = Madd(qf, -0.78515625f * 4, x);
  x        = Madd(qf, -0.00024187564849853515625f * 4, x);
  x        = Madd(qf, -3.7747668102383613586e-08f * 4, x);
  x        = Madd(qf, -1.2816720341285448015e-12f * 4, x);
  x        = float(kM_PI_2) - (float(kM_PI_2) - x);  // crush denormals
  float s  = x * x;
  // NOTE: same exact polynomials as FastSin and FastCos above
  if ((q & 1) != 0) x = -x;
  float su = 2.6083159809786593541503e-06f;
  su       = Madd(su, s, -0.0001981069071916863322258f);
  su       = Madd(su, s, +0.00833307858556509017944336f);
  su       = Madd(su, s, -0.166666597127914428710938f);
  su       = Madd(s, su * x, x);
  float cu = -2.71811842367242206819355e-07f;
  cu       = Madd(cu, s, +2.47990446951007470488548e-05f);
  cu       = Madd(cu, s, -0.00138888787478208541870117f);
  cu       = Madd(cu, s, +0.0416666641831398010253906f);
  cu       = Madd(cu, s, -0.5f);
  cu       = Madd(cu, s, +1.0f);
  if ((q & 1) != 0) cu = -cu;
  if (std::fabs(su) > 1.0f) su = 0.0f;
  if (std::fabs(cu) > 1.0f) cu = 0.0f;
  *sine   = su;
  *cosine = cu;
}

inline float FastExp2(const float& xval) {
  // clamp to safe range for final addition
  float x = Clamp(xval, -126.0f, 126.0f);
  // range reduction
  int m = int(x);
  x -= float(m);
  x = 1.0f - (1.0f - x);  // crush denormals (does not affect max ulps!)
  // 5th degree polynomial generated with sollya
  // Examined 2247622658 values of exp2 on [-126,126]: 2.75764912 avg ulp diff,
  // 232 max ulp ulp histogram:
  //  0  = 87.81%
  //  1  =  4.18%
  float r = 1.33336498402e-3f;
  r       = Madd(x, r, 9.810352697968e-3f);
  r       = Madd(x, r, 5.551834031939e-2f);
  r       = Madd(x, r, 0.2401793301105f);
  r       = Madd(x, r, 0.693144857883f);
  r       = Madd(x, r, 1.0f);
  // multiply by 2 ^ m by adding in the exponent
  // NOTE: left-shift of negative number is undefined behavior
  return BitCast<unsigned, float>(BitCast<float, unsigned>(r) +
                                  (unsigned(m) << 23));
}

template <typename T>
inline T FastExp(const T& x) {
  // Examined 2237485550 values of exp on [-87.3300018,87.3300018]: 2.6666452
  // avg ulp diff, 230 max ulp
  return FastExp2(x * T(1 / kM_LN2));
}

inline float FastAtan2(const float y, const float x) {
  // based on atan approximation above
  // the special cases around 0 and infinity were tested explicitly
  // the only case not handled correctly is x=NaN,y=0 which returns 0 instead of
  // nan
  const float a = std::fabs(x);
  const float b = std::fabs(y);
  // TODO
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#endif
  const float k = (b == 0) ? 0.0f : ((a == b) ? 1.0f : (b > a ? a / b : b / a));
#ifdef __clang__
#pragma clang diagnostic pop
#endif
  const float s = 1.0f - (1.0f - k);  // crush denormals
  const float t = s * s;

  float r = s * Madd(0.430165678f, t, 1.0f) /
            Madd(Madd(0.0579354987f, t, 0.763007998f), t, 1.0f);

  if (b > a) r = 1.570796326794896557998982f - r;  // account for arg reduction
  if (BitCast<float, unsigned>(x) & 0x80000000u)   // test sign bit of x
    r = float(kPi) - r;
  return copysignf(r, y);
}

inline float FastAsin(const float x) {
  // based on acosf approximation above
  // max error is 4.51133e-05 (ulps are higher because we are consistently off
  // by a little amount)
  const float f = std::fabs(x);
  const float m =
      (f < 1.0f) ? 1.0f - (1.0f - f) : 1.0f;  // clamp and crush denormals
  const float a =
      float(kM_PI_2) -
      sqrtf(1.0f - m) *
          (1.5707963267f +
           m * (-0.213300989f + m * (0.077980478f + m * -0.02164095f)));
  return std::copysign(a, x);
}

inline float FastSinh(const float x) {
  float a = std::fabs(x);
  if (a > 1.0f) {
    // Examined 53389559 values of sinh on [1,87.3300018]: 33.6886442 avg ulp
    // diff, 178 max ulp
    float e = FastExp(a);
    return copysignf(0.5f * e - 0.5f / e, x);
  } else {
    a        = 1.0f - (1.0f - a);  // crush denorms
    float a2 = a * a;
    // degree 7 polynomial generated with sollya
    // Examined 2130706434 values of sinh on [-1,1]: 1.19209e-07 max error
    float r = 2.03945513931e-4f;
    r       = Madd(r, a2, 8.32990277558e-3f);
    r       = Madd(r, a2, 0.1666673421859f);
    r       = Madd(r * a, a2, a);
    return copysignf(r, x);
  }
}

inline float FastLog2(const float& xval) {
  // NOTE: clamp to avoid special cases and make result "safe" from large
  // negative values/nans
  float x = Clamp(xval, (std::numeric_limits<float>::min)(),
                  (std::numeric_limits<float>::max)());
  // based on https://github.com/LiraNuna/glsl-sse2/blob/master/source/vec4.h
  unsigned bits = BitCast<float, unsigned>(x);
  int exponent  = int(bits >> 23) - 127;
  float f = BitCast<unsigned, float>((bits & 0x007FFFFF) | 0x3f800000) - 1.0f;
  // Examined 2130706432 values of log2 on [1.17549435e-38,3.40282347e+38]:
  // 0.0797524457 avg ulp diff, 3713596 max ulp, 7.62939e-06 max error ulp
  // histogram:
  //  0  = 97.46%
  //  1  =  2.29%
  //  2  =  0.11%
  float f2 = f * f;
  float f4 = f2 * f2;
  float hi = Madd(f, -0.00931049621349f, 0.05206469089414f);
  float lo = Madd(f, 0.47868480909345f, -0.72116591947498f);
  hi       = Madd(f, hi, -0.13753123777116f);
  hi       = Madd(f, hi, 0.24187369696082f);
  hi       = Madd(f, hi, -0.34730547155299f);
  lo       = Madd(f, lo, 1.442689881667200f);
  return ((f4 * hi) + (f * lo)) + float(exponent);
}

inline float FastLog(const float& x) {
  // Examined 2130706432 values of logf on [1.17549435e-38,3.40282347e+38]:
  // 0.313865375 avg ulp diff, 5148137 max ulp, 7.62939e-06 max error
  return FastLog2(x) * float(kM_LN2);
}

// ---------------------------------------------------------

}  // namespace fast_math
}  // namespace pbrlab
#endif  // PBRLAB_MATH_H_
