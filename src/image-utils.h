#ifndef PBRLAB_IMAGE_UTILS_H_
#define PBRLAB_IMAGE_UTILS_H_

#include <string>
#include <tuple>
#include <vector>

namespace pbrlab {

template <typename T>
T SrgbToLiner(const T c_srgb);
extern template float SrgbToLiner(const float c_srgb);
extern template double SrgbToLiner(const double c_srgb);

template <typename T>
T LinerTosRGB(const T c_liner);
extern template float LinerTosRGB(const float c_liner);
extern template double LinerTosRGB(const double c_liner);

template <typename T>
void SrgbToLiner(const std::vector<T>& src, const size_t width,
                 const size_t height, const size_t channels,
                 std::vector<T>* out);
extern template void SrgbToLiner(const std::vector<float>& src,
                                 const size_t width, const size_t height,
                                 const size_t channels,
                                 std::vector<float>* out);
extern template void SrgbToLiner(const std::vector<double>& src,
                                 const size_t width, const size_t height,
                                 const size_t channels,
                                 std::vector<double>* out);

template <typename T>
void LinerToSrgb(const std::vector<T>& src, const size_t width,
                 const size_t height, const size_t channels,
                 std::vector<T>* out);
extern template void LinerToSrgb(const std::vector<float>& src,
                                 const size_t width, const size_t height,
                                 const size_t channels,
                                 std::vector<float>* out);
extern template void LinerToSrgb(const std::vector<double>& src,
                                 const size_t width, const size_t height,
                                 const size_t channels,
                                 std::vector<double>* out);
}  // namespace pbrlab

#endif  // PBRLAB_IMAGE_UTILS_H_
