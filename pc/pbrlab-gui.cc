#include <stdio.h>
#include <stdlib.h>

#include <atomic>
#include <iostream>
#include <string>

#if defined(PBRLAB_USE_SDL)
#include "sdl-window.h"
#else
#include "glfw-window.h"
#endif

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

struct GuiItem {
  std::shared_ptr<ImageBuffer> pImageBuffer;
};

#if !defined(PBRLAB_USE_SDL)

#define CHECK_GL(tag) do { \
  GLenum err = glGetError(); \
  if (err != GL_NO_ERROR) { \
    std::cerr << "OpenGL err: " << __FILE__ << ":" << __LINE__ << ":" << __func__ << " code = " << err << ", tag = " << tag << "\n"; \
  } \
} while(0)

static void GlfwErrorCallback(int error, const char* description) {
  fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

static void InitImageBuffer(const size_t width, const size_t height,
                            const size_t channel, GLWindow* gl_window,
                            GuiItem* gui_item) {
  const size_t tex_id = gl_window->CreateGlTexture();
  gl_window->SetCurrentGlTexture(tex_id);

  const size_t buffer_id = gl_window->CreateBuffer(width, height, channel);
  gl_window->SetCurrentBuffer(buffer_id);
  gui_item->pImageBuffer = gl_window->FetchBuffer(buffer_id);
}

#endif



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
    std::lock_guard<std::mutex> lock(layer.mtx);
    std::vector<std::thread> workers;
    std::atomic_uint32_t i(0);

    for (uint32_t thread_id = 0; thread_id < num_threads; ++thread_id) {
      workers.emplace_back([&, thread_id](void) {
        (void)thread_id;
        uint32_t px_id = 0;
        while ((px_id = i++) < layer.count.size()) {
          if (layer.count[px_id] == 0) {
            tmp[px_id * 4 + 0] = 0.f;
            tmp[px_id * 4 + 1] = 0.f;
            tmp[px_id * 4 + 2] = 0.f;
            tmp[px_id * 4 + 3] = 1.f;
          } else {
            tmp[px_id * 4 + 0] =
                layer.rgba[px_id * 4 + 0] / float(layer.count[px_id]);
            tmp[px_id * 4 + 1] =
                layer.rgba[px_id * 4 + 1] / float(layer.count[px_id]);
            tmp[px_id * 4 + 2] =
                layer.rgba[px_id * 4 + 2] / float(layer.count[px_id]);
            tmp[px_id * 4 + 3] =
                layer.rgba[px_id * 4 + 3] / float(layer.count[px_id]);
          }
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

  if (argc < 2) {
    std::cerr << "Needs at least one input scene filename(.obj or .hair)\n";
    return -1;
  }

#ifdef PBRLAB_USE_STACK_TRACE_LOGGER
  google::InitGoogleLogging(argv[0]);
  google::InstallFailureSignalHandler();
#endif

#if !defined(PBRLAB_USE_SDL)

  glfwSetErrorCallback(GlfwErrorCallback);

  if (glfwInit() == GL_FALSE) {
    std::cerr << "Failed to initialize GLFW." << std::endl;
    return -1;
  }

#if defined(__APPLE__)
  // macOS only supports GL 3.2+
  const char* glsl_version = "#version 150";
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // macOS. core profile only
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
  // GL 3.0 + GLSL 130
  const char* glsl_version = "#version 130";
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
  // glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+
  // only glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // 3.0+ only
#endif

  atexit(glfwTerminate);

  const uint32_t width   = 1024;  // TODO
  const uint32_t height  = 1024;  // TODO
  const uint32_t samples = 32;    // TODO


  std::shared_ptr<RenderItem> pRenderItem(new RenderItem());

  if (!CreateScene(argc, argv, &(pRenderItem->scene))) {
    return EXIT_FAILURE;
  }

  GLWindow gl_window(int(width), int(height), glsl_version, "PBR lab Viewer");
  printf("start app\n");

  GuiItem gui_item;
  InitImageBuffer(width, height, 4, &gl_window, &gui_item);

  gl_window.SetRenderItem(pRenderItem);

  std::atomic_bool& finish_frag        = pRenderItem->finish_frag;
  std::atomic_bool& cancel_render_flag = pRenderItem->cancel_render_flag;
  std::atomic_size_t& last_render_pass = pRenderItem->last_render_pass;
  std::atomic_size_t& finish_pass      = pRenderItem->finish_pass;
  std::atomic_size_t& max_pass         = pRenderItem->max_pass;

  max_pass = samples;

  // Start Rendering thread
  std::thread rendering([&](void) {
    while (!finish_frag) {
      if (finish_pass >= max_pass) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        continue;
      }

      cancel_render_flag = false;

      pRenderItem->edit_queue.EditAndPopAll();

      pbrlab::Render(pRenderItem->scene, width, height,
                     uint32_t(max_pass.load()), cancel_render_flag,
                     &(pRenderItem->layer), &finish_pass);
    }
  });

  std::thread buffer_updater([&](void) {
    last_render_pass = 0;
    while (!finish_frag) {
      if (finish_pass >= max_pass) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        continue;
      }
      if (last_render_pass >= finish_pass) {
        std::this_thread::sleep_for(std::chrono::microseconds(333333));
        continue;
      }
      last_render_pass.store(finish_pass.load());
      BufferUpdater(pRenderItem->layer, gui_item.pImageBuffer.get());
    }
  });

  // When the window is open
  while (gl_window.ShouldClose() == GL_FALSE) {

    CHECK_GL("draw loop begin");

    glClearColor(0.2f, 0.2f, 0.25f, 0.0f);
    // Clear Buffer
    glClear(GL_COLOR_BUFFER_BIT);
    //glClearDepth(1.0);

    CHECK_GL("gl Clear");

    gl_window.DrawCurrentBuffer();

    CHECK_GL("draw current buffer");

    gl_window.DrawImguiUI();
    CHECK_GL("draw Imgui stuff");

    // exchange color buffer and Fetch Event
    gl_window.SwapBuffers();

    CHECK_GL("swap buffers");
  }

  cancel_render_flag = true;
  finish_frag        = true;

  rendering.join();
  buffer_updater.join();

#else
  // TODO

#endif

  printf("finish app\n");

  return EXIT_SUCCESS;
}
