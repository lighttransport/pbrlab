#include "cycles-principled-shader.h"

#include <assert.h>

#include <array>

#include "closure/lambert.h"
#include "closure/microfacet-ggx.h"
#include "material-param.h"
#include "matrix.h"
#include "pbrlab-util.h"
#include "random/rng.h"
#include "sampler/sampling-utils.h"
#include "shader-utils.h"
#include "shader/random-walk-sss.h"
#include "type.h"

namespace pbrlab {

struct CyclesPrincipledBsdf {
  // diffuse or SSS
  bool enable_diffuse   = false;
  float3 diffuse_weight = float3(0.f);

  bool enable_subsurface   = false;
  float3 subsurface_weight = float3(0.f);
  float3 subsurface_albedo = float3(0.f);
  float3 subsurface_radius = float3(0.f);

  // specular
  bool enable_specular   = false;
  float3 specular_weight = float3(0.f);
  float alpha_x          = 1.f;
  float alpha_y          = 1.f;
  float ior              = 1.5f;
  float3 specular_color  = float3(0.f);

  // clearcoat
  bool enable_clearcoat   = false;
  float3 clearcoat_weight = float3(0.f);
  float clearcoat_alpha_x = 1.f;
  float clearcoat_alpha_y = 1.f;
  float clearcoat_ior     = 1.5f;
  float3 clearcoat_color  = float3(0.0f);
};

struct CyclesSampleWeight {
  float diffuse_sample_weight;
  float subsurface_sample_weight;
  float specular_sample_weight;
  float clearcoat_sample_weight;
};

static float3 SpecularColor(const float3& omega_in, const float3& omega_out,
                            const float3& specular_color, const float ior) {
  const float3 h = vnormalized(omega_in + omega_out);
  const float f0 = FresnelDielectricCos(1.0f, ior);
  const float fh =
      (FresnelDielectricCos(vdot(h, omega_out), ior) - f0) / (1.0f - f0);
  return (specular_color) * (1.f - fh) + float3(fh);
}

static CyclesSampleWeight FetchClosureSampleWeight(
    const float3& omega_out, const CyclesPrincipledBsdf& bsdf) {
  CyclesSampleWeight ret;

  ret.diffuse_sample_weight =
      bsdf.enable_diffuse ? RgbToY(bsdf.diffuse_weight) : 0.f;

  ret.subsurface_sample_weight =
      bsdf.enable_subsurface ? RgbToY(bsdf.subsurface_weight) : 0.f;

  ret.specular_sample_weight =
      bsdf.enable_specular
          ? RgbToY(bsdf.specular_weight *
                   SpecularColor(float3(-omega_out[0], -omega_out[1],
                                        omega_out[2]) /*half vector is normal*/,
                                 omega_out, bsdf.specular_color, bsdf.ior))
          : 0.f;

  ret.clearcoat_sample_weight =
      bsdf.enable_clearcoat
          ? RgbToY(bsdf.clearcoat_weight *
                   SpecularColor(float3(-omega_out[0], -omega_out[1],
                                        omega_out[2]) /*half vector is normal*/,
                                 omega_out, bsdf.clearcoat_color,
                                 bsdf.clearcoat_ior))
          : 0.f;

  {  // normalize
    float sum = 0.0f;
    sum += ret.diffuse_sample_weight;
    sum += ret.subsurface_sample_weight;
    sum += ret.specular_sample_weight;
    sum += ret.clearcoat_sample_weight;

    ret.diffuse_sample_weight /= sum;
    ret.subsurface_sample_weight /= sum;
    ret.specular_sample_weight /= sum;
    ret.clearcoat_sample_weight /= sum;

    if (!std::isfinite(ret.diffuse_sample_weight))
      ret.diffuse_sample_weight = 0.f;
    if (!std::isfinite(ret.subsurface_sample_weight))
      ret.subsurface_sample_weight = 0.f;
    if (!std::isfinite(ret.specular_sample_weight))
      ret.specular_sample_weight = 0.f;
    if (!std::isfinite(ret.clearcoat_sample_weight))
      ret.clearcoat_sample_weight = 0.f;
  }
  return ret;
}

static void EvalBsdf(const float3& omega_in, const float3& omega_out,
                     const CyclesPrincipledBsdf& bsdf, float3* bsdf_f,
                     float* pdf) {
  const CyclesSampleWeight w = FetchClosureSampleWeight(omega_out, bsdf);

  *bsdf_f = float3(0.0f);
  *pdf    = 0.0f;

  if (bsdf.enable_diffuse) {  // diffuse
    float _pdf          = 0.f;
    const float _brdf_f = LambertBrdfPdf(omega_in, omega_out, &_pdf);
    *bsdf_f += bsdf.diffuse_weight * _brdf_f;
    *pdf += w.diffuse_sample_weight * _pdf;
  }

  if (bsdf.enable_specular) {  // specular
    float _pdf          = 0.f;
    const float _brdf_f = MicrofacetGGXBsdfPdf(
        omega_in, omega_out, bsdf.alpha_x, bsdf.alpha_y, /*distrib =*/2, &_pdf);

    // Calc fresnel
    *bsdf_f +=
        bsdf.specular_weight *
        SpecularColor(omega_in, omega_out, bsdf.specular_color, bsdf.ior) *
        _brdf_f;
    *pdf += w.specular_sample_weight * _pdf;
  }

  if (bsdf.enable_clearcoat) {  // clearcoat
    float _pdf = 0.f;
    const float _brdf_f =
        MicrofacetGGXBsdfPdf(omega_in, omega_out, bsdf.clearcoat_alpha_x,
                             bsdf.clearcoat_alpha_y, /*distrib =*/1, &_pdf);

    // Calc fresnel
    *bsdf_f += bsdf.clearcoat_weight *
               SpecularColor(omega_in, omega_out, bsdf.clearcoat_color,
                             bsdf.clearcoat_ior) *
               _brdf_f;
    *pdf += w.clearcoat_sample_weight * _pdf;
  }
}

/**
 *@param[in] scene scene
 *@param[in] omega_out omega_out
 *@param[in] bsdf cycles principled bsdf
 *@param[in] rng random generator
 *@param[in,out] surface_info surface information
 *@param[in,out] Rgl Rotation matrix global to local
 *@param[out] omega_in omega_in
 *@param[out] bsdf_f bsdf_f
 *@param[out] contribute contribute
 *@param[out] pdf pdf
 */
static void SampleBsdf(const Scene& scene, const float3& omega_out,
                       const CyclesPrincipledBsdf& bsdf, const RNG& rng,
                       SurfaceInfo* surface_info, float Rgl[4][4],
                       float3* omega_in, float3* bsdf_f, float3* contribute,
                       float* pdf) {
  *contribute = float3(0.f);

  const auto w = FetchClosureSampleWeight(omega_out, bsdf);

  const float select_closure = rng.Draw();

  // Sample Bsdf
  if (select_closure < w.diffuse_sample_weight) {
    const std::array<float, 2> u_lambert = {rng.Draw(), rng.Draw()};
    float _pdf                           = 0.f;  // TODO
    const float _brdf_f =
        LambertBrdfSample(omega_out, u_lambert, omega_in, &_pdf);
    (void)_brdf_f;  // TODO
  } else if (select_closure <
             w.diffuse_sample_weight + w.subsurface_sample_weight) {
    float3 new_omega_out;
    float3 sss_throuput;
    int scatter_bounce;
    const bool success = random_walk_sss::RandomWalkSubsurface(
        scene, bsdf.subsurface_weight, bsdf.subsurface_albedo,
        bsdf.subsurface_radius, rng, surface_info, Rgl, &new_omega_out,
        &sss_throuput, &scatter_bounce);

    if (success) {
      CyclesPrincipledBsdf new_bsdf;
      new_bsdf.enable_diffuse = true;
      new_bsdf.diffuse_weight = sss_throuput;

      {
        const float3& normal = surface_info->normal_s;
        const float3 d       = DirectIllumination(
            scene, new_omega_out, *surface_info, Rgl, normal, rng,
            [&new_bsdf](const float3& omega_in_, const float3& omega_out_,
                        float3* bsdf_f_, float* pdf_) {
              EvalBsdf(omega_in_, omega_out_, new_bsdf, bsdf_f_, pdf_);
            });
        (*contribute) = d;
      }
      float3 dummy_contrib;
      SampleBsdf(scene, new_omega_out, new_bsdf, rng, surface_info, Rgl,
                 omega_in, bsdf_f, &dummy_contrib, pdf);
      return;
    }
    *omega_in = 0.f;
    *bsdf_f   = 0.f;
    *pdf      = 0.f;
    return;
  } else if (select_closure < w.diffuse_sample_weight +
                                  w.subsurface_sample_weight +
                                  w.specular_sample_weight) {
    const std::array<float, 2> u_specular = {rng.Draw(), rng.Draw()};
    float _pdf                            = 0.f;  // TODO
    const float _brdf_f =
        MicrofacetGGXSample(omega_out, bsdf.alpha_x, bsdf.alpha_y, u_specular,
                            /*refractive*/ false,
                            /*distrib*/ 2, omega_in, &_pdf);
    (void)_brdf_f;  // TODO
  } else {
    const std::array<float, 2> u_specular = {rng.Draw(), rng.Draw()};
    float _pdf                            = 0.f;  // TODO
    const float _brdf_f                   = MicrofacetGGXSample(
        omega_out, bsdf.clearcoat_alpha_x, bsdf.clearcoat_alpha_y, u_specular,
        /*refractive*/ false,
        /*distrib*/ 1, omega_in, &_pdf);
    (void)_brdf_f;  // TODO
  }

  EvalBsdf(*omega_in, omega_out, bsdf, bsdf_f, pdf);
}

static CyclesPrincipledBsdf ParamToBsdf(const Scene& scene,
                                        const SurfaceInfo& surface_info) {
  const CyclesPrincipledBsdfParameter& m_param =
      mpark::get<kCyclesPrincipledBsdfParameter>(
          *(surface_info.material_param));

  const float3 weight = 1.f;  // whole weight TODO
  // parameters
  //
  // use_principled_diffuse          true or false
  //
  // base_color                      [0,1]^3
  // subsurface                      [0,1]
  // subsurface_radius               [0,1]^3
  // subsurface_color                [0,1]^3
  //
  // metallic                        [0,1]
  //
  // specular                        [0,1]
  // specular_tint                   [0,1]
  // roughness                       [0,1]
  // anisotropic                     [0,1]
  // anisotropic_rotation            [0,1]
  //
  // sheen                           [0,1]
  // sheen_tint                      [0,1]
  //
  // clearcoat                       [0,1]
  // clearcoat_roughness             [0,1]
  // ior                             float
  //
  // transmission                    [0,1]
  // transmission_roughness          [0,1]^3
  //
  // base_color_tex_id               uint32_t
  // subsurface_color_tex_id         uint32_t

  float3 base_color;
  if (m_param.base_color_tex_id != uint32_t(-1)) {
    const Texture* texture = scene.GetTexture(m_param.base_color_tex_id);

    texture->FetchFloat3(surface_info.texcoord.x(), surface_info.texcoord.y(),
                         base_color.v);
  } else {
    base_color = m_param.base_color;
  }

  float subsurface                = m_param.subsurface;
  const float3& subsurface_radius = m_param.subsurface_radius;
  float3 subsurface_color;
  if (m_param.subsurface_color_tex_id != uint32_t(-1)) {
    const Texture* texture = scene.GetTexture(m_param.subsurface_color_tex_id);

    texture->FetchFloat3(surface_info.texcoord.x(), surface_info.texcoord.y(),
                         subsurface_color.v);
  } else {
    subsurface_color = m_param.subsurface_color;
  }

  const float& metallic = m_param.metallic;

  const float& specular             = m_param.specular;
  const float& specular_tint        = m_param.specular_tint;
  const float& roughness            = m_param.roughness;
  const float& anisotropic          = m_param.anisotropic;
  const float& anisotropic_rotation = m_param.anisotropic_rotation;

  const float& sheen      = m_param.sheen;       // TODO
  const float& sheen_tint = m_param.sheen_tint;  // TODO

  const float& clearcoat           = m_param.clearcoat;
  const float& clearcoat_roughness = m_param.clearcoat_roughness;
  const float& ior                 = m_param.ior;

  const float& transmission           = m_param.transmission;
  const float& transmission_roughness = m_param.transmission_roughness;

  const float kClosureWeightCutOff = kEps;  // TODO

  CyclesPrincipledBsdf bsdf;

  // precompute value
  const float _diffuse_weight =  // Not output
      (1.0f - Saturate(metallic)) * (1.0f - Saturate(transmission));
  const float _final_transmission =
      Saturate(transmission) * (1.0f - Saturate(metallic));
  const float _specular_weight = (1.0f - _final_transmission);  // Not Output

  // diffuse
  {
    const float3 mixed_ss_base_color =
        subsurface_color * subsurface + base_color * (1.0f - subsurface);

    bsdf.enable_diffuse = false;
    if (Average(mixed_ss_base_color) > kClosureWeightCutOff) {
      if (subsurface < kClosureWeightCutOff &&
          _diffuse_weight > kClosureWeightCutOff) {
        bsdf.enable_diffuse = true;
        bsdf.diffuse_weight = weight * base_color * _diffuse_weight;
        // TODO roughness (use principled diffuse)
      } else if (subsurface > kClosureWeightCutOff /* TODO && _diffuse_weight > kClosureWeightCutOff ? */) {
        bsdf.enable_subsurface = true;
        const float3 subsurf_weight =
            weight * mixed_ss_base_color * _diffuse_weight;

        bsdf.subsurface_weight = subsurf_weight;
        bsdf.subsurface_albedo = mixed_ss_base_color;
        bsdf.subsurface_radius = subsurface_radius * subsurface;
        float3 add_diffuse_weight(0.f);

        random_walk_sss::BssrdfSetup(
            /*burey_radius*/ true, /*scale_mfp*/ true, /*use_eq5*/ true,
            &(bsdf.subsurface_weight), &(bsdf.subsurface_albedo),
            &(bsdf.subsurface_radius), &add_diffuse_weight);

        if (!IsBlack(add_diffuse_weight)) {
          bsdf.enable_diffuse = true;
          bsdf.diffuse_weight = bsdf.diffuse_weight + add_diffuse_weight;
        }
      }
    }
  }

  // sheen
  {
    // TODO
  }

  /* specular reflection */
  bsdf.enable_specular = false;
  if (_specular_weight > kClosureWeightCutOff &&
      (specular > kClosureWeightCutOff || metallic > kClosureWeightCutOff)) {
    bsdf.enable_specular = true;

    bsdf.specular_weight = weight * _specular_weight;

    bsdf.ior = (2.0f / (1.0f - SafeSqrtf(0.08f * specular))) - 1.0f;

    const float aspect     = SafeSqrtf(1.0f - anisotropic * 0.9f);
    const float roughness2 = roughness * roughness;

    bsdf.alpha_x = roughness2 / aspect;
    bsdf.alpha_y = roughness2 * aspect;

    const float y_base_color = RgbToY(base_color);
    const float3 rho_tint =
        y_base_color > 0.0f ? base_color / y_base_color : float3(0.0f);
    const float3 rho_specular = Lerp(float3(1.0f), rho_tint, specular_tint);
    bsdf.specular_color =
        Lerp(0.08f * specular * rho_specular, base_color, metallic);
  }

  /* clearcoat */
  bsdf.enable_clearcoat = false;
  if (clearcoat > kClosureWeightCutOff) {
    bsdf.enable_clearcoat = true;

    bsdf.clearcoat_weight = float3(0.25f * clearcoat);

    bsdf.clearcoat_alpha_x = clearcoat_roughness * clearcoat_roughness;
    bsdf.clearcoat_alpha_y = clearcoat_roughness * clearcoat_roughness;

    bsdf.clearcoat_color = float3(0.04f);

    bsdf.clearcoat_ior = 1.5f;
  }

  return bsdf;
}

void CyclesPrincipledShader(const Scene& scene, const float3& global_omega_out,
                            const RNG& rng, SurfaceInfo* surface_info,
                            float3* global_omega_in, float3* throuput,
                            float3* contribute, float* pdf) {
  if (surface_info->face_direction == SurfaceInfo::kAmbiguous) {
    *global_omega_in = global_omega_out;
    *throuput        = float3(0.0f);
    *contribute      = float3(0.0f);
    *pdf             = 0.0f;
    return;
  }

  // TODO tangent, binormal
  const float3 ez = (surface_info->face_direction == SurfaceInfo::kFront)
                        ? surface_info->normal_s
                        : -surface_info->normal_s;

  float3 ex, ey;
  BranchlessONB(ez, &ex, &ey);

#ifdef NDEBUG
#else
  assert(CheckONB(ex, ey, ez));
#endif

  float Rgl[4][4];  // global to shading local
  GrobalToShadingLocal(ex, ey, ez, Rgl);

  float3 omega_out;
  Matrix::MultV(global_omega_out.v, Rgl, omega_out.v);

  assert(std::abs(omega_out[2] - vdot(global_omega_out, ez)) < kEps);

  const CyclesPrincipledBsdf bsdf = ParamToBsdf(scene, *surface_info);

  *contribute = float3(0.f);
  {
    const float3 d = DirectIllumination(
        scene, omega_out, *surface_info, Rgl, ez, rng,
        [&bsdf](const float3& omega_in_, const float3& omega_out_,
                float3* bsdf_f_, float* pdf_) {
          EvalBsdf(omega_in_, omega_out_, bsdf, bsdf_f_, pdf_);
        });
    (*contribute) = (*contribute) + d;
  }

  // Sample Bsdf
  float3 omega_in(0.f), bsdf_f(0.f), _contrib(0.f);
  float ret_pdf = 0.f;
  SampleBsdf(scene, omega_out, bsdf, rng, surface_info, Rgl, &omega_in, &bsdf_f,
             &_contrib, &ret_pdf);

  *contribute = (*contribute) + _contrib;

  auto& Rlg = Rgl;
  ShadingLocalToGlobal(ex, ey, ez, Rlg);  // local to global
  Matrix::MultV(omega_in.v, Rlg, global_omega_in->v);

  assert(std::abs(vdot(*global_omega_in, ez) - omega_in[2]) < kEps);

  const auto cos_i = abs(omega_in[2]);
  *throuput        = bsdf_f * cos_i / ret_pdf;
  *pdf             = ret_pdf;  // pdf of bsdf sampling

  assert(*pdf < std::numeric_limits<float>::epsilon() || IsFinite(*throuput));

  if (!IsFinite(*throuput) || !std::isfinite(*pdf)) {
    *throuput = float3(0.f);
    *pdf      = 0.f;
  }
}

}  // namespace pbrlab
