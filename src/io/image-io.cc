#include "io/image-io.h"

#include <ctype.h>  // tolower

#include <algorithm>
#include <iostream>
#include <type_traits>

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#endif

#if defined(__cplusplus) && __cplusplus >= 201703L && defined(__has_include)
#if __has_include(<filesystem>)
#define GHC_USE_STD_FS
#include <filesystem>
namespace fs = std::filesystem;
#endif
#endif
#ifndef GHC_USE_STD_FS
#include <ghc/filesystem.hpp>
namespace fs = ghc::filesystem;
#endif

#include "io/stb_image.h"
#include "io/stb_image_write.h"
#include "io/tinyexr.h"

#ifdef __clang__
#pragma clang diagnostic pop
#endif

namespace pbrlab {
namespace io {

template <typename T>
static T Clamp(const T x, const T a, const T b) {
  return std::max(a, std::min(b, x));
}

template <typename T>
static bool LoadExrImage(const std::string& filepath, std::vector<T>* pixels,
                         size_t* width, size_t* height, size_t* channels) {
  int w, h;
  float* rgba;
  const char* err = nullptr;
  const int ret   = LoadEXR(&rgba, &w, &h, filepath.c_str(), &err);

  if (ret == TINYEXR_SUCCESS) {
    *width    = size_t(w);
    *height   = size_t(h);
    *channels = 4;

    size_t n_elem = size_t(w * h);

    // RGBA
    for (size_t i = 0; i < n_elem; i++) {
      (*pixels)[4 * i + 0] = T(rgba[4 * i + 0]);
      (*pixels)[4 * i + 1] = T(rgba[4 * i + 1]);
      (*pixels)[4 * i + 2] = T(rgba[4 * i + 2]);
      (*pixels)[4 * i + 3] = T(rgba[4 * i + 3]);
    }

    free(rgba);
    return true;
  }
  return false;
}

template <typename T>
bool LoadImage(const std::string& filename, const std::string& asset_path,
               std::vector<T>* pixels, size_t* width, size_t* height,
               size_t* channels) {
  if (pixels == nullptr || width == nullptr || height == nullptr ||
      channels == nullptr) {
    return false;
  }
  const fs::path file_path = fs::path(asset_path) / filename;

  if (std::is_same<float, T>::value || std::is_same<double, T>::value) {  // exr
    const std::string file_extension = fs::path(filename).extension();

    std::string tmp;
    std::transform(file_extension.begin(), file_extension.end(), tmp.begin(),
                   tolower);
    if (file_extension == ".exr") {
      return LoadExrImage(file_path.string(), pixels, width, height, channels);
    }
  }

  int tmp_w = 0;
  int tmp_h = 0;
  int tmp_c = 0;

  static_assert(
      std::is_same<unsigned char, T>::value || std::is_same<float, T>::value,
      "ivalid pixel type");

  if (std::is_same<unsigned char, T>::value) {
    unsigned char* pixeli =
        stbi_load(file_path.c_str(), &tmp_w, &tmp_h, &tmp_c, 0);

    *width    = size_t(tmp_w);
    *height   = size_t(tmp_h);
    *channels = size_t(tmp_c);

    const size_t n = (*width) * (*height) * (*channels);

    if (pixeli != nullptr) {
      pixels->resize(n);
      for (size_t i = 0; i < n; ++i) {
        (*pixels)[i] = T(pixeli[i]);
      }
    }

    stbi_image_free(pixeli);
  } else if (std::is_same<float, T>::value) {
    float* pixelf = stbi_loadf(file_path.c_str(), &tmp_w, &tmp_h, &tmp_c, 0);

    *width    = size_t(tmp_w);
    *height   = size_t(tmp_h);
    *channels = size_t(tmp_c);

    const size_t n = (*width) * (*height) * (*channels);

    if (pixelf != nullptr) {
      pixels->resize(n);
      for (size_t i = 0; i < n; ++i) {
        (*pixels)[i] = T(pixelf[i]);
      }
    }

    stbi_image_free(pixelf);
  }

  return *width != 0 && *height != 0 && *channels != 0 &&
         pixels->size() == (*width) * (*height) * (*channels);
}

template bool LoadImage(const std::string& filename,
                        const std::string& asset_path,
                        std::vector<unsigned char>* pixels, size_t* width,
                        size_t* height, size_t* channels);

template bool LoadImage(const std::string& filename,
                        const std::string& asset_path,
                        std::vector<float>* pixels, size_t* width,
                        size_t* height, size_t* channels);

template <typename T>
bool WritePNG(const std::string& filename, const std::string& asset_path,
              const std::vector<T>& pixels, const size_t width,
              const size_t height, const size_t channels) {
  const fs::path file_path         = fs::path(asset_path) / filename;
  const std::string file_extension = file_path.extension();
  {
    std::string tmp;
    std::transform(file_extension.begin(), file_extension.end(), tmp.begin(),
                   tolower);
    if (file_extension != ".png") {
      // TODO logger
      std::cerr << "warning! the file extension is not \"png\"" << std::endl;
      return false;
    }
  }
  if (pixels.size() == 0 || pixels.size() != width * height * channels) {
    // TODO logger
    std::cerr << "the image data is broken" << std::endl;
    return false;
  }

  const size_t n = width * height * channels;

  std::vector<unsigned char> pixel8(n);

  static_assert(
      std::is_same<unsigned char, T>::value || std::is_same<float, T>::value,
      "ivalid pixel type");

  if (std::is_same<unsigned char, T>::value) {
    std::copy(pixels.begin(), pixels.end(), pixel8.begin());
  } else if (std::is_same<float, T>::value) {
    for (size_t i = 0; i < n; ++i) {
      pixel8[i] =
          static_cast<unsigned char>(Clamp(pixels[i] * 256.0f, 0.0f, 255.0f));
      // TODO 255.0f ?
    }
  }
  int ret = stbi_write_png(file_path.c_str(), int(width), int(height),
                           int(channels), pixel8.data(), int(width * channels));

  if (!ret) {
    // TODO logger
    std::cerr << "faild save image" << std::endl;
    return false;
  }

  // TODO logger
  std::cerr << "write png file [ " << filename << " ]" << std::endl;

  return true;
}
template bool WritePNG(const std::string& filename,
                       const std::string& asset_path,
                       const std::vector<unsigned char>& pixels,
                       const size_t width, const size_t height,
                       const size_t channels);

template bool WritePNG(const std::string& filename,
                       const std::string& asset_path,
                       const std::vector<float>& pixels, const size_t width,
                       const size_t height, const size_t channels);

}  // namespace io
}  // namespace pbrlab
