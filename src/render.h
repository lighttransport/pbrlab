#ifndef PBRLAB_RENDER_H_
#define PBRLAB_RENDER_H_

#include <atomic>

#include "pbrlab_math.h"
#include "ray.h"
#include "render-layer.h"
#include "scene.h"
#include "type.h"

namespace pbrlab {

bool Render(const Scene& scene, const uint32_t width, const uint32_t height,
            const uint32_t num_sample,
            const std::atomic_bool& cancel_render_flag, RenderLayer* layer,
            std::atomic_size_t* finish_pass);

}  // namespace pbrlab
#endif  // PBRLAB_RENDER_H_
