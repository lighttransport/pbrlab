#ifndef PBRLAB_RENDER_LAYER_H_
#define PBRLAB_RENDER_LAYER_H_

#include <stdint.h>

#include <mutex>
#include <vector>

namespace pbrlab {

struct RenderLayer {
  explicit RenderLayer();
  explicit RenderLayer(const size_t w, const size_t h);
  ~RenderLayer();

  void Clear(void);
  void Resize(const size_t w, const size_t h);

  size_t width;
  size_t height;

  std::vector<float> rgba;
  std::vector<uint32_t> count;

  mutable std::mutex mtx;
};

};  // namespace pbrlab

#endif  // PBRLAB_RENDER_LAYER_H_
