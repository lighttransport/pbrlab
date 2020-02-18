#ifndef PBRLAB_GLFW_WINDOW_H_
#define PBRLAB_GLFW_WINDOW_H_
#include <memory>

#include "gui-parameter.h"

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#endif

// embeded font data for ImGui
#include "imgui/IconsIonicons.h"
#include "imgui/ionicons_embed.inc.h"
#include "imgui/roboto_mono_embed.inc.h"

// deps/imgui
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

// deps/ImGuizmo
#include "ImGuizmo/ImGuizmo.h"

// glad
#include "glad/glad.h"

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#ifdef __APPLE__
#include <OpenGL/glu.h>
#else
#include <GL/glu.h>
#endif

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#endif
#include "GLFW/glfw3.h"

#ifdef __clang__
#pragma clang diagnostic pop
#endif

class GLWindow {
  // handle(pointer) of window
  GLFWwindow *const window_;

  GuiParameter gui_param;

public:
  GLWindow() = delete;
  GLWindow(int width, int height, const char *title);

  virtual ~GLWindow();
  // Determine if the window should be closed
  int ShouldClose() const;
  // Fetch event with swapping color buffer
  void SwapBuffers();
};
#endif  // PBRLAB_GLFW_WINDOW_H_
