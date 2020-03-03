#include <stdio.h>
#include <stdlib.h>

#include <atomic>
#include <iostream>
#include <string>

#include "glfw-window.h"
#include "image-utils.h"
#include "pc-common.h"
#include "render.h"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#endif

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#endif

#ifdef PBRLAB_USE_STACK_TRACE_LOGGER
#include <glog/logging.h>
#endif

#ifdef __clang__
#pragma clang diagnostic pop
#endif

static void GlfwErrorCallback(int error, const char* description) {
  fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

struct GuiContext {
  std::shared_ptr<ImageBuffer> pImageBuffer;
};

static void InitImageBuffer(const size_t width, const size_t height,
                            const size_t channel, GLWindow* gl_window,
                            GuiContext* gui_context) {
  const size_t tex_id = gl_window->CreateGlTexture();
  gl_window->SetCurrentGlTexture(tex_id);

  const size_t buffer_id = gl_window->CreateBuffer(width, height, channel);
  gl_window->SetCurrentBuffer(buffer_id);
  gui_context->pImageBuffer = gl_window->FetchBuffer(buffer_id);
}

static void BufferUpdater(const pbrlab::RenderLayer& layer,
                          ImageBuffer* buffer) {
  const size_t width   = layer.width;
  const size_t height  = layer.height;
  const size_t channel = 4;
  if (width * height * channel == 0) return;
  std::vector<float> tmp(width * height * channel);

  const uint32_t num_threads =
      1U;  // std::max(1U, std::thread::hardware_concurrency());

  {
    std::vector<std::thread> workers;
    std::atomic_uint32_t i(0);

    for (uint32_t thread_id = 0; thread_id < num_threads; ++thread_id) {
      workers.emplace_back([&, thread_id](void) {
        (void)thread_id;
        uint32_t px_id = 0;
        while ((px_id = i++) < layer.count.size()) {
          tmp[px_id * 4 + 0] = layer.rgba[px_id * 4 + 0] / layer.count[px_id];
          tmp[px_id * 4 + 1] = layer.rgba[px_id * 4 + 1] / layer.count[px_id];
          tmp[px_id * 4 + 2] = layer.rgba[px_id * 4 + 2] / layer.count[px_id];
          tmp[px_id * 4 + 3] = layer.rgba[px_id * 4 + 3] / layer.count[px_id];
        }
      });
    }
    for (auto& w : workers) {
      w.join();
    }
  }

  pbrlab::LinerToSrgb(tmp, width, height, 4, &tmp);

  {
    std::vector<std::thread> workers;
    std::atomic_uint32_t i(0);

    std::lock_guard<std::mutex> lock(buffer->mtx);
    buffer->buffer.resize(width * height * channel);
    buffer->width   = width;
    buffer->height  = height;
    buffer->channel = channel;
    for (uint32_t thread_id = 0; thread_id < num_threads; ++thread_id) {
      workers.emplace_back([&, thread_id](void) {
        (void)thread_id;
        uint32_t px_id = 0;
        while ((px_id = i++) < width * height * channel) {
          buffer->buffer[px_id] = tmp[px_id];
        }
      });
    }
    for (auto& w : workers) {
      w.join();
    }
    buffer->has_update = true;
  }
}

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;
#ifdef PBRLAB_USE_STACK_TRACE_LOGGER
  google::InitGoogleLogging(argv[0]);
  google::InstallFailureSignalHandler();
#endif

  glfwSetErrorCallback(GlfwErrorCallback);

  if (glfwInit() == GL_FALSE) {
    std::cerr << "Failed to initialize GLFW." << std::endl;
    return -1;
  }

  atexit(glfwTerminate);

  const uint32_t width   = 1024;  // TODO
  const uint32_t height  = 1024;  // TODO
  const uint32_t samples = 512;   // TODO
  GLWindow gl_window(int(width), int(height), "PBR lab Viewer");
  printf("start app\n");

  GuiContext gui_context;
  InitImageBuffer(width, height, 4, &gl_window, &gui_context);

  // Set Scene
  const std::string obj_filename = std::string(argv[1]);
  pbrlab::Scene scene;
  if (!CreateScene(obj_filename, &scene)) {
    return EXIT_FAILURE;
  }

  std::atomic_bool finish_frag(false);

  // Start Rendering thread
  pbrlab::RenderLayer layer;
  std::thread rendering(
      [&](void) { pbrlab::Render(scene, width, height, samples, &layer); });

  std::thread buffer_updater([&](void) {
    while (!finish_frag) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
      BufferUpdater(layer, gui_context.pImageBuffer.get());
    }
  });

  // When the window is open
  while (gl_window.ShouldClose() == GL_FALSE) {
    glClearColor(0.2f, 0.2f, 0.2f, 0.0f);
    // Clear Buffer
    glClear(GL_COLOR_BUFFER_BIT);
    glClearDepth(1.0);

    gl_window.DrawCurrentBuffer();
    // exchange color buffer and Fetch Event
    gl_window.SwapBuffers();
  }

  finish_frag = true;

  rendering.join();
  buffer_updater.join();

  return EXIT_SUCCESS;
}
