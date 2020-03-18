#ifndef PBRLAB_ENERGY_CONSERVING_HAIR_BSDF_H_
#define PBRLAB_ENERGY_CONSERVING_HAIR_BSDF_H_
#include <math.h>
#include <stdint.h>

#include <array>
#include <iostream>
#include <numeric>

#include "pbrlab-util.h"
#include "pbrlab_math.h"

// Use improved robe evaluation method described in
//   siggraph course: path tracing in production 2018
//   From Bricks to Bunnies - Adapting a Raytracer to New Requirements
//   https://jo.dreggn.org/path-tracing-in-production/2018/index.html
#define USE_IMPROVED_ROBE_EVALUATION (1)
#define USE_FAST_MATH (1)

namespace pbrlab {
namespace hair_bsdf {

// TODO using namespace
#if USE_FAST_MATH
#define MY_COS(x) fast_math::FastCos((x))
#define MY_SIN(x) fast_math::FastSin((x))
#define MY_ATAN2(x, y) fast_math::FastAtan2((x), (y))
#define MY_ASIN(x) fast_math::FastAsin((x))
#define MY_EXP(x) fast_math::FastExp((x))
#define MY_LOG(x) fast_math::FastLog((x))
// #define MY_SINH(x) fast_math::FastSinh((x))
#else
#define MY_COS(x) std::cos((x))
#define MY_SIN(x) std::sin((x))
#define MY_ATAN2(x, y) std::atan2((x), (y))
#define MY_ASIN(x) std::asin((x))
#define MY_EXP(x) std::exp((x))
#define MY_LOG(x) std::log((x))
#define MY_SINH(x) std::sinh((x))
#endif

inline float SafeASin(const float x) {
  const float ret = MY_ASIN(x);
  if (std::isnan(ret)) {
    std::cerr << "asin warning x = " << x << std::endl;
    return MY_ASIN(Clamp(x, -1.0, 1.0));
  }
  return ret;
}

inline float I0(const float x,
                const int n = 10) {  // When x is close to 12, there is a
                                     // difference from double (about 0.001).
  double ret           = 0.0;
  double m_exclamation = 1.0;
  double x_2_2m        = 1.0;
  const double x_2     = double(x) * 0.5;

  for (int m = 0; m < n; m++) {
    if (m != 0) {
      m_exclamation *= m;
    }

    const double x_m = 1.0 / (m_exclamation * m_exclamation) * x_2_2m;

    if (!std::isfinite(x_m)) {
      // TODO use logger
      std::cerr << "x_" << m << " is nan or inf" << std::endl;
      std::cerr << "m_exclamation = " << m_exclamation << " x_2_2m = " << x_2_2m
                << " x =" << x << std::endl;
      continue;
    }

    ret += x_m;

    x_2_2m *= x_2 * x_2;
  }

  return float(ret);
}

inline float Horner(const float x, const float a[], const int n) {
  float f = a[n];

  for (int i = n - 1; i >= 0; i--) {
    f = f * x + a[i];
  }

  return f;
}

inline float SafeLogI0(float x) {
  x = std::abs(x);

#if 0
  float A=0;

  if (x < 7.5f) {
    static const float P[] = {
      1.00000003928615375e+00f,
      2.49999576572179639e-01f,
      2.77785268558399407e-02f,
      1.73560257755821695e-03f,
      6.96166518788906424e-05f,
      1.89645733877137904e-06f,
      4.29455004657565361e-08f,
      3.90565476357034480e-10f,
      1.48095934745267240e-11f 
    };  

    float x22 = x * x / 4.0f;

    A = std::log(x22 * Horner(x22,P,8) + 1.0f);
  } else {
    static const float P[] = {
      3.98942651588301770e-01f,   
      4.98327234176892844e-02f,   
      2.91866904423115499e-02f,   
      1.35614940793742178e-02f,   
      1.31409251787866793e-01f    
    };

    const float inv_x = 1.0f / x;

    const float Px = Horner(inv_x,P,4);

    A = x + 0.5f * std::log(Px * Px * inv_x);
  }

  float B = 0.0;

  if (x > 12.0f) {
    B = x + 0.5f * (-std::log(2.0f * kPI) + std::log(1.0f / x) + 1.0f / (8.0f * x));
  }else { 
    B = std::log(I0(x));
  }

  std::cerr << x << " " << A << " " << B << " " << std::abs(A - B) / std::abs(B) << std::endl;
#endif

#if USE_IMPROVED_ROBE_EVALUATION
  if (x < 7.5f) {
    static const float P[] = {
        1.00000003928615375e+00f, 2.49999576572179639e-01f,
        2.77785268558399407e-02f, 1.73560257755821695e-03f,
        6.96166518788906424e-05f, 1.89645733877137904e-06f,
        4.29455004657565361e-08f, 3.90565476357034480e-10f,
        1.48095934745267240e-11f};

    float x22 = x * x / 4.0f;

    return MY_LOG(x22 * Horner(x22, P, 8)) + 1.0f;
  }
  static const float P[] = {3.98942651588301770e-01f, 4.98327234176892844e-02f,
                            2.91866904423115499e-02f, 1.35614940793742178e-02f,
                            1.31409251787866793e-01f};

  const float inv_x = 1.0f / x;

  const float Px = Horner(inv_x, P, 4);

  return x + 0.5f * MY_LOG(Px * Px * inv_x);
#else
  if (x > 12.0f) {
    return x +
           0.5f * (-MY_LOG(2.0f * kPI) + MY_LOG(1.0f / x) + 1.0f / (8.0f * x));
  }
  return MY_LOG(I0(x));
#endif
}

inline float Mp(const float sin_theta_i, const float cos_theta_i,
                const float sin_theta_o, const float cos_theta_o, float v) {
  const float ccv = cos_theta_i * cos_theta_o / v;
  const float ssv = sin_theta_i * sin_theta_o / v;

  if (!std::isfinite(ccv)) {
    // TODO logger
    std::cerr << cos_theta_i << " " << cos_theta_o << " " << v << std::endl;
  }

#if USE_IMPROVED_ROBE_EVALUATION

  v = Clamp(v, 1e-5f, 1e4f);

  const float ret = MY_EXP(SafeLogI0(ccv) - ssv - 1.0f / v + MY_LOG(1.0f / v) -
                           MY_LOG(1.0f - MY_EXP(-2.0f / v)));

#else
  const float ret =
      (v <= 0.1f) ? (MY_EXP(SafeLogI0(ccv) - ssv - 1.0f / v + 0.6931f +
                            MY_LOG(1.0f / (2.0f * v))))
                  : (MY_EXP(-ssv) * I0(ccv)) /
                        (MY_SINH(1.0f / v) * 2.0f * v);  // v > 0.1 ⇒ ccv <= 10
#endif

  if (!std::isfinite(ret)) {
    std::cerr << "Mp is nan" << std::endl;
  }

  return ret;
}

inline std::array<float, 4> BetamToV(const float beta_m) {
  std::array<float, 4> vs;
  vs[0] = Sqr(0.726f * beta_m + 0.812f * Sqr(beta_m) + 3.7f * Pow<20>(beta_m));
  vs[1] = 0.25f * vs[0];
  vs[2] = 4.0f * vs[0];
  vs[3] = vs[2];

  return vs;
}

// TODO to closure-util.h
inline float FrDielectric(float cos_theta_i, float eta_i, float eta_t) {
  cos_theta_i   = Clamp(cos_theta_i, -1.0f, 1.0f);
  bool entering = cos_theta_i > 0.0f;
  if (!entering) {
    const float a = eta_i;
    eta_i         = eta_t;
    eta_t         = a;
    cos_theta_i   = std::abs(cos_theta_i);
  }

  // Compute _cosThetaT_ using Snell's law
  const float sin_theta_i =
      std::sqrt(std::max(0.0f, 1.0f - cos_theta_i * cos_theta_i));
  const float sin_theta_t = eta_i / eta_t * sin_theta_i;

  // Handle total internal reflection
  if (sin_theta_t >= 1.0f) return 1.0f;
  const float cos_theta_t =
      std::sqrt(std::max(0.0f, 1.0f - sin_theta_t * sin_theta_t));
  const float r_parl = ((eta_t * cos_theta_i) - (eta_i * cos_theta_t)) /
                       ((eta_t * cos_theta_i) + (eta_i * cos_theta_t));
  const float r_perp = ((eta_i * cos_theta_i) - (eta_t * cos_theta_t)) /
                       ((eta_i * cos_theta_i) + (eta_t * cos_theta_t));
  return (r_parl * r_parl + r_perp * r_perp) * 0.5f;
}

inline std::array<float3, 4> Ap(const float cos_theta_o, const float eta,
                                const float h, const float3 &T) {
  const float cos_gamma_o = SafeSqrtf(1.0f - h * h);

  const float cos_theta = cos_theta_o * cos_gamma_o;

  const float f = FrDielectric(cos_theta, 1.0f, eta);

  std::array<float3, 4> ap;

  ap[0] = float3(f);

  ap[1] = Sqr(1.0f - f) * T;

  ap[2] = ap[1] * T * f;

  ap[3] = ap[2] * f * T / (float3(1.0f) - T * f);
  if (!std::isfinite(ap[3][0]) || !std::isfinite(ap[3][1]) ||
      !std::isfinite(ap[3][2])) {
    ap[3] = float3(0.0f);
    // for both T and f are 1
  }

  return ap;
}

inline float Logistic(float x, const float s) {
  x = std::abs(x);

  const float numerator = MY_EXP(-x / s);
  return numerator / (s * Sqr(1.0f + numerator));
}

inline float LogisticCDF(const float x, const float s) {
  return 1.0f / (1.0f + MY_EXP(-x / s));
}

inline float TrimmedLogistic(const float x, const float s, const float a,
                             const float b) {
  return Logistic(x, s) / (LogisticCDF(b, s) - LogisticCDF(a, s));
}

inline float Phi(const int p, const float gamma_o, const float gamma_t) {
  return 2.0f * p * gamma_t - 2.0f * gamma_o + p * kPi;
}

inline float Fmod(const float a, const float b) {
  return a - std::floor(a / b) * b;
}

inline float Np(const float phi, const int p, const float s,
                const float gamma_o, const float gamma_t) {
  float dphi = Fmod(phi - Phi(p, gamma_o, gamma_t), 2.0f * kPi);
  if (dphi >= kPi) {
    dphi -= 2.0f * kPi;
  }

  return TrimmedLogistic(dphi, s, -kPi, kPi);
}

inline float CalcS(const float beta_n) {
  const float beta_n2 = Sqr(beta_n);
  return std::sqrt(kPi / 8.0f) *
         (0.265f * beta_n + 1.194f * beta_n2 + 5.372f * Pow<11>(beta_n2));
}

inline float AbsCosTheta(const float3 &omega) {
  return std::sqrt(omega[2] * omega[2] + omega[1] * omega[1]);
}

// TODO shader
inline float3 CalcSigmaAUsingMelaninParameter(float melanin,
                                              float melaninredness) {
  melanin        = Clamp(melanin, 0.0f, 1.0f);
  melaninredness = Clamp(melaninredness, 0.0f, 1.0f);

  melanin = -MY_LOG(std::max(1.0f - melanin, 0.0001f));

  const float eumelanin   = melanin * (1.0f - melaninredness);
  const float pheomelanin = melanin * melaninredness;

  return float3(std::max(0.0f, eumelanin * 0.506f + pheomelanin * 0.343f),
                std::max(0.0f, eumelanin * 0.841f + pheomelanin * 0.733f),
                std::max(0.0f, eumelanin * 1.653f + pheomelanin * 1.924f));
}

inline float3 CalcSigmaAFromConcentration(const float eumelanin,
                                          const float pheomelanin) {
  float3 eumelaninSigmaA(0.419f, 0.697f, 1.37f);
  float3 pheomelaninSigmaA(0.187f, 0.4f, 1.05f);

  return eumelanin * eumelaninSigmaA + pheomelanin * pheomelaninSigmaA;
}

inline float3 CalcSigmaAFromRGB(const float3 &c, const float beta_n) {
  float3 ret;

  for (int i = 0; i < 3; i++) {
    ret[i] =
        Sqr(MY_LOG(c[i]) / (5.969f - 0.215f * beta_n + 2.532f * Sqr(beta_n) -
                            10.73f * Pow<3>(beta_n) + 5.574f * Pow<4>(beta_n) +
                            0.245f * Pow<5>(beta_n)));
  }

  return ret;
}

inline float3 CalcSigmaAUsingMelaninParameterRandom(
    float melanin, float melaninredness, const float3 &tint, const float beta_n,
    const float random_color, const float random_value) {
  const float factor_random_color =
      1.0f + 2.0f * (random_value - 0.5f) * random_color;
  melanin        = Clamp(melanin, 0.0f, 1.0f) * factor_random_color;
  melaninredness = Clamp(melaninredness, 0.0f, 1.0f);

  melanin = -MY_LOG(std::max(1.0f - melanin, 0.0001f));

  const float eumelanin   = melanin * (1.0f - melaninredness);
  const float pheomelanin = melanin * melaninredness;

  const float3 melanin_sigma =
      float3(std::max(0.0f, eumelanin * 0.506f + pheomelanin * 0.343f),
             std::max(0.0f, eumelanin * 0.841f + pheomelanin * 0.733f),
             std::max(0.0f, eumelanin * 1.653f + pheomelanin * 1.924f));

  const float3 tint_sigma = CalcSigmaAFromRGB(tint, beta_n);

  return melanin_sigma + tint_sigma;
}

inline float3 EnergyConservingHairBsdfPdf(
    const float3 &omega_in, const float3 &omega_out, const float h,
    const std::array<float, 4> &v, const float s, const float sigma_a,
    const float eta, const float alpha /*radian*/,
    const std::array<float, 4> &tints, const float transparent_scale,
    float *pdf) {
  *pdf = 0.0f;

  const float sin_theta_o = omega_out[0];
  const float cos_theta_o = SafeSqrtf(1.0f - Sqr(sin_theta_o));

  float sin_2k_alpha[3];
  float cos_2k_alpha[3];

#if USE_FAST_MATH
  fast_math::FastSincos(alpha, &sin_2k_alpha[0], &cos_2k_alpha[0]);
#else
  sin_2k_alpha[0] = std::sin(alpha);
  cos_2k_alpha[0] = std::cos(alpha);
#endif

  for (int i = 1; i < 3; i++) {
    sin_2k_alpha[i] = 2.0f * sin_2k_alpha[i - 1] * cos_2k_alpha[i - 1];
    cos_2k_alpha[i] = Sqr(cos_2k_alpha[i - 1]) - Sqr(sin_2k_alpha[i - 1]);
  }

  float sin_theta_o_crts[4], cos_theta_o_crts[4];

  sin_theta_o_crts[0] =
      sin_theta_o * cos_2k_alpha[1] - cos_theta_o * sin_2k_alpha[1];
  cos_theta_o_crts[0] =
      cos_theta_o * cos_2k_alpha[1] + sin_theta_o * sin_2k_alpha[1];

  sin_theta_o_crts[1] =
      sin_theta_o * cos_2k_alpha[0] + cos_theta_o * sin_2k_alpha[0];
  cos_theta_o_crts[1] =
      cos_theta_o * cos_2k_alpha[0] - sin_theta_o * sin_2k_alpha[0];

  sin_theta_o_crts[2] =
      sin_theta_o * cos_2k_alpha[2] + cos_theta_o * sin_2k_alpha[2];
  cos_theta_o_crts[2] =
      cos_theta_o * cos_2k_alpha[2] - sin_theta_o * sin_2k_alpha[2];

  sin_theta_o_crts[3] = sin_theta_o;
  cos_theta_o_crts[3] = cos_theta_o;

  const float phi_o = MY_ATAN2(omega_out[2], omega_out[1]);

  const float sin_theta_i = omega_in[0];
  const float cos_theta_i = SafeSqrtf(1.0f - Sqr(sin_theta_i));

  const float phi_i = MY_ATAN2(omega_in[2], omega_in[1]);

  const float sin_theta_t = sin_theta_o / eta;
  const float cos_theta_t = SafeSqrtf(1.f - Sqr(sin_theta_t));

  const float etap = std::sqrt(eta * eta - Sqr(sin_theta_o)) / cos_theta_o;
  const float sin_gamma_t = h / etap;
  const float cos_gamma_t = SafeSqrtf(1.0f - Sqr(sin_gamma_t));
  const float gamma_t     = SafeASin(sin_gamma_t);

  const float l = transparent_scale * 2.0f * cos_gamma_t / cos_theta_t;

  const float3 T = float3(MY_EXP(-sigma_a[0] * l), MY_EXP(-sigma_a[1] * l),
                          MY_EXP(-sigma_a[2] * l));

  const float gamma_o = SafeASin(h);

  const float phi = phi_i - phi_o;

  std::array<float3, 4> ap = Ap(cos_theta_o, eta, h, T);

  std::array<float, 4> apPdf;
  const float sum = std::accumulate(
      ap.begin(), ap.end(), 0.0f,
      [](const float s_, const float3 &ap_) { return s_ + RgbToY(ap_); });

  for (size_t i = 0; i < 4; i++) apPdf[i] = RgbToY(ap[i]) / sum;

  std::array<float, 4> pdfs;
  float3 ret(0.0f);
  for (size_t p = 0; p < 3; p++) {
    const float mpnp = Mp(sin_theta_i, cos_theta_i, sin_theta_o_crts[p],
                          cos_theta_o_crts[p], v[p]) *
                       Np(phi, int(p), s, gamma_o, gamma_t);
    pdfs[p] = mpnp * apPdf[p];

    ret += mpnp * ap[p] * tints[p];
  }

  const float mpnp =
      Mp(sin_theta_i, cos_theta_i, sin_theta_o, cos_theta_o, v[3]) *
      (1.0f / (2.0f * kPi)) * tints[3];
  pdfs[3] = mpnp * apPdf[3];

  ret += mpnp * ap[3] * tints[3];

  if ((!std::isfinite(ret[0])) || (!std::isfinite(ret[1])) ||
      (!std::isfinite(ret[2]))) {
    return float3(0.0f);
  }

  *pdf = std::accumulate(pdfs.begin(), pdfs.end(), 0.0f);

  if (!std::isfinite((*pdf))) {
    (*pdf) = 0.0f;
    return float3(0.0f);
  }

  return ret;
}

inline float SampleTrimmedLogistic(const float s, const float a, const float b,
                                   const float u) {
  const float T = LogisticCDF(b, s) - LogisticCDF(a, s);
  float ret =
      -s * MY_LOG(1.0f / (u * T + 1.0f / (1.0f + MY_EXP(-a / s))) - 1.0f);

  if (std::isinf(ret)) {
    if (std::isfinite(ret)) ret = b;  // when u neary equal 1
  }
  return ret;
}

inline float3 EnergyConservingHairSample(
    const float3 &omega_out, const float h, const std::array<float, 4> &v,
    const float s, const float sigma_a, const float eta,
    const float alpha /*radian*/, const std::array<float, 4> &tints,
    const float transparent_scale, const std::array<float, 4> us,
    float3 *omega_in, float *pdf) {
  (*pdf) = 0.0f;

  const float sin_theta_o = omega_out[0];
  const float cos_theta_o = SafeSqrtf(1.0f - Sqr(sin_theta_o));

  float sin_2k_alpha[3];
  float cos_2k_alpha[3];

#if USE_FAST_MATH
  fast_math::FastSincos(alpha, &sin_2k_alpha[0], &cos_2k_alpha[0]);
#else
  sin_2k_alpha[0] = fast_sin(alpha);
  cos_2k_alpha[0] = fast_cos(alpha);
#endif

  for (int i = 1; i < 3; i++) {
    sin_2k_alpha[i] = 2.0f * sin_2k_alpha[i - 1] * cos_2k_alpha[i - 1];
    cos_2k_alpha[i] = Sqr(cos_2k_alpha[i - 1]) - Sqr(sin_2k_alpha[i - 1]);
  }

  float sin_theta_o_crts[4], cos_theta_o_crts[4];

  sin_theta_o_crts[0] =
      sin_theta_o * cos_2k_alpha[1] - cos_theta_o * sin_2k_alpha[1];
  cos_theta_o_crts[0] =
      cos_theta_o * cos_2k_alpha[1] + sin_theta_o * sin_2k_alpha[1];

  sin_theta_o_crts[1] =
      sin_theta_o * cos_2k_alpha[0] + cos_theta_o * sin_2k_alpha[0];
  cos_theta_o_crts[1] =
      cos_theta_o * cos_2k_alpha[0] - sin_theta_o * sin_2k_alpha[0];

  sin_theta_o_crts[2] =
      sin_theta_o * cos_2k_alpha[2] + cos_theta_o * sin_2k_alpha[2];
  cos_theta_o_crts[2] =
      cos_theta_o * cos_2k_alpha[2] - sin_theta_o * sin_2k_alpha[2];

  sin_theta_o_crts[3] = sin_theta_o;
  cos_theta_o_crts[3] = cos_theta_o;

  const float phi_o = MY_ATAN2(omega_out[2], omega_out[1]);

  const float sin_theta_t = sin_theta_o / eta;
  const float cos_theta_t = SafeSqrtf(1.0f - Sqr(sin_theta_t));

  const float etap = std::sqrt(eta * eta - Sqr(sin_theta_o)) / cos_theta_o;
  const float sin_gamma_t = h / etap;
  const float cos_gamma_t = SafeSqrtf(1.0f - Sqr(sin_gamma_t));
  const float gamma_t     = SafeASin(sin_gamma_t);

  const float l = transparent_scale * 2.0f * cos_gamma_t / cos_theta_t;

  const float3 T = float3(MY_EXP(-sigma_a[0] * l), MY_EXP(-sigma_a[1] * l),
                          MY_EXP(-sigma_a[2] * l));

  const float gamma_o = SafeASin(h);

  std::array<float3, 4> ap = Ap(cos_theta_o, eta, h, T);

  std::array<float, 4> apPdf;

  const float sum = std::accumulate(
      ap.begin(), ap.end(), 0.0f,
      [](const float s_, const float3 &ap_) { return s_ + RgbToY(ap_); });

  for (size_t i = 0; i < 4; i++) apPdf[i] = RgbToY(ap[i]) / sum;

  size_t p = 0;
  float u0 = us[0];
  for (p = 0; p < 4 - 1; p++) {
    if (u0 < apPdf[p]) break;
    u0 -= apPdf[p];
  }

  float sin_theta_i, cos_theta_i;
  {
    // const float u1 = std::max(us[1],1e-5f),u2 = us[2];
    const float u1 = us[1], u2 = us[2];

    const float u =
        1.0f + v[p] * MY_LOG(u1 + (1.0f - u1) * MY_EXP(-2.0f / v[p]));

    sin_theta_i = -u * sin_theta_o_crts[p] + SafeSqrtf(1.0f - Sqr(u)) *
                                                 MY_COS(2.0f * kPi * u2) *
                                                 cos_theta_o_crts[p];
    cos_theta_i = SafeSqrtf(1.0f - Sqr(sin_theta_i));
  }

  float dphi;
  if (p < 3) {
    dphi = Phi(int(p), gamma_o, gamma_t) +
           SampleTrimmedLogistic(s, -kPi, kPi, us[3]);
  } else {
    dphi = 2.0f * kPi * us[3];
  }

  const float phi_i = phi_o + dphi;

  *omega_in = float3(sin_theta_i, cos_theta_i * MY_COS(phi_i),
                     cos_theta_i * MY_SIN(phi_i));

  for (int i = 0; i < 3; i++) {
    if (!std::isfinite((*omega_in)[i])) {
      std::cerr << "warnig sample omega: ";
      for (int j = 0; j < 3; j++) {
        std::cerr << (*omega_in)[j] << " ";
      }
      std::cerr << std::endl;

      std::cerr << "phi " << SampleTrimmedLogistic(s, -kPi, kPi, us[3])
                << std::endl;
      std::cerr << "sin " << sin_theta_i << "  cos " << cos_theta_i
                << std::endl;
      break;
    }
  }

  std::array<float, 4> pdfs;
  float3 ret(0.0f);
  for (size_t q = 0; q < 3; q++) {
    const float mpnp = Mp(sin_theta_i, cos_theta_i, sin_theta_o_crts[q],
                          cos_theta_o_crts[q], v[q]) *
                       Np(dphi, int(q), s, gamma_o, gamma_t);
    pdfs[q] = mpnp * apPdf[q];
    ret += mpnp * ap[q] * tints[q];
  }

  const float mpnp =
      Mp(sin_theta_i, cos_theta_i, sin_theta_o, cos_theta_o, v[3]) *
      (1.0f / (2.0f * kPi));
  pdfs[3] = mpnp * apPdf[3];

  ret += mpnp * ap[3] * tints[3];

  if ((!std::isfinite(ret[0])) || (!std::isfinite(ret[1])) ||
      (!std::isfinite(ret[2]))) {
    return float3(0.0f);
  }

  *pdf = std::accumulate(pdfs.begin(), pdfs.end(), 0.0f);

  if (!std::isfinite((*pdf))) {
    (*pdf) = 0.0f;
    return float3(0.0f);
  }

  return ret;
}

}  // namespace hair_bsdf
}  // namespace pbrlab
#endif  // PBRLAB_ENERGY_CONSERVING_HAIR_BSDF_H_
