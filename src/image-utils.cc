#include "image-utils.h"

#include <assert.h>
#include <math.h>

namespace pbrlab {

template <typename T>
T SrgbToLiner(const T c_srgb) {
  T c_liner;
  if (c_srgb <= static_cast<T>(0.04045)) {
    c_liner = c_srgb / static_cast<T>(12.92);
  } else {
    const T a = static_cast<T>(0.055);
    c_liner =
        std::pow((c_srgb + a) / (static_cast<T>(1.0) + a), static_cast<T>(2.4));
  }

  return c_liner;
}

template float SrgbToLiner(const float c_srgb);
template double SrgbToLiner(const double c_srgb);

template <typename T>
T LinerTosRGB(const T c_liner) {
  T c_srgb;
  if (c_liner <= static_cast<T>(0.0031308)) {
    c_srgb = static_cast<T>(12.92) * c_liner;
  } else {
    const T a = static_cast<T>(0.055);
    c_srgb    = std::pow((static_cast<T>(1.0) + a) * c_liner,
                      static_cast<T>(1.0 / 2.4)) -
             a;
  }

  return c_srgb;
}

template float LinerTosRGB(const float c_liner);
template double LinerTosRGB(const double c_liner);

template <typename T>
void SrgbToLiner(const std::vector<T>& src, const size_t width,
                 const size_t height, const size_t channels,
                 std::vector<T>* out) {
  assert(src.size() == width * height * channels);

  out->resize(width * height * channels);
  for (size_t i = 0; i < height; ++i) {
    for (size_t j = 0; j < width; ++j) {
      for (size_t k = 0; k < channels; ++k) {
        const size_t index = i * width * channels + j * channels + k;
        if (k < 3) {
          (*out)[index] = SrgbToLiner(src[index]);
        } else {
          (*out)[index] = src[index];
        }
      }
    }
  }
}

template void SrgbToLiner(const std::vector<float>& src, const size_t width,
                          const size_t height, const size_t channels,
                          std::vector<float>* out);
template void SrgbToLiner(const std::vector<double>& src, const size_t width,
                          const size_t height, const size_t channels,
                          std::vector<double>* out);

template <typename T>
void LinerToSrgb(const std::vector<T>& src, const size_t width,
                 const size_t height, const size_t channels,
                 std::vector<T>* out) {
  assert(src.size() == width * height * channels);

  out->resize(width * height * channels);
  for (size_t i = 0; i < height; ++i) {
    for (size_t j = 0; j < width; ++j) {
      for (size_t k = 0; k < channels; ++k) {
        const size_t index = i * width * channels + j * channels + k;
        if (k < 3) {
          (*out)[index] = LinerTosRGB(src[index]);
        } else {
          (*out)[index] = src[index];
        }
      }
    }
  }
}

template void LinerToSrgb(const std::vector<float>& src, const size_t width,
                          const size_t height, const size_t channels,
                          std::vector<float>* out);
template void LinerToSrgb(const std::vector<double>& src, const size_t width,
                          const size_t height, const size_t channels,
                          std::vector<double>* out);

}  // namespace pbrlab
