#ifndef PBRLAB_CYCLES_PRINCIPLED_SHADER_H_
#define PBRLAB_CYCLES_PRINCIPLED_SHADER_H_
#include <tuple>

#include "closure/lambert.h"
#include "material-param.h"
#include "random/rng.h"
#include "scene.h"
#include "shader-utils.h"
#include "type.h"

namespace pbrlab {

void CyclesPrincipledShader(const Scene& scene, const float3& global_omega_out,
                            const SurfaceInfo& surface_info, const RNG& rng,
                            float3* next_ray_org, float3* next_ray_dir,
                            float3* throuput, float3* contribute, float* pdf);
}  // namespace pbrlab
#endif  // PBRLAB_CYCLES_PRINCIPLED_SHADER_H_
