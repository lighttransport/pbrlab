#include <stdio.h>
#include <stdlib.h>

#include <iostream>

#include "glfw-window.h"

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

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;
#ifdef PBRLAB_USE_STACK_TRACE_LOGGER
  google::InitGoogleLogging(argv[0]);
  google::InstallFailureSignalHandler();
#endif

  if (glfwInit() == GL_FALSE) {
    std::cerr << "Failed to initialize GLFW." << std::endl;
    return -1;
  }

  glfwSetErrorCallback(GlfwErrorCallback);

  atexit(glfwTerminate);

  GLWindow gl_window(960, 720, "PBR lab Viewer");
  printf("start app\n");

  // When the window is open
  while (gl_window.ShouldClose() == GL_FALSE) {
    // Clear Buffer
    glClear(GL_COLOR_BUFFER_BIT);
    // exchange color buffer and Fetch Event
    gl_window.SwapBuffers();
  }

  return EXIT_SUCCESS;
}
