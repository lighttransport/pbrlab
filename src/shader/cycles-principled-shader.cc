#include "cycles-principled-shader.h"

#include <assert.h>

#include <array>

#include "closure/lambert.h"
#include "closure/microfacet-ggx.h"
#include "material-param.h"
#include "matrix.h"
#include "random/rng.h"
#include "sampler/sampling-utils.h"
#include "shader-utils.h"
#include "type.h"

namespace pbrlab {

struct CyclesPrincipledBsdf {
  float3 diffuse_weight;
};

static auto FetchClosureSampleWeight(const CyclesPrincipledBsdf& bsdf) {
  (void)bsdf;  // TODO
  struct {
    float diffuse_sample_weight;
  } ret;

  ret.diffuse_sample_weight = 1.0f;
  return ret;
}

static auto EvalBsdf(const float3& omega_in, const float3& omega_out,
                     const CyclesPrincipledBsdf& bsdf) {
  struct {
    float3 bsdf_f = float3(0.f);
    float pdf     = 0.0f;
  } ret;

  const auto w = FetchClosureSampleWeight(bsdf);

  {  // diffuse
    const auto _ret = LambertBrdfPdf(omega_in, omega_out);
    ret.bsdf_f += bsdf.diffuse_weight * _ret.brdf_f;
    ret.pdf += w.diffuse_sample_weight * _ret.pdf;
  }

  return ret;
}

static auto SampleBsdf(const float3& omega_out,
                       const CyclesPrincipledBsdf& bsdf, const RNG& rng) {
  struct {
    float3 omega_in = float3(0.f, 0.f, 1.f);
    float3 bsdf_f   = float3(0.f);
    float pdf       = 0.0f;
  } ret;

  const auto w = FetchClosureSampleWeight(bsdf);

  float select_closure = rng.Draw();

  // Sample Bsdf
  if (select_closure < w.diffuse_sample_weight) {
    const std::array<float, 2> u_lambert = {rng.Draw(), rng.Draw()};
    const auto tmp = LambertBrdfSample(omega_out, u_lambert);
    ret.omega_in   = tmp.omega_in;
  }

  const auto tmp = EvalBsdf(ret.omega_in, omega_out, bsdf);
  ret.bsdf_f     = tmp.bsdf_f;
  ret.pdf        = tmp.pdf;

  return ret;
}

static CyclesPrincipledBsdf ParamToBsdf(
    const CyclesPrincipledBsdfParameter& m_param) {
  CyclesPrincipledBsdf bsdf;
  bsdf.diffuse_weight = m_param.base_color;

  return bsdf;
}
CyclesPrincipledBsdf ParamToBsdf(const CyclesPrincipledBsdfParameter& m_param);

void CyclesPrincipledShader(const Scene& scene, const float3& global_omega_out,
                            const SurfaceInfo& surface_info, const RNG& rng,
                            float3* next_ray_org, float3* next_ray_dir,
                            float3* throuput, float3* contribute, float* pdf) {
  if (surface_info.face_direction == SurfaceInfo::kAmbiguous) {
    *next_ray_org = surface_info.global_position;
    *next_ray_dir = global_omega_out;
    *throuput     = float3(0.0f);
    *contribute   = float3(0.0f);
    *pdf          = 0.0f;
  }

  // TODO tangent, binormal
  const float3 ez = (surface_info.face_direction == SurfaceInfo::kFront)
                        ? surface_info.normal_s
                        : -surface_info.normal_s;

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

  const CyclesPrincipledBsdfParameter* m_param =
      mpark::get_if<CyclesPrincipledBsdfParameter>(surface_info.material_param);

  const CyclesPrincipledBsdf bsdf = ParamToBsdf(*m_param);

  *contribute = float3(0.f);
  // TODO : function
  {
    const std::shared_ptr<LightManager> light_manager = scene.GetLightManager();

    const auto result_light_sample = light_manager->SampleAllLight(rng);
    if (result_light_sample.light_type == kAreaLight) {
      // Area measure
      const float3& pos            = surface_info.global_position;
      const float3& light_position = result_light_sample.v1;
      const float3& light_normal   = result_light_sample.v2;
      const float3 dir_to_light =
          vnormalized(light_position - surface_info.global_position);
      const float dist = vlength(pos - light_position);

      const float wl_dot_nl = -vdot(dir_to_light, light_normal);
      const float wl_dot_np = vdot(dir_to_light, ez);

      // To solid angle measure
      const float pdf_sigma =
          result_light_sample.pdf * dist * dist / (wl_dot_nl * wl_dot_np);

      if (wl_dot_nl > 0.0f && wl_dot_np > 0.0f &&
          !ShadowRay(scene, pos, dir_to_light, dist)) {
        float3 omega_l;
        Matrix::MultV(dir_to_light.v, Rgl, omega_l.v);

        const auto ret = EvalBsdf(omega_l, omega_out, bsdf);

        const float weight =
            PowerHeuristicWeight(pdf_sigma /*light*/, ret.pdf /*bsdf*/);

        (*contribute) = (*contribute) + ret.bsdf_f *
                                            result_light_sample.emission *
                                            weight / (pdf_sigma);
      }
    }
  }

  // Sample Bsdf
  const auto ret = SampleBsdf(omega_out, bsdf, rng);

  const auto cos_i = ret.omega_in[2];

  auto& Rlg = Rgl;
  ShadingLocalToGlobal(ex, ey, ez, Rlg);  // local to world
  Matrix::MultV(ret.omega_in.v, Rgl, next_ray_dir->v);

  assert(std::abs(vdot(*next_ray_dir, ez) - ret.omega_in[2]) < kEps);

  *next_ray_org = surface_info.global_position;
  *throuput     = ret.bsdf_f * cos_i / ret.pdf;
  *pdf          = ret.pdf;  // pdf of bsdf sampling
}

}  // namespace pbrlab
