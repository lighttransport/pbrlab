#ifndef PBRLAB_RENDER_CONFIG_IO_H_
#define PBRLAB_RENDER_CONFIG_IO_H_

#include <string>

#include "render-config.h"

namespace pbrlab {

bool LoadConfigFromJson(const std::string& filepath, RenderConfig* config);

}

#endif  // PBRLAB_RENDER_CONFIG_IO_H_
