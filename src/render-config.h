#ifndef PBRLAB_RENDER_CONFIG_H_
#define PBRLAB_RENDER_CONFIG_H_

#include <string>
#include <vector>

namespace pbrlab {

struct RenderConfig {
  std::vector<std::string> scene_filepaths;

  uint32_t width  = 512;
  uint32_t height = 512;

  uint32_t max_pass = 32;

  int thread = -1;
};

}  // namespace pbrlab

#endif  // PBRLAB_RENDER_CONFIG_H_
