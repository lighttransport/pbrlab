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

void BilinearFilter(const std::vector<float>& image, const size_t width,
                    const size_t height, const size_t components, const float u,
                    const float v, const bool wrap_u, const bool wrap_v,
                    float* dst) {
  float uu, vv;

  if (wrap_u) {
    uu = u - floor(u);
  } else {
    // clamp
    uu = std::max(u, 0.0f);
    uu = std::min(uu, 1.0f);
  }

  if (wrap_v) {
    vv = v - floor(v);
  } else {
    vv = std::max(v, 0.0f);
    vv = std::min(vv, 1.0f);
  }

  const float px = float(width) * uu;
  const float py = float(height) * vv;

  const int x0 = std::max(0, std::min(int(width) - 1, int(px)));
  const int y0 = std::max(0, std::min(int(height) - 1, int(py)));

  int x1, y1;

  if (wrap_u) {
    x1 = ((x0 + 1) >= int(width)) ? 0 : (x0 + 1);
  } else {
    x1 = ((x0 + 1) >= int(width)) ? (int(width) - 1) : (x0 + 1);
  }

  if (wrap_v) {
    y1 = ((y0 + 1) >= int(height)) ? 0 : (y0 + 1);
  } else {
    y1 = ((y0 + 1) >= int(height)) ? (int(height) - 1) : (y0 + 1);
  }
  const float dx = px - float(x0);
  const float dy = py - float(y0);

  float w[4];
  w[0] = (1.0f - dx) * (1.0f - dy);
  w[1] = (1.0f - dx) * (dy);  // +y
  w[2] = (dx) * (1.0f - dy);  // +x
  w[3] = (dx) * (dy);         // +x, +y

  const int stride = int(components);

  const int i00 = stride * (y0 * int(width) + x0);
  const int i01 = stride * (y0 * int(width) + x1);
  const int i10 = stride * (y1 * int(width) + x0);
  const int i11 = stride * (y1 * int(width) + x1);

  float texel[4][4];
  for (int i = 0; i < stride; i++) {
    texel[0][i] = image[size_t(i00 + i)];
    texel[1][i] = image[size_t(i10 + i)];
    texel[2][i] = image[size_t(i01 + i)];
    texel[3][i] = image[size_t(i11 + i)];
  }

  for (int i = 0; i < stride; i++) {
    dst[i] = texel[0][i] * w[0] + texel[1][i] * w[1] + texel[2][i] * w[2] +
             texel[3][i] * w[3];
  }
}

}  // namespace pbrlab
