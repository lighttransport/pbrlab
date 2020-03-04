#ifndef PBRLAB_RANDOM_WALK_SSS_H_
#define PBRLAB_RANDOM_WALK_SSS_H_

#include "closure/lambert.h"
#include "matrix.h"
#include "pbrlab-util.h"
#include "pbrlab_math.h"
#include "scene.h"
#include "shader/shader.h"
#include "type.h"

namespace pbrlab {
namespace random_walk_sss {

/*
 * Copyright 2011-2013 Blender Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* Approximate Reflectance Profiles
 * http://graphics.pixar.com/library/ApproxBSSRDF/paper.pdf
 */

inline float BssrdfBurleyFitting(float A) {
  /* Diffuse surface transmission, equation (6). */
  return 1.9f - A + 3.5f * (A - 0.8f) * (A - 0.8f);
}

inline float BssrdfBurleyFitting5(float A) {
  /* Diffuse surface transmission, equation (5). */
  return 1.85f - A + 7.0f * std::fabs((A - 0.8f) * (A - 0.8f) * (A - 0.8f));
}

/* Scale mean free path length so it gives similar looking result
 * to Cubic and Gaussian models.
 */
inline float3 BssrdfBurleyCompatibleMfp(float3 r) {
  return 0.25f * (1.0f / kPi) * r;
}

inline void BssrdfBurleySetup(const float3& albedo, const float3& radius,
                              const bool scale_mfp, const int mode,
                              float3* radius_out) {
  /* Mean free path length. */
  const float3 l = scale_mfp ? BssrdfBurleyCompatibleMfp(radius) : radius;
  /* Surface albedo. */
  const float3 A = albedo;
  float3 s;
  if (mode == 0) {  // eq6
    s = float3(BssrdfBurleyFitting(A[0]), BssrdfBurleyFitting(A[1]),
               BssrdfBurleyFitting(A[2]));
  } else {  // eq5
    s = float3(BssrdfBurleyFitting5(A[0]), BssrdfBurleyFitting5(A[1]),
               BssrdfBurleyFitting5(A[2]));
  }

  (*radius_out) = l / s;
}

inline void BssrdfSetup(const bool burey_radius, const bool scale_mfp,
                        const bool use_eq5, float3* weight, float3* albedo,
                        float3* radius, float3* diffuse_weight) {
  (*diffuse_weight) = float3(0.0f, 0.0f, 0.0f);

  const float kBssrdfMinRadius(1e-8f);
  float3 kd(0.0f);

  int bssrdf_channels = 3;

  // Turn BSSRDF into BSDF(diffuse) when the scattering radius is too small.
  for (int i = 0; i < 3; ++i) {
    if ((*radius)[i] < kBssrdfMinRadius) {
      kd[i]        = (*weight)[i];
      (*weight)[i] = 0.f;
      (*radius)[i] = 0.f;
      bssrdf_channels--;
    }
  }

  if (bssrdf_channels < 3) {
    // Create diffuse BSDF
    // Currently we only returns diffuse weight.
    (*diffuse_weight) = kd;
  }

  if (bssrdf_channels > 0) {
    if (burey_radius) {
      float3 updated_radius;
      BssrdfBurleySetup(*albedo, *radius, scale_mfp, use_eq5, &updated_radius);
      *radius = updated_radius;
    }
  }
}

//
// "Practical and Controllable Subsurface Scattering for Production Path
//  Tracing". Matt Jen-Yuan Chiang, Peter Kutz, Brent Burley. SIGGRAPH 2016.
//

inline void ComputeScatteringCoefficientFromAlbedo(const float A, const float d,
                                                   float* sigma_t,
                                                   float* sigma_s) {
  /* Compute attenuation and scattering coefficients from albedo. */
  const float a =
      1.0f - std::exp(A * (-5.09406f + A * (2.61188f - A * 4.31805f)));
  const float s = 1.9f - A + 3.5f * Sqr(A - 0.8f);

  *sigma_t = 1.0f / std::max(d * s, 1e-16f);
  *sigma_s = *sigma_t * a;
}

inline void ComputeScatteringCoefficient(const float3& weight,
                                         const float3& albedo,
                                         const float3& radius, float3* sigma_t,
                                         float3* sigma_s, float3* throughput) {
  // For each channel
  ComputeScatteringCoefficientFromAlbedo(albedo[0], radius[0], &((*sigma_t)[0]),
                                         &((*sigma_s)[0]));
  ComputeScatteringCoefficientFromAlbedo(albedo[1], radius[1], &((*sigma_t)[1]),
                                         &((*sigma_s)[1]));
  ComputeScatteringCoefficientFromAlbedo(albedo[2], radius[2], &((*sigma_t)[2]),
                                         &((*sigma_s)[2]));

  (*throughput) = SafeDivideSpectrum(weight, albedo);
}

//
// r  = uniform random number [0, 1)
//
inline int SampleChannel(const float3& albedo, const float3& throughput,
                         const float r, float3* pdf) {
  /* Sample color channel proportional to throughput and single scattering
   * albedo, to significantly reduce noise with many bounce, following:
   *
   * "Practical and Controllable Subsurface Scattering for Production Path
   *  Tracing". Matt Jen-Yuan Chiang, Peter Kutz, Brent Burley. SIGGRAPH 2016.
   */

  float3 weights;
  weights[0] = std::fabs(throughput[0] * albedo[0]);
  weights[1] = std::fabs(throughput[1] * albedo[1]);
  weights[2] = std::fabs(throughput[2] * albedo[2]);

  float sum_weights = weights[0] + weights[1] + weights[2];

  if (sum_weights > 0.0f) {
    (*pdf)[0] = weights[0] / sum_weights;
    (*pdf)[1] = weights[1] / sum_weights;
    (*pdf)[2] = weights[2] / sum_weights;
  } else {
    (*pdf) = float3(1.0f / 3.0f, 1.0f / 3.0f, 1.0f / 3.0f);
  }

  if (r < pdf->x()) {
    return 0;
  } else if (r < pdf->x() + pdf->y()) {
    return 1;
  } else {
    return 2;
  }
}

inline float SampleScatterDistance(const float3 throughput,
                                   const float3 sigma_s, const float3 sigma_t,
                                   const std::array<float, 2>& u,
                                   float3* channel_pdf) {
  float3 albedo = SafeDivideSpectrum(sigma_s, sigma_t);

  int channel = SampleChannel(albedo, throughput, u[0], channel_pdf);
  const float sample_sigma_t = sigma_t[channel];

  float distance = -logf(1.0f - u[1]) / sample_sigma_t;
  assert(std::isfinite(distance));

  return distance;
}

inline float3 AttenuateTransmission(const float3& sigma_t, float distance) {
  float3 tau;

  tau[0] = std::exp(-sigma_t[0] * distance);
  tau[1] = std::exp(-sigma_t[1] * distance);
  tau[2] = std::exp(-sigma_t[2] * distance);

  return tau;
}

///
/// Do random walk path tracing for subsurface scattering
/// Terminate the ray when the scattered ray hits the surface or absorbed in
/// the medium.
///
///
///           N
///           |\
///           |
///         P |           out
///   --------*-------------o-------------
///            \            |
///             \           |
///              \       /\/
///               o-----o
///
/**
 *@param[in] scene scene
 *@param[in] weight subsurface weight
 *@param[in] albedo subsurface albedo
 *@param[in] radius subsurface radius
 *@param[in] rng random generator
 *@param[in,out] surface_info surface information
 *@param[in,out] Rgl Rotation matrix global to local
 *@param[out] new_omega_out new omega out
 *@param[out] throughput_out local throughput
 *@param[out] scatter_bounces scatter bounce
 */
inline bool RandomWalkSubsurface(const Scene& scene,
                                 const float3& weight,  // subsurface weight
                                 const float3& albedo,  // subsurface color
                                 const float3& radius,  // subsurface radius
                                 const RNG& rng, SurfaceInfo* surface_info,
                                 float Rgl[4][4], float3* new_omega_out,
                                 float3* throughput_out, int* scatter_bounces) {
  (*scatter_bounces) = 0;

  if (surface_info->face_direction != SurfaceInfo::kFront) {
    // TODO
    // assert(false);
    return false;
  }

  float Rlg[4][4];
  Matrix::Copy(Rgl, Rlg);
  for (size_t i = 0; i < 4; ++i) {
    for (size_t j = 0; j < 4; ++j) {
      Rlg[i][j] = Rgl[j][i];
    }
  }

  // 1. Sample diffuse surface scatter into the object
  float3 global_dir;
  {
    float3 tmp;
    float pdf              = 0.0f;
    std::array<float, 2> u = {rng.Draw(), rng.Draw()};
    LambertBrdfSample(float3(0.f, 0.f, 1.f), u, &tmp, &pdf);
    tmp = -tmp;

    Matrix::MultV(tmp.v, Rlg, global_dir.v);
    if (vdot(-surface_info->normal_g, global_dir) <= 0.0f) {
      return false;
    }
  }

  // 2. Compute volume scattering coefficients from apparent subsurface color
  // and radius
  //    (albedo inversion)
  float3 sigma_t, sigma_s;
  float3 throughput(1.0f);

  ComputeScatteringCoefficient(weight, albedo, radius, &sigma_t, &sigma_s,
                               &throughput);

  Ray ray;
  ray.ray_org = surface_info->global_position;
  ray.ray_dir = global_dir;
  ray.min_t   = 1e-3f;
  ray.max_t   = kInf;

  const uint32_t max_subsuface_bounces = 128;

  TraceResult trace_result;
  bool hit = false;  // TODO

  // 3. Random walk until we hit the surface again.
  for (uint32_t bounce = 0; bounce <= max_subsuface_bounces; ++bounce) {
    if (bounce > 0) {
      // Sample scattering direction.
      float3 wi;
      float direction_pdf;  // not used
      if (false) {
        // TODO HenyeyGreenstein
      } else {
        // Isotropic Scatter
        wi            = UniformSampleSphere(rng.Draw(), rng.Draw());
        direction_pdf = UniformSampleSpherePdf();
      }

      wi = vnormalize(wi);

      // Update ray direction.
      ray.ray_dir[0] = wi[0];
      ray.ray_dir[1] = wi[1];
      ray.ray_dir[2] = wi[2];

      ray.min_t = 0.f;
    }

    // Distance sampling(includes sample color channel with MIS)
    float3 channel_pdf;
    std::array<float, 2> u_ = {rng.Draw(), rng.Draw()};
    float t_scatter =
        SampleScatterDistance(throughput, sigma_s, sigma_t, u_, &channel_pdf);

    ray.max_t = t_scatter;  // Limit trace distance.

    trace_result = scene.TraceFirstHit1(ray);  // TODO limit instance id

    hit = (trace_result.instance_id != static_cast<uint32_t>(-1));

    const float t = hit ? trace_result.t : t_scatter;

    // Update throughput
    float3 transmittance = AttenuateTransmission(sigma_t, t);

    // TODO normal check
    // if (hit) {
    //   if (vdot(ray.ray_dir, float3(trace_result.normal_g)) < 0.0f) {
    //     hit = false;
    //     break;
    //   }
    // }

    if (hit) {
      // surface hit
      const float pdf = vdot(channel_pdf, transmittance);
      throughput      = throughput * transmittance / pdf;

      // We've reached surface again!
      break;

    } else {
      // scatter
      float pdf  = vdot(channel_pdf, sigma_t * transmittance);
      throughput = throughput * (sigma_s * transmittance) / pdf;
    }

    // Russian roulette
    {
      const float p = Saturate(SpectrumNorm(throughput));
      const float q = rng.Draw();
      if (q >= p) {
        break;
      }

      throughput = throughput / p;
    }

    // Advance to new scatter(or surface hit) location.
    ray.ray_org = ray.ray_org + t * ray.ray_dir;

    (*scatter_bounces)++;
  }  // bounces

  if (!hit) {
    // Ray was absorbed or reached the maximum number of scattering bounces.
    return false;
  }

  const uint32_t prev_instance_id = surface_info->instance_id;

  *surface_info = TraceResultToSufaceInfo(ray, scene, trace_result);

  // TODO filtering with ray intersector
  if (surface_info->instance_id != prev_instance_id) {
    return false;
  }

  if (surface_info->face_direction != SurfaceInfo::kBack) {
    // TODO
    // assert(surface_info->face_direction != SurfaceInfo::kAmbiguous);
    return false;
  }

  {  // Update Rgl
    // TODO tangent, binormal
    const float3 ez = surface_info->normal_s;
    float3 ex, ey;
    BranchlessONB(ez, &ex, &ey);

#ifdef NDEBUG
#else
    assert(CheckONB(ex, ey, ez));
#endif

    GrobalToShadingLocal(ex, ey, ez, Rgl);
  }

  Matrix::MultV(ray.ray_dir.v, Rgl, new_omega_out->v);

  (*throughput_out) = throughput;

  return true;
}

}  // namespace random_walk_sss
}  // namespace pbrlab
#endif  // PBRLAB_RANDOM_WALK_SSS_H_
