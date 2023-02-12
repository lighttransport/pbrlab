#ifndef PBRLAB_SDL_WINDOW_H_
#define PBRLAB_SDL_WINDOW_H_
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

// embeded font data for ImGui
#include "imgui/IconsIonicons.h"
#include "imgui/ionicons_embed.inc.h"
#include "imgui/roboto_mono_embed.inc.h"

// deps/imgui
#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl2.h"
#include "imgui/imgui_impl_sdlrenderer.h"

// deps/ImGuizmo
#include "ImGuizmo/ImGuizmo.h"

#ifdef __clang__
#pragma clang diagnostic pop
#endif

class SDLWindow {
private:
  // handle(pointer) of window
  SDL_Window *const window_{nullptr};
  GuiParameter gui_param_;

  size_t current_tex_id_ = size_t(-1);
  //std::vector<GLuint> texture_ids_;

public:
  SDLWindow() = delete;
  SDLWindow(int width, int height, const char *glsl_version, const char *title);

  virtual ~SDLWindow();

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

#endif  // PBRLAB_SDL_WINDOW_H_
