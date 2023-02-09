#ifndef PBRLAB_IMAGE_IO_H_
#define PBRLAB_IMAGE_IO_H_

#include <string>
#include <vector>

namespace pbrlab {
namespace io {

///
/// Loads Image
///
/// @param[in] filename Input filename.
/// @param[in] asset_path Asset search path for input image filename.
/// @param[out] pixels Image data
/// @param[out] width Image width
/// @param[out] height Image height
/// @param[out] channels Image channels
///
/// @return true upon success to load image.
///
template <typename T>
bool LoadImageFromFile(const std::string& filename, const std::string& asset_path,
               std::vector<T>* pixels, size_t* width, size_t* height,
               size_t* channels);

extern template bool LoadImageFromFile(const std::string& filename,
                               const std::string& asset_path,
                               std::vector<unsigned char>* pixels,
                               size_t* width, size_t* height, size_t* channels);

extern template bool LoadImageFromFile(const std::string& filename,
                               const std::string& asset_path,
                               std::vector<float>* pixels, size_t* width,
                               size_t* height, size_t* channels);

///
/// Write PNG Image
///
/// @param[in] filename Input filename.
/// @param[in] asset_path Asset search path for input image filename.
/// @param[in] pixels Image data
/// @param[in] width Image width
/// @param[in] height Image height
/// @param[in] channels Image channels
///
/// @return true upon success to write image.
///
template <typename T>
bool WritePNG(const std::string& filename, const std::string& asset_path,
              const std::vector<T>& pixels, const size_t width,
              const size_t height, const size_t channels);

extern template bool WritePNG(const std::string& filename,
                              const std::string& asset_path,
                              const std::vector<unsigned char>& pixels,
                              const size_t width, const size_t height,
                              const size_t channels);

extern template bool WritePNG(const std::string& filename,
                              const std::string& asset_path,
                              const std::vector<float>& pixels,
                              const size_t width, const size_t height,
                              const size_t channels);

}  // namespace io
}  // namespace pbrlab

#endif  // PBRLAB_IMAGE_IO_H_
