#include <stdlib.h>

#include <iostream>
#include <vector>

#include "image-utils.h"
#include "io/image-io.h"
#include "io/triangle-mesh-io.h"
#include "pc-common.h"
#include "render.h"

#ifdef PBRLAB_USE_STACK_TRACE_LOGGER
#include <glog/logging.h>
#endif

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;
#ifdef PBRLAB_USE_STACK_TRACE_LOGGER
  google::InitGoogleLogging(argv[0]);
  google::InstallFailureSignalHandler();
#endif

  if (argc < 2) {
    std::cerr << "not specified input scene filename" << std::endl;
    return EXIT_FAILURE;
  }

  const std::string obj_filename = std::string(argv[1]);

  pbrlab::Scene scene;
  if (!CreateScene(argc, argv, &scene)) {
    return EXIT_FAILURE;
  }

  const size_t width   = 512;
  const size_t height  = 512;
  const size_t samples = 32;

  std::atomic_bool cancel_render_flag(false);
  std::atomic_size_t finish_pass(0);

  pbrlab::RenderLayer layer;
  pbrlab::Render(scene, width, height, samples, cancel_render_flag, &layer,
                 &finish_pass);

  std::vector<float> color(width * height * 4);

  for (size_t i = 0; i < layer.count.size(); ++i) {
    color[i * 4 + 0] = layer.rgba[i * 4 + 0] / float(layer.count[i]);
    color[i * 4 + 1] = layer.rgba[i * 4 + 1] / float(layer.count[i]);
    color[i * 4 + 2] = layer.rgba[i * 4 + 2] / float(layer.count[i]);
    color[i * 4 + 3] = layer.rgba[i * 4 + 3] / float(layer.count[i]);
  }

  pbrlab::LinerToSrgb(color, width, height, 4, &color);
  pbrlab::io::WritePNG("rgba.png", "./", color, width, height, 4);

  return EXIT_SUCCESS;
}
