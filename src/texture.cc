#include "texture.h"

#include <assert.h>

#include "image-utils.h"

namespace pbrlab {

Texture::Texture(void) : width_(0), height_(0), channels_(0) {}
Texture::Texture(const std::vector<float>& pixels, const uint32_t width,
                 const uint32_t height, const uint32_t channels)
    : width_(width), height_(height), channels_(channels), pixels_(pixels) {
  // TODO error processing
  assert(width_ * height_ * channels_ == pixels_.size());
}

bool Texture::Reset(const std::vector<float>& pixels, const uint32_t width,
                    const uint32_t height, const uint32_t channels) {
  if (width * height * channels != pixels.size()) {
    return false;
  }

  pixels_   = pixels;
  width_    = width;
  height_   = height;
  channels_ = channels;

  return true;
}

uint32_t Texture::GetWidth(void) const { return width_; }
uint32_t Texture::GetHeight(void) const { return height_; }
uint32_t Texture::GetChannels(void) const { return channels_; }

void Texture::FetchFloatN(const float u, const float v, const uint32_t n,
                          float* dst) const {
  std::vector<float> tmp(channels_);

  BilinearFilter(pixels_, width_, height_, channels_, u, v, false, false,
                 tmp.data());

  for (size_t i = 0; i < n; ++i) {
    if (i < channels_) {
      dst[i] = tmp[i];
    } else {
      dst[i] = 0.f;
    }
  }
}

void Texture::FetchFloat(const float u, const float v, float* dst) const {
  FetchFloatN(u, v, 1, dst);
}
void Texture::FetchFloat2(const float u, const float v, float* dst) const {
  FetchFloatN(u, v, 2, dst);
}
void Texture::FetchFloat3(const float u, const float v, float* dst) const {
  FetchFloatN(u, v, 3, dst);
}
void Texture::FetchFloat4(const float u, const float v, float* dst) const {
  FetchFloatN(u, v, 4, dst);
}

}  // namespace pbrlab
