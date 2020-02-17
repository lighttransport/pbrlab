/*
 * Adapted from Open Shading Language with this license:
 *
 * Copyright (c) 2009-2010 Sony Pictures Imageworks Inc., et al.
 * All Rights Reserved.
 *
 * Modifications Copyright 2011, Blender Foundation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * * Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * * Neither the name of Sony Pictures Imageworks nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* GGX microfacet importance sampling from:
 *
 * Importance Sampling Microfacet-Based BSDFs using the Distribution of Visible
 * Normals. E. Heitz and E. d'Eon, EGSR 2014
 */

#ifndef PBRLAB_MICROFACET_GGX_H_
#define PBRLAB_MICROFACET_GGX_H_

#include "closure/closure-util.h"
#include "pbrlab_math.h"
#include "type.h"

namespace pbrlab {

inline float D_GTR1(const float3 h, const float alpha) {
  if (alpha >= 1.0f) return 1.0f / kPi;
  const float alpha2 = alpha * alpha;
  const float t      = 1.0f + (alpha2 - 1.0f) * h[2] * h[2];
  return (alpha2 - 1.0f) / (kPi * logf(alpha2) * t);
}

inline float D_GTR2(const float3& h, const float alpha2) {
  const float& cos_theta_m = h[2];  // n dot m
  const float cos_theta_m2 = cos_theta_m * cos_theta_m;
  const float cos_theta_m4 = cos_theta_m2 * cos_theta_m2;
  const float tan_theta_m2 = (1.0f - cos_theta_m2) / cos_theta_m2;

  return alpha2 / (kPi * cos_theta_m4 * (alpha2 + tan_theta_m2) *
                   (alpha2 + tan_theta_m2));
}

inline void MicrofacetGgxSampleSlopes(const float cos_theta_i,
                                      const float sin_theta_i,
                                      const float randu, float randv,
                                      float* slope_x, float* slope_y,
                                      float* G1i) {
  const float k2PI = 2.0f * kPi;

  /* special case (normal incidence) */
  if (cos_theta_i >= 0.99999f) {
    const float r   = sqrtf(randu / (1.0f - randu));
    const float phi = k2PI * randv;
    *slope_x        = r * cosf(phi);
    *slope_y        = r * sinf(phi);
    *G1i            = 1.0f;

    return;
  }

  /* precomputations */
  const float tan_theta_i = sin_theta_i / cos_theta_i;
  const float G1_inv =
      0.5f * (1.0f + SafeSqrtf(1.0f + tan_theta_i * tan_theta_i));

  *G1i = 1.0f / G1_inv;

  /* sample slope_x */
  const float A         = 2.0f * randu * G1_inv - 1.0f;
  const float AA        = A * A;
  const float tmp       = 1.0f / (AA - 1.0f);
  const float B         = tan_theta_i;
  const float BB        = B * B;
  const float D         = SafeSqrtf(BB * (tmp * tmp) - (AA - BB) * tmp);
  const float slope_x_1 = B * tmp - D;
  const float slope_x_2 = B * tmp + D;
  *slope_x =
      (A < 0.0f || slope_x_2 * tan_theta_i > 1.0f) ? slope_x_1 : slope_x_2;

  /* sample slope_y */
  float S;

  if (randv > 0.5f) {
    S     = 1.0f;
    randv = 2.0f * (randv - 0.5f);
  } else {
    S     = -1.0f;
    randv = 2.0f * (0.5f - randv);
  }

  const float z =
      (randv * (randv * (randv * 0.27385f - 0.73369f) + 0.46341f)) /
      (randv * (randv * (randv * 0.093073f + 0.309420f) - 1.000000f) +
       0.597999f);
  *slope_y = S * z * SafeSqrtf(1.0f + (*slope_x) * (*slope_x));
}

/* return half vector*/
inline float3 MicrofacetSampleStretched(const float3 omega_i,
                                        const float alpha_x,
                                        const float alpha_y, const float randu,
                                        const float randv, float* G1i) {
  /* 1. stretch omega_i */
  const float3 omega_i_ = vnormalized(
      float3(alpha_x * omega_i[0], alpha_y * omega_i[1], omega_i[2]));

  /* get polar coordinates of omega_i_ */
  float costheta_ = 1.0f;
  float sintheta_ = 0.0f;
  float cosphi_   = 1.0f;
  float sinphi_   = 0.0f;

  if (omega_i_[2] < 0.99999f) {
    costheta_ = omega_i_[2];
    sintheta_ = SafeSqrtf(1.0f - costheta_ * costheta_);

    float invlen = 1.0f / sintheta_;
    cosphi_      = omega_i_[0] * invlen;
    sinphi_      = omega_i_[1] * invlen;
  }

  /* 2. sample P22_{omega_i}(x_slope, y_slope, 1, 1) */

  // TODO beckmann
  float slope_x = 0.f, slope_y = 0.f;
  MicrofacetGgxSampleSlopes(costheta_, sintheta_, randu, randv, &slope_x,
                            &slope_y, G1i);

  /* 3. rotate */
  float tmp = cosphi_ * slope_x - sinphi_ * slope_y;
  slope_y   = sinphi_ * slope_x + cosphi_ * slope_y;
  slope_x   = tmp;

  /* 4. unstretch */
  slope_x = alpha_x * slope_x;
  slope_y = alpha_y * slope_y;

  /* 5. compute normal */
  return vnormalize(float3(-slope_x, -slope_y, 1.0f));
}

inline float MicrofacetGGXBsdfPdf(const float3& omega_in,
                                  const float3& omega_out, const float alpha_x,
                                  const float alpha_y, const int distrib,
                                  float* pdf) {
  // n is ez

  const float cos_n_o = omega_out[2];  // n dot omega_out
  const float cos_n_i = omega_in[2];   // n dot omega_in

  if (cos_n_o > 0 && cos_n_i > 0) {  // reflection
    /* half vector*/                 // TODO rename
    const float3 m     = vnormalized(omega_in + omega_out);
    const float alpha2 = alpha_x * alpha_y;
    float D = 0.f, G1o = 0.f, G1i = 0.f;

    if (std::fabs(alpha_x - alpha_y) <
        std::numeric_limits<float>::epsilon()) {  // isotropic
      /*
       * eq. 20: (F*G*D)/(4*in*on)
       * eq. 33: first we calculate D(m) */
      if (distrib == 1) {  // GTR
        D = D_GTR1(m, alpha_x);
      } else {  // GTR2
        D = D_GTR2(m, alpha2);
      }
      /* eq. 34: now calculate G1(i,m) and G1(o,m) */
      G1o = 2 / (1 + SafeSqrtf(1 + alpha2 * (1 - cos_n_o * cos_n_o) /
                                       (cos_n_o * cos_n_o)));
      G1i = 2 / (1 + SafeSqrtf(1 + alpha2 * (1 - cos_n_i * cos_n_i) /
                                       (cos_n_i * cos_n_i)));
    } else {
      /* anisotropic */
      // tanget -> ex

      const float slope_x   = -m[0] / (m[2] * alpha_x);
      const float slope_y   = -m[1] / (m[2] * alpha_y);
      const float slope_len = 1 + slope_x * slope_x + slope_y * slope_y;

      const float cosThetaM  = m[2];
      const float cosThetaM2 = cosThetaM * cosThetaM;
      const float cosThetaM4 = cosThetaM2 * cosThetaM2;

      D = 1.f / ((slope_len * slope_len) * kPi * alpha2 * cosThetaM4);

      /* G1(i,m) and G1(o,m) */
      const float tanThetaO2 = (1.f - cos_n_o * cos_n_o) / (cos_n_o * cos_n_o);
      const float cosPhiO    = omega_out[0];  // ex dot omega_out
      const float sinPhiO    = omega_out[1];  // ey dot omega_out

      float alphaO2 = (cosPhiO * cosPhiO) * (alpha_x * alpha_x) +
                      (sinPhiO * sinPhiO) * (alpha_y * alpha_y);
      alphaO2 /= cosPhiO * cosPhiO + sinPhiO * sinPhiO;

      G1o = 2 / (1 + SafeSqrtf(1 + alphaO2 * tanThetaO2));

      const float tanThetaI2 = (1 - cos_n_i * cos_n_i) / (cos_n_i * cos_n_i);
      const float cosPhiI    = omega_in[0];  // ex dot omega_in
      const float sinPhiI    = omega_in[1];  // ey dot omega_in

      float alphaI2 = (cosPhiI * cosPhiI) * (alpha_x * alpha_x) +
                      (sinPhiI * sinPhiI) * (alpha_y * alpha_y);
      alphaI2 /= cosPhiI * cosPhiI + sinPhiI * sinPhiI;

      G1i = 2 / (1 + SafeSqrtf(1 + alphaI2 * tanThetaI2));
    }
    const float G = G1o * G1i;

    /* eq. 20 */
    const float common = D * 0.25f / cos_n_o / cos_n_i;

    float bsdf_f = G * common;
    if (distrib == 1) bsdf_f = 0.25f * bsdf_f;

    *pdf = G1o * common;

    return bsdf_f;
  }
  // TODO transmittion
  *pdf = 0.f;
  return 0.f;
}

inline float MicrofacetGGXSample(const float3& omega_out, const float alpha_x,
                                 const float alpha_y,
                                 const std::array<float, 2>& u,
                                 const bool refractive, const int distrib,
                                 float3* omega_in, float* pdf) {
  const float cos_n_o = omega_out[2];  // n dot omega_out

  float ret_bsdf_f = 0.f;

  // It is possible to sample only when cos_n_o > 0
  if (cos_n_o > 0.f) {
    // tanget -> ex
    float G1o      = 0.f;
    const float3 m = MicrofacetSampleStretched(omega_out, alpha_x, alpha_y,
                                               u[0], u[1], &G1o);

    bool tir_flag = false;
    if (refractive) {
      // TODO
    }
    if (!refractive || tir_flag) {
      const float& cos_m_o = vdot(m, omega_out);
      if (cos_m_o > 0) {
        /* eq. 39 - compute actual reflected direction */
        *omega_in = 2 * cos_m_o * m - omega_out;
        // TODO calculation without G1o. The performance improve?
        (void)G1o;
        ret_bsdf_f = MicrofacetGGXBsdfPdf(*omega_in, omega_out, alpha_x,
                                          alpha_y, distrib, pdf);
      } else {
        // error
        // TODO
      }
    }
  } else {
    assert(false);
  }

  return ret_bsdf_f;
}
}  // namespace pbrlab

#endif  // PBRLAB_MICROFACET_GGX_H_
