#ifndef PBRLAB_TEXTURE_H_
#define PBRLAB_TEXTURE_H_

#include <stdint.h>

#include <string>
#include <vector>

namespace pbrlab {

class Texture {
public:
  explicit Texture(void);
  explicit Texture(const std::vector<float>& pixels, const uint32_t width,
                   const uint32_t height, const uint32_t channels,
                   const std::string& name);

  bool Reset(const std::vector<float>& pixels, const uint32_t width,
             const uint32_t height, const uint32_t channels);

  void SetName(const std::string& name);

  uint32_t GetWidth(void) const;
  uint32_t GetHeight(void) const;
  uint32_t GetChannels(void) const;
  std::string GetName(void) const;

  void FetchFloatN(const float u, const float v, const uint32_t n,
                   float* dst) const;

  void FetchFloat(const float u, const float v, float* dst) const;
  void FetchFloat2(const float u, const float v, float* dst) const;
  void FetchFloat3(const float u, const float v, float* dst) const;
  void FetchFloat4(const float u, const float v, float* dst) const;

private:
  uint32_t width_;
  uint32_t height_;
  uint32_t channels_;
  std::vector<float> pixels_;
  std::string name_;
};

}  // namespace pbrlab

#endif  // PBRLAB_TEXTURE_H_
