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

/**
 *@param[in] scene scene
 *@param[in] global_omega_out omega out (global)
 *@param[in] rng random generator
 *@param[in,out] surface_info surface information
 *@param[out] global_omega_in next ray direction
 *@param[out] throuput local throuput
 *@param[out] contribute contribution of DI
 *@param[out] pdf pdf of bsdf sampling
 */
void CyclesPrincipledShader(const Scene& scene, const float3& global_omega_out,
                            const RNG& rng, SurfaceInfo* surface_info,
                            float3* global_omega_in, float3* throuput,
                            float3* contribute, float* pdf);

}  // namespace pbrlab
#endif  // PBRLAB_CYCLES_PRINCIPLED_SHADER_H_
