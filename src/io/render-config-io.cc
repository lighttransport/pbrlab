#include "io/render-config-io.h"

#include <iostream>

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#endif

#include "staticjson/staticjson.hpp"

#ifdef __clang__
#pragma clang diagnostic pop
#endif

namespace staticjson {

template <>
void init(pbrlab::RenderConfig* config, ObjectHandler* h) {
  h->add_property("scene_filepaths", &(config->scene_filepaths));

  h->add_property("width", &(config->width));
  h->add_property("height", &(config->height));

  h->add_property("max_pass", &(config->max_pass));

  h->add_property("thread", &(config->thread), staticjson::Flags::Optional);
}

}  // namespace staticjson

namespace pbrlab {

bool LoadConfigFromJson(const std::string& filepath, RenderConfig* config) {
  staticjson::ParseStatus res;
  if (!staticjson::from_json_file(filepath, config, &res)) {
    // TOOD use logger
    std::cerr << "Failed to parse JSON.\n";
    std::cerr << res.description() << "\n";
    return false;
  }
  return true;
}

}  // namespace pbrlab
