#include "glfw-window.h"

#include <stdio.h>
#include <stdlib.h>

#include <string>

#include "gui-parameter.h"

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#endif

#if defined(__cplusplus) && __cplusplus >= 201703L && defined(__has_include)
#if __has_include(<filesystem>)
#define GHC_USE_STD_FS
#include <filesystem>
namespace fs = std::filesystem;
#endif
#endif
#ifndef GHC_USE_STD_FS
#include <ghc/filesystem.hpp>
namespace fs = ghc::filesystem;
#endif

#ifdef __clang__
#pragma clang diagnostic pop
#endif

static void ReshapeFunc(GLFWwindow *window, int w, int h) {
  int fb_w, fb_h;
  // Get actual framebuffer size.
  glfwGetFramebufferSize(window, &fb_w, &fb_h);
  glViewport(0, 0, fb_w, fb_h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(45.0, double(w) / double(h), 0.01, 100.0);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  auto *param =
      reinterpret_cast<GuiParameter *>(glfwGetWindowUserPointer(window));

  param->width  = w;
  param->height = h;
}

static void KeyboardFunc(GLFWwindow *window, int key, int scancode, int action,
                         int mods) {
  (void)window;
  (void)scancode;
  (void)mods;

  ImGuiIO &io = ImGui::GetIO();
  if (io.WantCaptureKeyboard) {
    return;
  }

  if (action == GLFW_PRESS || action == GLFW_REPEAT) {
    // Close window
    if (key == GLFW_KEY_Q || key == GLFW_KEY_ESCAPE)
      glfwSetWindowShouldClose(window, GL_TRUE);
  }
}

static void ClickFunc(GLFWwindow *window, int button, int action, int mods) {
  (void)mods;

  auto *params =
      reinterpret_cast<GuiParameter *>(glfwGetWindowUserPointer(window));

  if (button == GLFW_MOUSE_BUTTON_LEFT) {
    if (action == GLFW_PRESS) {
      params->mouse_left_pressed = true;
    } else if (action == GLFW_RELEASE) {
      params->mouse_left_pressed = false;
    }
  }
  if (button == GLFW_MOUSE_BUTTON_RIGHT) {
    if (action == GLFW_PRESS) {
      params->mouse_righ_tPressed = true;
    } else if (action == GLFW_RELEASE) {
      params->mouse_righ_tPressed = false;
    }
  }
  if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
    if (action == GLFW_PRESS) {
      params->mouse_midd_lePressed = true;
    } else if (action == GLFW_RELEASE) {
      params->mouse_midd_lePressed = false;
    }
  }
}

static void MotionFunc(GLFWwindow *window, double mouse_x, double mouse_y) {
  const float kTransScale = 2.0f;

  auto *params =
      reinterpret_cast<GuiParameter *>(glfwGetWindowUserPointer(window));

  if (!ImGui::GetIO().WantCaptureMouse) {
    if (params->mouse_left_pressed) {
      // TODO
    } else if (params->mouse_midd_lePressed) {
      params->eye[0] -= kTransScale * (float(mouse_x) - params->prev_mouse_x) /
                        float(params->width);
      params->lookat[0] -= kTransScale *
                           (float(mouse_x) - params->prev_mouse_x) /
                           float(params->width);
      params->eye[1] += kTransScale * (float(mouse_y) - params->prev_mouse_y) /
                        float(params->height);
      params->lookat[1] += kTransScale *
                           (float(mouse_y) - params->prev_mouse_y) /
                           float(params->height);
    } else if (params->mouse_righ_tPressed) {
      params->eye[2] += kTransScale * (float(mouse_y) - params->prev_mouse_y) /
                        float(params->height);
      params->lookat[2] += kTransScale *
                           (float(mouse_y) - params->prev_mouse_y) /
                           float(params->height);
    }
  }

  // Update mouse point
  params->prev_mouse_x = float(mouse_x);
  params->prev_mouse_y = float(mouse_y);
}

static void InitializeImgui(GLFWwindow *window) {
  // Setup Dear ImGui context
  ImGui::CreateContext();
  auto &io = ImGui::GetIO();

  // Read .ini file from parent directory if imgui.ini does not exist in the
  // current directory.
  if (fs::exists("../imgui.ini") && !fs::exists("./imgui.ini")) {
    printf("Use ../imgui.ini as Init file.\n");
    io.IniFilename = "../imgui.ini";
  }

  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

  float default_font_scale = 18.0f;
  ImFontConfig roboto_config;
  strcpy(roboto_config.Name, "Roboto");
  roboto_config.SizePixels = default_font_scale;

  io.Fonts->AddFontFromMemoryCompressedTTF(roboto_mono_compressed_data,
                                           roboto_mono_compressed_size,
                                           default_font_scale, &roboto_config);

  // Load Icon fonts
  ImFontConfig ionicons_config;
  ionicons_config.MergeMode          = true;
  ionicons_config.GlyphMinAdvanceX   = default_font_scale;
  ionicons_config.OversampleH        = 1;
  ionicons_config.OversampleV        = 1;
  static const ImWchar icon_ranges[] = {ICON_MIN_II, ICON_MAX_II, 0};
  io.Fonts->AddFontFromMemoryCompressedTTF(
      ionicons_compressed_data, ionicons_compressed_size, default_font_scale,
      &ionicons_config, icon_ranges);

  ImGui::StyleColorsDark();

  // Setup Platform/Renderer bindings
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init();
}

static GLuint CreateGlShader() {
  // Compile vertex shader
  GLuint v_shader_id       = glCreateShader(GL_VERTEX_SHADER);
  std::string vertexShader = R"#(
    attribute vec3 position;
    attribute vec2 uv;
    varying vec2 vuv;
    void main(void){
        gl_Position = vec4(position, 1.0);
        vuv = uv;
    }
    )#";
  const char *vs           = vertexShader.c_str();
  glShaderSource(v_shader_id, 1, &vs, nullptr);
  glCompileShader(v_shader_id);

  // Compile fragment shader
  GLuint f_shader_id         = glCreateShader(GL_FRAGMENT_SHADER);
  std::string fragmentShader = R"#(
    varying vec2 vuv;
    uniform sampler2D texture;
    void main(void){
        gl_FragColor = texture2D(texture, vuv);
    }
    )#";
  const char *fs             = fragmentShader.c_str();
  glShaderSource(f_shader_id, 1, &fs, nullptr);
  glCompileShader(f_shader_id);

  // Create program object
  GLuint program_id = glCreateProgram();
  glAttachShader(program_id, v_shader_id);
  glAttachShader(program_id, f_shader_id);

  // link
  glLinkProgram(program_id);

  return program_id;
}

GLWindow::GLWindow(int width, int height, const char *title)
    : window_(glfwCreateWindow(width, height, title, nullptr, nullptr)) {
  // Create Window
  if (window_ == nullptr) {
    fprintf(stderr, "failed creating GLFW window.\n");
    exit(EXIT_FAILURE);
  }
  fprintf(stderr, "success openning GLFW.\n");

  // make this window target
  glfwMakeContextCurrent(window_);

  // Set Buffer Swap Timing
  glfwSwapInterval(1);

  // register gui_param pointer in this instance
  glfwSetWindowUserPointer(window_, &gui_param_);

  // Register Callback funtion that is called when window size is changed
  glfwSetWindowSizeCallback(window_, ReshapeFunc);

  // Register Callback funtion that is called when keyboard is inputed
  glfwSetKeyCallback(window_, KeyboardFunc);

  // Register Callback funtion that is called when mouse button is pushed
  glfwSetMouseButtonCallback(window_, ClickFunc);

  // Register Callback funtion that is called when mouse is moved
  glfwSetCursorPosCallback(window_, MotionFunc);

  if (gladLoadGL() == 0) {
    fprintf(stderr, "Failed to initialize GLAD.\n");
    glfwDestroyWindow(window_);
    exit(EXIT_FAILURE);
  }

  if (!GLAD_GL_VERSION_3_0) {
    fprintf(stderr, "OpenGL 3.0 context not available.\n");
    glfwDestroyWindow(window_);
    exit(EXIT_FAILURE);
  }

  InitializeImgui(window_);

  ReshapeFunc(window_, width, height);

  // create shader program object
  shader_program_id_ = CreateGlShader();
  if (shader_program_id_ == 0) {
    fprintf(stderr, "faild create shader program object\n");
    exit(EXIT_FAILURE);
  }
}

GLWindow::~GLWindow() {
  // Release Texture
  for (const auto ti : texture_ids_) {
    GLuint tmp = GLuint(ti);
    glBindTexture(GL_TEXTURE_2D, 0);
    glDeleteTextures(1, &tmp);
    assert(glGetError() == GL_NO_ERROR);
  }

  glfwDestroyWindow(window_);
}

static GLuint CreateGlTexture(void) {
  GLuint ret;
  glGenTextures(1, &ret);
  return ret;
}

size_t GLWindow::CreateGlTexture(void) {
  const size_t tex_id = texture_ids_.size();
  texture_ids_.emplace_back(::CreateGlTexture());
  return tex_id;
}

bool GLWindow::SetCurrentGlTexture(const size_t tex_id) {
  if (tex_id < texture_ids_.size()) {
    current_tex_id_ = tex_id;
    return true;
  }
  return false;
}

size_t GLWindow::CreateBuffer(const size_t width, const size_t height,
                              const size_t channel) {
  const size_t buffer_id = gui_param_.pImageBuffers.size();

  gui_param_.pImageBuffers.emplace_back(new ImageBuffer);
  auto &bf = gui_param_.pImageBuffers.back();

  {
    std::lock_guard<std::mutex> lock(bf->mtx);
    bf->buffer.resize(width * height * channel, 0.f);
    bf->width      = width;
    bf->height     = height;
    bf->channel    = channel;
    bf->has_update = true;
  }

  return buffer_id;
}

bool GLWindow::SetCurrentBuffer(const size_t buffer_id) {
  if (buffer_id < gui_param_.pImageBuffers.size()) {
    gui_param_.current_buffer_id = buffer_id;
    return true;
  }
  return false;
}

std::shared_ptr<ImageBuffer> GLWindow::FetchBuffer(const size_t buffer_id) {
  return gui_param_.pImageBuffers.at(buffer_id);
}

void GLWindow::DrawCurrentBuffer(void) {
  if (gui_param_.current_buffer_id >= gui_param_.pImageBuffers.size()) {
    return;
  }
  ImageBuffer *image_buffer =
      gui_param_.pImageBuffers[gui_param_.current_buffer_id].get();
  if (current_tex_id_ >= texture_ids_.size()) {
    return;
  }
  const GLuint tex_id = texture_ids_[current_tex_id_];

  glUseProgram(shader_program_id_);
  // vertex data
  const float vertex_position[] = {1.f, 1.f, -1.f, 1.f, -1.f, -1.f, 1.f, -1.f};
  const GLfloat vertex_uv[]     = {1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 1.f, 1.f};

  // Fetch attributes location
  const GLint position_location =
      glGetAttribLocation(shader_program_id_, "position");
  const GLint uv_location = glGetAttribLocation(shader_program_id_, "uv");
  const GLint texture_location =
      glGetUniformLocation(shader_program_id_, "texture");

  // Enable attributes
  glEnableVertexAttribArray(GLuint(position_location));
  glEnableVertexAttribArray(GLuint(uv_location));

  // uniform attribute
  glUniform1i(texture_location, 0);
  // register attribute
  glVertexAttribPointer(GLuint(position_location), 2, GL_FLOAT, false, 0,
                        vertex_position);
  glVertexAttribPointer(GLuint(uv_location), 2, GL_FLOAT, false, 0, vertex_uv);

  // Set Buffer
  {
    std::lock_guard<std::mutex> lock(image_buffer->mtx);
    if (image_buffer->has_update) {
      // Transfer buffer to GPU
      glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
      glBindTexture(GL_TEXTURE_2D, tex_id);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, int(image_buffer->width),
                   int(image_buffer->height), 0, GL_RGBA, GL_FLOAT,
                   image_buffer->buffer.data());
      // SetTexture
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

      // unbind TODO need?
      glBindTexture(GL_TEXTURE_2D, 0);
      image_buffer->has_update = false;
    }
  }

  // Draw
  glBindTexture(GL_TEXTURE_2D, tex_id);
  glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

// Determine if the window should be closed
int GLWindow::ShouldClose() const { return glfwWindowShouldClose(window_); }
// Fetch event with swapping color buffer
void GLWindow::SwapBuffers() {
  // swap color buffer
  glfwSwapBuffers(window_);
  // Fetch event
  glfwPollEvents();
}