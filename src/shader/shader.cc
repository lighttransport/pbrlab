#include "shader/shader.h"

#include "shader/cycles-principled-shader.h"

namespace pbrlab {

void Shader(const Scene& scene, const float3& global_omega_out, const RNG& rng,
            SurfaceInfo* surface_info, float3* global_omega_in,
            float3* throuput, float3* contribute, float* pdf) {
  if (surface_info->material_param == nullptr) {
    *global_omega_in = global_omega_out;
    *throuput        = float3(0.0f);
    *contribute      = float3(0.0f);
    *pdf             = 0.0f;
    return;
  }

  if (surface_info->material_param->index() == kCyclesPrincipledBsdfParameter) {
    CyclesPrincipledShader(scene, global_omega_out, rng, surface_info,
                           global_omega_in, throuput, contribute, pdf);
    return;
  }
  assert(false);

  *global_omega_in = global_omega_out;
  *throuput        = float3(0.0f);
  *contribute      = float3(0.0f);
  *pdf             = 0.0f;
}  // namespace pbrlab

}  // namespace pbrlab
