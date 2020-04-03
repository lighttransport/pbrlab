#ifndef PBRLAB_PC_COMMON_H_
#define PBRLAB_PC_COMMON_H_
#include <atomic>
#include <iostream>
#include <string>

#include "render-layer.h"
#include "scene.h"

struct RenderItem {
  RenderItem(void);

  pbrlab::Scene scene;
  pbrlab::RenderLayer layer;

  std::atomic_bool cancel_render_flag;
  std::atomic_bool finish_frag;

  std::atomic_size_t last_render_pass;
  std::atomic_size_t finish_pass;
  std::atomic_size_t max_pass;
};

bool CreateScene(int argc, char** argv, pbrlab::Scene* scene);

#endif  // PBRLAB_PC_COMMON_H_
