#include "render-layer.h"

namespace pbrlab {

RenderLayer::RenderLayer() = default;
RenderLayer::RenderLayer(const size_t w, const size_t h) {
  RenderLayer();
  Resize(w, h);
  Clear();
}
RenderLayer::~RenderLayer() = default;

void RenderLayer::Clear(void) {
  std::lock_guard<std::mutex> lock(mtx);

  std::fill_n(rgba.begin(), rgba.size(), 0.0f);
  std::fill_n(count.begin(), count.size(), uint32_t(0));
}

void RenderLayer::Resize(const size_t w, const size_t h) {
  std::lock_guard<std::mutex> lock(mtx);

  width  = w;
  height = h;

  size_t n = width * height;

  rgba.resize(n * 4);
  count.resize(n);  // scalar
}

};  // namespace pbrlab
