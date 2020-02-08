#include "shader/shader.h"

#include "shader/cycles-principled-shader.h"

namespace pbrlab {

void Shader(const Scene& scene, const float3& global_omega_out,
            const SurfaceInfo& surface_info, const RNG& rng,
            float3* next_ray_org, float3* next_ray_dir, float3* throuput,
            float3* contribute, float* pdf) {
  if (surface_info.material_param == nullptr) {
    *next_ray_org = surface_info.global_position;
    *next_ray_dir = global_omega_out;
    *throuput     = float3(0.0f);
    *contribute   = float3(0.0f);
    *pdf          = 0.0f;
  }

  if (surface_info.material_param->index() == kCyclesPrincipledBsdfParameter) {
    CyclesPrincipledShader(scene, global_omega_out, surface_info, rng,
                           next_ray_org, next_ray_dir, throuput, contribute,
                           pdf);
    return;
  }
  assert(false);

  *next_ray_org = surface_info.global_position;
  *next_ray_dir = global_omega_out;
  *throuput     = float3(0.0f);
  *contribute   = float3(0.0f);
  *pdf          = 0.0f;
}  // namespace pbrlab

}  // namespace pbrlab
