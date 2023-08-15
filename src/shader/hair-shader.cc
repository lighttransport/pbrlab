#include "shader/hair-shader.h"

#include "closure/energy‐conserving-hair-bsdf.h"
#include "material-param.h"

namespace pbrlab {

struct HairBsdf {
  float3 sigma_a;
  float h;
  std::array<float, 4> v;
  float s;
  float eta;
  float alpha;
  std::array<float3, 4> tints;
  float transparent_scale;
};

static std::array<float, 4> BetamToV(const float beta_m) {
  std::array<float, 4> vs;
  vs[0] = Sqr(0.726f * beta_m + 0.812f * Sqr(beta_m) + 3.7f * Pow<20>(beta_m));
  vs[1] = 0.25f * vs[0];
  vs[2] = 4.0f * vs[0];
  vs[3] = vs[2];

  return vs;
}

static float CalcS(const float beta_n) {
  const float beta_n2 = Sqr(beta_n);
  return std::sqrt(kPi / 8.0f) *
         (0.265f * beta_n + 1.194f * beta_n2 + 5.372f * Pow<11>(beta_n2));
}

static float3 CalcSigmaAFromRGB(const float3& c, const float beta_n) {
  float3 ret;

  for (int i = 0; i < 3; i++) {
    ret[i] =
        Sqr(MY_LOG(c[i]) / (5.969f - 0.215f * beta_n + 2.532f * Sqr(beta_n) -
                            10.73f * Pow<3>(beta_n) + 5.574f * Pow<4>(beta_n) +
                            0.245f * Pow<5>(beta_n)));
  }

  return ret;
}

static float3 CalcSigmaAUsingMelaninParameter(float melanin,
                                              float melaninredness,
                                              const float random_value = 0.5f) {
  const float factor_random_value = 1.f + 2.f * (random_value - 0.5f);  // TODO

  melanin        = Clamp(melanin, 0.0f, 1.0f) * factor_random_value;
  melaninredness = Clamp(melaninredness, 0.0f, 1.0f);

  melanin = -MY_LOG(std::max(1.0f - melanin, 0.0001f));

  const float eumelanin   = melanin * (1.0f - melaninredness);
  const float pheomelanin = melanin * melaninredness;

  return float3(std::max(0.0f, eumelanin * 0.506f + pheomelanin * 0.343f),
                std::max(0.0f, eumelanin * 0.841f + pheomelanin * 0.733f),
                std::max(0.0f, eumelanin * 1.653f + pheomelanin * 1.924f));
}

#if 0  // TODO
static float3 CalcSigmaAFromConcentration(const float eumelanin,
                                          const float pheomelanin) {
  float3 eumelaninSigmaA(0.419f, 0.697f, 1.37f);
  float3 pheomelaninSigmaA(0.187f, 0.4f, 1.05f);

  return eumelanin * eumelaninSigmaA + pheomelanin * pheomelaninSigmaA;
}

static float3 CalcSigmaAUsingMelaninParameterRandom(
    float melanin, float melaninredness, const float3& tint, const float beta_n,
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

#endif

static HairBsdf ParamToBsdf(const HairBsdfParameter& m_param,
                            const float geom_v /* [0,1] */) {
  // parameters
  // coloring_hair         kRGB or kMelanin
  // base_color            [0,1]^3
  //
  // melanin               [0,1]
  // melanin_redness       [0,1]
  // melanin_randomize     [0,1]
  //
  // roughness             [0,1]
  // azimuthal_roughness   [0,1]
  //
  // ior                   float
  //
  // shift                 [0,360) deg
  //
  // specular_tint         [0,1]^3
  // second_specular_tint  [0,1]^3
  // transmission_tint     [0,1]^3

  HairBsdf bsdf;

  if (m_param.coloring_hair == HairBsdfParameter::kRGB) {
    bsdf.sigma_a =
        CalcSigmaAFromRGB(m_param.base_color, m_param.azimuthal_roughness);
  } else if (m_param.coloring_hair == HairBsdfParameter::kMelanin) {
    // TODO randamize
    bsdf.sigma_a = CalcSigmaAUsingMelaninParameter(m_param.melanin,
                                                   m_param.melanin_redness);
  } else {
    assert(false);
  }

  bsdf.h = geom_v;

  bsdf.v = BetamToV(m_param.roughness);
  bsdf.s = CalcS(m_param.azimuthal_roughness);

  bsdf.eta = m_param.ior;

  bsdf.alpha = m_param.shift * kPi / 180.f;  // To radian

  bsdf.tints[0] = m_param.specular_tint;
  bsdf.tints[1] = m_param.transmission_tint;
  bsdf.tints[2] = m_param.second_specular_tint;
  bsdf.tints[3] = 1.f;  // TODO

  bsdf.transparent_scale = 1.f;  // TODO

  return bsdf;
}

void HairShader(const Scene& scene, const float3& global_omega_out,
                const RNG& rng, SurfaceInfo* surface_info,
                float3* global_omega_in, float3* throuput, float3* contribute,
                float* pdf) {
  if (surface_info->face_direction == SurfaceInfo::kAmbiguous) {
    *global_omega_in = global_omega_out;
    *throuput        = float3(0.0f);
    *contribute      = float3(0.0f);
    *pdf             = 0.0f;
    return;
  }

  // NOTE : curve mesh treats tangents as normals.
  const float3 ex = surface_info->normal_s;  // tangent
  const float3 ey = vnormalized(vcross(vcross(global_omega_out, ex), ex));
  const float3 ez = vcross(ex, ey);

  assert(CheckONB(ex, ey, ez));

  float Rgl[4][4];  // global to shading local
  GrobalToShadingLocal(ex, ey, ez, Rgl);

  float3 omega_out;
  Matrix::MultV(global_omega_out.v, Rgl, omega_out.v);

  assert(std::fabs(omega_out[2] - vdot(global_omega_out, ez)) < kEps);

  const HairBsdfParameter* m_param =
      mpark::get_if<HairBsdfParameter>(surface_info->material_param);

  const HairBsdf bsdf = ParamToBsdf(*m_param, surface_info->v);

  *contribute = float3(0.f);
  {
    // NOTE : global normal == ex
    const float3 d = DirectIllumination(
        scene, omega_out, *surface_info, Rgl, ex, rng,
        [&bsdf](const float3& omega_in_, const float3& omega_out_,
                float3* bsdf_f_, float* pdf_) {
          float3 bsdf_f_cos_i = hair_bsdf::EnergyConservingHairBsdfCosPdf(
              omega_in_, omega_out_, bsdf.h, bsdf.v, bsdf.s, bsdf.sigma_a,
              bsdf.eta, bsdf.alpha, bsdf.tints, bsdf.transparent_scale, pdf_);

          // Fit the ordinary BSDF coordinate system.
          *bsdf_f_ = bsdf_f_cos_i / std::fabs(omega_in_[0]);
        },
        false);
    (*contribute) = (*contribute) + d;
  }

  // Sample Bsdf
  float3 omega_in(0.f), bsdf_f_cos_i(0.f), _contrib(0.f);
  float ret_pdf = 0.f;
  {
    std::array<float, 4> us = {rng.Draw(), rng.Draw(), rng.Draw(), rng.Draw()};

    bsdf_f_cos_i = hair_bsdf::EnergyConservingHairSample(
        omega_out, bsdf.h, bsdf.v, bsdf.s, bsdf.sigma_a, bsdf.eta, bsdf.alpha,
        bsdf.tints, bsdf.transparent_scale, us, &omega_in, &ret_pdf);
  }

  auto& Rlg = Rgl;
  ShadingLocalToGlobal(ex, ey, ez, Rlg);  // local to global
  Matrix::MultV(omega_in.v, Rlg, global_omega_in->v);

  assert(std::fabs(vdot(*global_omega_in, ez) - omega_in[2]) < kEps);

  *throuput = bsdf_f_cos_i / ret_pdf;
  *pdf      = ret_pdf;  // pdf of bsdf sampling

  assert(*pdf < std::numeric_limits<float>::epsilon() || IsFinite(*throuput));

  if (!IsFinite(*throuput) || !std::isfinite(*pdf)) {
    *throuput = float3(0.f);
    *pdf      = 0.f;
  }
}

}  // namespace pbrlab
