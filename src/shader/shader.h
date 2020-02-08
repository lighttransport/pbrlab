#ifndef PBRLAB_SHADER_H_
#define PBRLAB_SHADER_H_
#include <tuple>

#include "random/rng.h"
#include "scene.h"
#include "shader-utils.h"
#include "type.h"

namespace pbrlab {
/**
 *@param[in] scene scene
 *@param[in] global_omega_out omega out (global)
 *@param[in] surface_info surface information
 *@param[in] rng random generator
 *@param[out] next_ray_org nex ray origin
 *@param[out] next_ray_dir nex ray direction
 *@param[out] throuput local throuput
 *@param[out] contribute contribution of DI
 *@param[out] pdf pdf of bsdf sampling
 */
void Shader(const Scene& scene, const float3& global_omega_out,
            const SurfaceInfo& surface_info, const RNG& rng,
            float3* next_ray_org, float3* next_ray_dir, float3* throuput,
            float3* contribute, float* pdf);

}  // namespace pbrlab
#endif  // PBRLAB_SHADER_H_
