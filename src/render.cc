#include "render.h"

#include <iostream>
#include <tuple>
#include <vector>

#include "light-manager.h"
#include "pbrlab-util.h"
#include "random/rng.h"
#include "scene.h"
#include "shader/shader-utils.h"
#include "shader/shader.h"

namespace pbrlab {
void Normalize(float* v);

float3 GetRadiance(const Ray& input_ray, const Scene& scene, const RNG& rng);

SurfaceInfo TraceResultToSufaceInfo(const Ray& ray, const Scene& scene,
                                    const TraceResult& trace_result);

float3 GetRadiance(const Ray& input_ray, const Scene& scene, const RNG& rng) {
  Ray ray = input_ray;
  float3 contribution(0.0f);
  float3 throuput(1.0f);
  float bsdf_sampling_pdf = 0.f;
  float prev_cos          = 0.f;

  for (uint32_t depth = 0;; depth++) {
    const TraceResult trace_result = scene.TraceFirstHit1(ray);

    if (trace_result.instance_id == static_cast<uint32_t>(-1)) {
      // TODO
      break;
    }

    const SurfaceInfo surface_info =
        TraceResultToSufaceInfo(ray, scene, trace_result);

    {
      if (surface_info.face_direction == SurfaceInfo::kFront) {
        const auto ret =
            scene.ImplicitAreaLight(trace_result.instance_id,
                                    trace_result.geom_id, trace_result.prim_id);
        const float inv_g =
            abs((trace_result.t * trace_result.t) /
                (prev_cos * vdot(surface_info.normal_s, ray.ray_dir)));

        const float weight =
            (depth == 0)
                ? 1.0f
                : PowerHeuristicWeight(bsdf_sampling_pdf, ret.pdf * inv_g);
        contribution = contribution + weight * ret.emission * throuput;
      }
    }

    // start russian roulette
    const float russian_roulette_p = SpectrumNorm(throuput);
    if (russian_roulette_p < rng.Draw()) break;
    throuput = throuput * float3(1.0f / russian_roulette_p);
    // end russian roulette

    // start shadeing and sampling nextray
    float3 next_ray_org;
    float3 next_ray_dir;
    float3 r_throuput;
    float3 d_contribute;
    float _bsdf_sampling_pdf;
    Shader(scene, -ray.ray_dir, surface_info, rng, &next_ray_org, &next_ray_dir,
           &r_throuput, &d_contribute, &_bsdf_sampling_pdf);

    contribution      = contribution + throuput * d_contribute;
    throuput          = r_throuput * throuput;
    bsdf_sampling_pdf = _bsdf_sampling_pdf;
    prev_cos          = vdot(next_ray_dir, surface_info.normal_s);

    ray.ray_org = next_ray_org;
    ray.ray_dir = next_ray_dir;
    ray.min_t   = 1e-3f;
    ray.max_t   = kInf;
    // end shadeing and sampling nextray
  }
  return contribution;
}

bool Render(const Scene& scene, const uint32_t width, const uint32_t height,
            const uint32_t num_sample, RenderLayer* layer) {
  float bmax[3], bmin[3];
  scene.FetchSceneAABB(bmin, bmax);

  // fov = 30 deg
  const float screen_size =
      std::max(bmax[0] - bmin[0], bmax[1] - bmin[1]) * 2.0f;
  const float x_center = (bmax[0] + bmin[0]) * 0.5f;
  const float y_center = (bmax[1] + bmin[1]) * 0.5f;
  const float z_center = bmax[2] + screen_size * 0.5f * std::sqrt(3.0f);
  float ray_org[3]     = {x_center, y_center, z_center};
  const float x_corner = (bmax[0] + bmin[0]) * 0.5f - screen_size * 0.5f;
  const float y_corner = (bmax[1] + bmin[1]) * 0.5f + screen_size * 0.5f;
  const float z_corner = bmax[2];
  const float dx       = screen_size / width;
  const float dy       = screen_size / height;

  layer->Resize(width, height);
  layer->Clear();

  RNG rng(12345, 67890);  // TODO
  for (uint32_t sample = 0; sample < num_sample; sample++) {
    std::cerr << "sample " << sample + 1 << std::endl;
    for (uint32_t y = 0; y < height; y++) {
      for (uint32_t x = 0; x < width; x++) {
        float target[3]  = {x_corner + dx * x, y_corner - dy * y, z_corner};
        float ray_dir[3] = {target[0] - ray_org[0], target[1] - ray_org[1],
                            target[2] - ray_org[2]};
        Normalize(ray_dir);

        Ray ray;
        ray.ray_dir = float3(ray_dir);
        ray.ray_org = float3(ray_org);

        const float3 radiance = GetRadiance(ray, scene, rng);
        layer->rgba[(y * width + x) * 4 + 0] += radiance[0];
        layer->rgba[(y * width + x) * 4 + 1] += radiance[1];
        layer->rgba[(y * width + x) * 4 + 2] += radiance[2];
        layer->rgba[(y * width + x) * 4 + 3] += 1.0f;

        layer->count[y * width + x]++;
      }
    }
  }

  return true;
}

SurfaceInfo TraceResultToSufaceInfo(const Ray& ray, const Scene& scene,
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
void Normalize(float* v) {
  const float inv_norm =
      1.0f / std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
  v[0] *= inv_norm;
  v[1] *= inv_norm;
  v[2] *= inv_norm;
}

}  // namespace pbrlab
