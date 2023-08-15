#ifndef PBRLAB_GLFW_WINDOW_H_
#define PBRLAB_GLFW_WINDOW_H_
#include <memory>

#ifdef _WIN32
#include <windows.h>
#endif

#include "gui-parameter.h"
#include "pc-common.h"

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#endif

#if 1
// deps/glad
// OpenGL loader is still required(imgui's OpenGL3 loader does not cover enough
// OpenGL APIs used in pbrlab)
#include "glad/glad.h"
#endif

// embeded font data for ImGui
#include "imgui/IconsIonicons.h"
#include "imgui/ionicons_embed.inc.h"
#include "imgui/roboto_mono_embed.inc.h"

// deps/imgui
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
//#include "imgui/imgui_impl_opengl3_loader.h"

// deps/ImGuizmo
#include "ImGuizmo/ImGuizmo.h"

#include "GLFW/glfw3.h"

#if 0 // glu is not used anymore
#ifdef __APPLE__
#include <OpenGL/glu.h>
#else
#include <GL/glu.h>
#endif
#endif


#ifdef __clang__
#pragma clang diagnostic pop
#endif

class GLWindow {
private:
  // handle(pointer) of window
  GLFWwindow *const window_;
  GuiParameter gui_param_;
  GLuint shader_program_id_{0};
  GLuint vba_id_{0}; // vertex attrib array id
  GLuint pos_buffer_id_{0};
  GLuint uv_buffer_id_{0};

  size_t current_tex_id_ = size_t(-1);
  std::vector<GLuint> texture_ids_;

public:
  GLWindow() = delete;
  GLWindow(int width, int height, const char *glsl_version, const char *title);

  virtual ~GLWindow();

  void CreateGLVertexData(void);
  size_t CreateGlTexture(void);
  bool SetCurrentGlTexture(const size_t tex_id);
  size_t CreateBuffer(const size_t width, const size_t height,
                      const size_t channel);
  bool SetCurrentBuffer(const size_t buffer_id);
  void SetRenderItem(std::shared_ptr<RenderItem> &render_item);

  std::shared_ptr<ImageBuffer> FetchBuffer(const size_t buffer_id);

  void DrawCurrentBuffer(void);

  void DrawImguiUI(void);

  // Determine if the window should be closed
  int ShouldClose() const;
  // Fetch event with swapping color buffer
  void SwapBuffers();
};
#endif  // PBRLAB_GLFW_WINDOW_H_
