#ifndef PBRLAB_SHADER_UTILS_H_
#define PBRLAB_SHADER_UTILS_H_
#include <math.h>

#include <functional>

#include "material-param.h"
#include "matrix.h"
#include "pbrlab-util.h"
#include "pbrlab_math.h"
#include "random/rng.h"
#include "ray.h"
#include "raytracer/raytracer.h"
#include "scene.h"
#include "type.h"

namespace pbrlab {
struct SurfaceInfo {
  float u;
  float v;

  uint32_t instance_id = static_cast<uint32_t>(-1);  // instance id
  uint32_t geom_id     = static_cast<uint32_t>(-1);  // local geom id
  uint32_t prim_id     = static_cast<uint32_t>(-1);  // primitive id

  float3 global_position;

  float3 normal_s;
  float3 normal_g;
  float3 tangent;    // TODO optional
  float3 bitangent;  // TODO optional

  // TODO texcoord

  enum FaceDirection { kFront, kBack, kAmbiguous };
  FaceDirection face_direction;

  const MaterialParameter* material_param = nullptr;
};

// https://graphics.pixar.com/library/OrthonormalB/paper.pdf
inline void BranchlessONB(const float3& n, float3* x, float3* y) {
  const float sign = copysignf(1.0f, n[2]);
  const float a    = -1.0f / (sign + n[2]);
  const float b    = n[0] * n[1] * a;
  *x = float3(1.0f + sign * n[0] * n[0] * a, sign * b, -sign * n[0]);
  *y = float3(b, sign + n[1] * n[1] * a, -n[1]);
}

inline bool CheckONB(const float3& ex, const float3& ey, const float3& ez) {
  bool ret        = true;
  const float eps = kEps;
  ret             = (ret && (std::abs(vlength(ex) - 1.0f) < eps));
  ret             = (ret && (std::abs(vlength(ey) - 1.0f) < eps));
  ret             = (ret && (std::abs(vlength(ez) - 1.0f) < eps));

  ret = (ret && (vlength(vcross(ex, ey) - ez)) < eps);
  ret = (ret && (vlength(vcross(ey, ez) - ex)) < eps);
  ret = (ret && (vlength(vcross(ez, ex) - ey)) < eps);

  return ret;
}

inline void GrobalToShadingLocal(const float3& ex, const float3& ey,
                                 const float3& ez, float mat[4][4]) {
  // v' = v * M

  mat[0][0] = ex[0];
  mat[0][1] = ey[0];
  mat[0][2] = ez[0];
  mat[0][3] = 0.0f;

  mat[1][0] = ex[1];
  mat[1][1] = ey[1];
  mat[1][2] = ez[1];
  mat[1][3] = 0.0f;

  mat[2][0] = ex[2];
  mat[2][1] = ey[2];
  mat[2][2] = ez[2];
  mat[2][3] = 0.0f;

  mat[3][0] = 0.0f;
  mat[3][1] = 0.0f;
  mat[3][2] = 0.0f;
  mat[3][3] = 1.0f;
}

inline void ShadingLocalToGlobal(const float3& ex, const float3& ey,
                                 const float3& ez, float mat[4][4]) {
  // v' = v * M

  mat[0][0] = ex[0];
  mat[0][1] = ex[1];
  mat[0][2] = ex[2];
  mat[0][3] = 0.0f;

  mat[1][0] = ey[0];
  mat[1][1] = ey[1];
  mat[1][2] = ey[2];
  mat[1][3] = 0.0f;

  mat[2][0] = ez[0];
  mat[2][1] = ez[1];
  mat[2][2] = ez[2];
  mat[2][3] = 0.0f;

  mat[3][0] = 0.0f;
  mat[3][1] = 0.0f;
  mat[3][2] = 0.0f;
  mat[3][3] = 1.0f;
}

inline bool ShadowRay(const Scene& scene, const float3& pos, const float3& dir,
                      const float dist) {
  Ray ray;
  ray.ray_org = pos;
  ray.ray_dir = dir;

  ray.min_t = kEps;
  ray.max_t = std::max(ray.min_t, dist - kEps);

  return scene.AnyHit1(ray);
}

inline SurfaceInfo TraceResultToSufaceInfo(const Ray& ray, const Scene& scene,
                                           const TraceResult& trace_result) {
  SurfaceInfo surface_info = {};

  surface_info.u = trace_result.u;
  surface_info.v = trace_result.v;

  surface_info.instance_id = trace_result.instance_id;
  surface_info.geom_id     = trace_result.geom_id;
  surface_info.prim_id     = trace_result.prim_id;

  surface_info.global_position = ray.ray_org + trace_result.t * ray.ray_dir;

  surface_info.normal_s = scene.FetchMeshShadingNormal(trace_result);
  surface_info.normal_g = float3(trace_result.normal_g);

  // TODO tangent, binormal

  // back face test
  if (vdot(ray.ray_dir, surface_info.normal_g) < 0.0f &&
      vdot(ray.ray_dir, surface_info.normal_s) < 0.0f)
    surface_info.face_direction = SurfaceInfo::kFront;
  else if (vdot(ray.ray_dir, surface_info.normal_g) > 0.0f &&
           vdot(ray.ray_dir, surface_info.normal_s) > 0.0f)
    surface_info.face_direction = SurfaceInfo::kBack;
  else
    surface_info.face_direction = SurfaceInfo::kAmbiguous;

  surface_info.material_param = scene.FetchMeshMaterialParamPtr(trace_result);

  return surface_info;
}

inline float3 DirectIllumination(
    const Scene& scene, const float3& omega_out,
    const SurfaceInfo& surface_info, const float Rgl[4][4],
    const float3 global_normal, const RNG& rng,
    const std::function<void(const float3& omega_in, const float3& omega_out,
                             float3* bsdf_f, float* pdf)>
        EvalFunc) {
  const LightManager* light_manager = scene.GetLightManager();
  const auto result_light_sample    = light_manager->SampleAllLight(rng);

  float3 contribute(0.f);

  if (result_light_sample.light_type == kAreaLight) {
    // Area measure
    const float3& pos            = surface_info.global_position;
    const float3& light_position = result_light_sample.v1;
    const float3& light_normal   = result_light_sample.v2;
    const float3 dir_to_light =
        vnormalized(light_position - surface_info.global_position);
    const float dist = vlength(pos - light_position);

    const float wl_dot_nl = -vdot(dir_to_light, light_normal);
    const float wl_dot_np = vdot(dir_to_light, global_normal);

    // To solid angle measure
    const float pdf_sigma =
        result_light_sample.pdf * dist * dist / (wl_dot_nl * wl_dot_np);

    if (wl_dot_nl > 0.0f && wl_dot_np > 0.0f &&
        !ShadowRay(scene, pos, dir_to_light, dist)) {
      float3 omega_l;
      Matrix::MultV(dir_to_light.v, Rgl, omega_l.v);

      float3 bsdf_f(0.0f);
      float ret_pdf = 0.f;
      EvalFunc(omega_l, omega_out, &bsdf_f, &ret_pdf);

      const float weight =
          PowerHeuristicWeight(pdf_sigma /*light*/, ret_pdf /*bsdf*/);

      contribute = bsdf_f * result_light_sample.emission * weight / (pdf_sigma);
      assert(IsFinite(contribute));
    }
  }
  return contribute;
}

}  // namespace pbrlab
#endif  // PBRLAB_SHADER_UTILS_H_
