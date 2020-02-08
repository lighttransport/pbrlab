#ifndef PBRLAB_RENDER_H_
#define PBRLAB_RENDER_H_

#include "pbrlab_math.h"
#include "ray.h"
#include "render-layer.h"
#include "scene.h"
#include "type.h"

namespace pbrlab {
bool Render(const Scene& scene, const uint32_t width, const uint32_t height,
            const uint32_t num_sample, RenderLayer* layer);
}  // namespace pbrlab
#endif  // PBRLAB_RENDER_H_
