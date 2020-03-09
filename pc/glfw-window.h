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
private:
  // handle(pointer) of window
  GLFWwindow *const window_;
  GuiParameter gui_param_;
  GLuint shader_program_id_;

  size_t current_tex_id_ = size_t(-1);
  std::vector<GLuint> texture_ids_;

public:
  GLWindow() = delete;
  GLWindow(int width, int height, const char *title);

  virtual ~GLWindow();

  size_t CreateGlTexture(void);
  bool SetCurrentGlTexture(const size_t tex_id);
  size_t CreateBuffer(const size_t width, const size_t height,
                      const size_t channel);
  bool SetCurrentBuffer(const size_t buffer_id);
  std::shared_ptr<ImageBuffer> FetchBuffer(const size_t buffer_id);

  void DrawCurrentBuffer(void);

  // Determine if the window should be closed
  int ShouldClose() const;
  // Fetch event with swapping color buffer
  void SwapBuffers();
};
#endif  // PBRLAB_GLFW_WINDOW_H_