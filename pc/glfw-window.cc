#include "glfw-window.h"

#include <stdio.h>
#include <stdlib.h>

#include <sstream>
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

#ifdef __APPLE__
#ifdef __clang__
// Suppress deprecated warning of using legacy OpenGL API
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif
#endif

#define CHECK_GL(tag) do { \
  GLenum err = glGetError(); \
  if (err != GL_NO_ERROR) { \
    std::cerr << "OpenGL err: " << __FILE__ << ":" << __LINE__ << ":" << __func__ << " code = " << err << ", tag = " << tag << "\n"; \
  } \
} while(0)


static void CreateViewport(GLFWwindow *window, int w, int h) {
  CHECK_GL("CreateViewport begin");
  int fb_w, fb_h;
  // Get actual framebuffer size.
  glfwGetFramebufferSize(window, &fb_w, &fb_h);
  glViewport(0, 0, fb_w, fb_h);
#if 0 // not available(returns glError) on GL 3.2+ Core profile
  CHECK_GL("glViewport");
  glMatrixMode(GL_PROJECTION);
  CHECK_GL("glProjection");
  glLoadIdentity();
  gluPerspective(45.0, double(w) / double(h), 0.01, 1000.0);
  CHECK_GL("gluPerspective");
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
#endif

  CHECK_GL("CreateViewport");

  auto *param =
      reinterpret_cast<GuiParameter *>(glfwGetWindowUserPointer(window));

  param->width  = w;
  param->height = h;
}

static void ReshapeFunc(GLFWwindow *window, int w, int h) {
  // TODO centering

  // int fb_w, fb_h;
  // // Get actual framebuffer size.
  // glfwGetFramebufferSize(window, &fb_w, &fb_h);
  // glViewport(0, 0, fb_w, fb_h);
  // glMatrixMode(GL_PROJECTION);
  // glLoadIdentity();
  // gluPerspective(45.0, double(w) / double(h), 0.01, 100.0);
  // glMatrixMode(GL_MODELVIEW);
  // glLoadIdentity();

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

static void InitializeImgui(GLFWwindow *window, const char *glsl_version) {

  if (glsl_version == nullptr) {
    std::cerr << "Invalid glsl_version\n";
    exit(-1);
  }

  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();

  ImGui::CreateContext();
  auto &io = ImGui::GetIO();

  // Read .ini file from parent directory if imgui.ini does not exist in the
  // current directory.
  if (fs::exists("../imgui.ini") && !fs::exists("./imgui.ini")) {
    printf("Use ../imgui.ini as Init file.\n");
    io.IniFilename = "../imgui.ini";
  }

  //io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
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
  std::cout << "Init OpenGL3 with GLSL version: " << glsl_version << "\n";
  ImGui_ImplOpenGL3_Init(glsl_version);

  CHECK_GL("ImplOpenGL3Init");
}

static GLuint CreateGlShader(const char *glsl_version) {
  // Assume GL 3.0+
  // Compile vertex shader
  GLuint v_shader_id       = glCreateShader(GL_VERTEX_SHADER);
  std::string vertexShader = R"#(
    in vec2 position;
    in vec2 uv;
    out vec2 vuv;
    void main(void){
        gl_Position = vec4(position, 0.0, 1.0);
        vuv = uv;
    }
    )#";

  // append version pragma
  vertexShader = std::string(glsl_version) + "\n" + vertexShader;
  const char *vs           = vertexShader.c_str();
  glShaderSource(v_shader_id, 1, &vs, nullptr);
  glCompileShader(v_shader_id);

  GLint isCompiled = 0;
  glGetShaderiv(v_shader_id, GL_COMPILE_STATUS, &isCompiled);
  if(isCompiled == GL_FALSE) {

    GLint maxLength = 0;
    glGetShaderiv(v_shader_id, GL_INFO_LOG_LENGTH, &maxLength);

    // The maxLength includes the NULL character
    std::vector<GLchar> errorLog(maxLength);
    glGetShaderInfoLog(v_shader_id, maxLength, &maxLength, &errorLog[0]);

    // Provide the infolog in whatever manor you deem best.
    // Exit with failure.
    glDeleteShader(v_shader_id); // Don't leak the shader.

    std::cerr << "Compile vertex shader failed: " << std::string(errorLog.data(), errorLog.size()) << "\n";
    exit(-1);
  }

  CHECK_GL("Compile vertx shader");

  // Compile fragment shader
  GLuint f_shader_id         = glCreateShader(GL_FRAGMENT_SHADER);
  std::string fragmentShader = R"#(
    in vec2 vuv;
    uniform sampler2D u_texture;
    out vec4 fc;

    void main(void){
        fc = texture(u_texture, vuv);
    }
    )#";

  fragmentShader = std::string(glsl_version) + "\n" + fragmentShader;
  const char *fs             = fragmentShader.c_str();
  glShaderSource(f_shader_id, 1, &fs, nullptr);
  glCompileShader(f_shader_id);

  glGetShaderiv(f_shader_id, GL_COMPILE_STATUS, &isCompiled);
  if(isCompiled == GL_FALSE) {

    GLint maxLength = 0;
    glGetShaderiv(f_shader_id, GL_INFO_LOG_LENGTH, &maxLength);

    // The maxLength includes the NULL character
    std::vector<GLchar> errorLog(maxLength);
    glGetShaderInfoLog(f_shader_id, maxLength, &maxLength, &errorLog[0]);

    // Provide the infolog in whatever manor you deem best.
    // Exit with failure.
    glDeleteShader(f_shader_id); // Don't leak the shader.

    std::cerr << "Compile fragment shader failed: " << std::string(errorLog.data(), errorLog.size()) << "\n";
    exit(-1);
  }

  CHECK_GL("Compile fragment shader");

  // Create program object
  GLuint program_id = glCreateProgram();
  std::cout << "prog_id " << program_id << "\n";
  glAttachShader(program_id, v_shader_id);
  glAttachShader(program_id, f_shader_id);

  glBindFragDataLocation(program_id, 0, "fc");

  // link
  glLinkProgram(program_id);
  CHECK_GL("Link program");

  GLint isLinked;
  glGetProgramiv(program_id, GL_LINK_STATUS, (int *)&isLinked);
  if (isLinked != GL_TRUE) {
    std::cerr << "Failed to link program\n";

    GLint maxLength = 0;
    glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &maxLength);

    // The maxLength includes the NULL character
    std::vector<GLchar> errorLog(maxLength);
    glGetProgramInfoLog(program_id, maxLength, &maxLength, &errorLog[0]);

    glDeleteProgram(program_id);

    std::cerr << "Link shaders failed: " << std::string(errorLog.data(), errorLog.size()) << "\n";
    exit(-1);
    
  }


  return program_id;
}

GLWindow::GLWindow(int width, int height, const char *glsl_version, const char *title)
    : window_(glfwCreateWindow(width, height, title, nullptr, nullptr)) {
  // Create Window
  if (window_ == nullptr) {
    fprintf(stderr, "failed creating GLFW window.\n");
    exit(EXIT_FAILURE);
  }
  fprintf(stderr, "success openning GLFW.\n");

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

  // make this window target
  glfwMakeContextCurrent(window_);

#if 1 
  if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
  {
    std::cerr << "Failed to initialize glad\n";
    glfwDestroyWindow(window_);
    exit(EXIT_FAILURE);
  }
  
  std::cout << "OpenGL version: " << GLVersion.major << '.' << GLVersion.minor << "\n";

  if (GLVersion.major < 3) {
    fprintf(stderr, "OpenGL 3.x context not available.\n");
    glfwDestroyWindow(window_);
    exit(EXIT_FAILURE);
  }

  std::cout << "initialize gl\n";
#endif

  // Set Buffer Swap Timing
  glfwSwapInterval(1);

  InitializeImgui(window_, glsl_version);

  std::cout << "Compiling GL shaders\n";

  // create shader program object
  shader_program_id_ = CreateGlShader(glsl_version);
  if (shader_program_id_ == 0) {
    fprintf(stderr, "faild create shader program object\n");
    exit(EXIT_FAILURE);
  }

  // setup VBA
  std::cout << "creating GL vertex data\n";
  glGenVertexArrays(1, &vba_id_);
  CHECK_GL("GenVertexArrays");
  CreateGLVertexData();

  std::cout << "creating GL viewport\n";

  CreateViewport(window_, width, height);

  std::cout << "DONE GLWindow creation\n";
}

GLWindow::~GLWindow() {
  // Release Texture
  for (const auto ti : texture_ids_) {
    GLuint tmp = GLuint(ti);
    glBindTexture(GL_TEXTURE_2D, 0);
    glDeleteTextures(1, &tmp);
    assert(glGetError() == GL_NO_ERROR);
  }

  // Cleanup
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  glfwDestroyWindow(window_);
}

static GLuint CreateGlTexture(void) {
  GLuint ret;
  glGenTextures(1, &ret);
  CHECK_GL("glGenTextures");
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

void GLWindow::CreateGLVertexData() {

  glUseProgram(shader_program_id_);
  CHECK_GL("glUseProgram " << shader_program_id_);

  // Fetch attributes location
  const GLint position_location =
      glGetAttribLocation(shader_program_id_, "position");
  CHECK_GL("failed to get position attr loc.");
  const GLint uv_location = glGetAttribLocation(shader_program_id_, "uv");
  CHECK_GL("failed to get uv attr loc.");

  glBindVertexArray(vba_id_);
  CHECK_GL("BindVertexArray " << vba_id_);

  // vertex data
  const float vertex_position[] = {1.f, 1.f, -1.f, 1.f, -1.f, -1.f, 1.f, -1.f};
  const GLfloat vertex_uv[]     = {1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 1.f, 1.f};


  glGenBuffers(1, &pos_buffer_id_); 
  glGenBuffers(1, &uv_buffer_id_); 
  CHECK_GL("GenBuffers");

  glBindBuffer(GL_ARRAY_BUFFER, pos_buffer_id_);
  CHECK_GL("BindBuffer pos");
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_position), vertex_position, GL_STATIC_DRAW);
  CHECK_GL("BufferData pos");

  glEnableVertexAttribArray(GLuint(position_location));
  CHECK_GL("EnableVertexAttrib pos");

  glVertexAttribPointer(
        position_location,
        2,
        GL_FLOAT,
        GL_FALSE,
        /* stride */sizeof(float) * 2,
        0
    );
  CHECK_GL("VertexAttribPointer pos");

  glBindBuffer(GL_ARRAY_BUFFER, uv_buffer_id_);
  CHECK_GL("BindBuffer uv");

  glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_uv), vertex_uv, GL_STATIC_DRAW);
  CHECK_GL("BufferData uv");
  
  glEnableVertexAttribArray(GLuint(uv_location));
  CHECK_GL("EnableVertexAttrib uv");

  glVertexAttribPointer(
        uv_location,
        2,
        GL_FLOAT,
        GL_FALSE,
        /* stride */sizeof(float) * 2,
        0
    );
  CHECK_GL("VertexAttribPointer uv");

  glBindVertexArray(0);

}

bool GLWindow::SetCurrentBuffer(const size_t buffer_id) {
  if (buffer_id < gui_param_.pImageBuffers.size()) {
    gui_param_.current_buffer_id = buffer_id;
    return true;
  }
  return false;
}

void GLWindow::SetRenderItem(std::shared_ptr<RenderItem> &pRenderItem) {
  gui_param_.pRenderItem = pRenderItem;
}

std::shared_ptr<ImageBuffer> GLWindow::FetchBuffer(const size_t buffer_id) {
  return gui_param_.pImageBuffers.at(buffer_id);
}

void GLWindow::DrawCurrentBuffer(void) {
  CHECK_GL("DrawCurrentBuffer begin");

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
  CHECK_GL("glUseProgram " << shader_program_id_);

  const GLint texture_location =
      glGetUniformLocation(shader_program_id_, "u_texture");
  CHECK_GL("failed to get u_texture uniform loc.");

  glBindVertexArray(vba_id_);
  CHECK_GL("BindVertexArray " << vba_id_);


  // uniform attribute
  glUniform1i(texture_location, 0);

  // Update texture if required
  {
    std::lock_guard<std::mutex> lock(image_buffer->mtx);
    if (image_buffer->has_update) {
      // Transfer buffer to GPU
      glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
      glBindTexture(GL_TEXTURE_2D, tex_id);
      CHECK_GL("Texture bind: txid = " << tex_id);

      // NOTE: Linear -> sRGB conversion is already done in pbrlab-gui.cc:BufferUpdater().
      // TODO: Use glTexSubImage2D for efficient texture update.
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, int(image_buffer->width),
                   int(image_buffer->height), 0, GL_RGBA, GL_FLOAT,
                   image_buffer->buffer.data());

      CHECK_GL("Texture transfer");

      // SetTexture
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

      CHECK_GL("Texture param set");

      // unbind TODO need?
      glBindTexture(GL_TEXTURE_2D, 0);
      image_buffer->has_update = false;
    }
  }

  // Draw
  glBindTexture(GL_TEXTURE_2D, tex_id);
  glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
  CHECK_GL("DrawArrays");

  glBindVertexArray(0);
  CHECK_GL("BindVertexArray(0)");

  glFlush();
}

static void RequestRerender(GuiParameter *gui_param) {
  gui_param->pRenderItem->cancel_render_flag.store(true);
  gui_param->pRenderItem->last_render_pass.store(0);
  gui_param->pRenderItem->finish_pass.store(0);
}

static void MainUI(GuiParameter *gui_param, bool *rerender) {
  ImGui::Begin("Render");

  const float progress = float(gui_param->pRenderItem->finish_pass.load()) /
                         float(gui_param->pRenderItem->max_pass.load());
  std::stringstream ss;
  ss << "Pass " << gui_param->pRenderItem->finish_pass << " of "
     << gui_param->pRenderItem->max_pass << " (" << progress * 100.f << "%)";
  ImGui::ProgressBar(progress, ImVec2(0.0f, 0.0f), ss.str().c_str());

  int _max_pass = int(gui_param->pRenderItem->max_pass);
  if (ImGui::DragInt("max_pass", &_max_pass, 1.f, 1,
                     (std::numeric_limits<int>::max)())) {
    gui_param->pRenderItem->max_pass = size_t(_max_pass);
    *rerender                        = true;
  }

  if (ImGui::Button("Rerender")) {
    *rerender = true;
  }

  ImGui::End();
}

static void ColorPicker3(const char *label, float *v, bool *update_material) {
  if (ImGui::TreeNodeEx("Color Picker", ImGuiTreeNodeFlags_NoAutoOpenOnLog)) {
    if (ImGui::ColorPicker3(label, v)) {
      *update_material = true;
    }
    ImGui::TreePop();
  }
}

static void CyclesPrincipledBsdfUI(
    pbrlab::CyclesPrincipledBsdfParameter *material_param,
    bool *update_material) {
  if (ImGui::TreeNodeEx("Base", ImGuiTreeNodeFlags_DefaultOpen)) {
    if (ImGui::DragFloat3("base color", material_param->base_color.v, 0.01f,
                          0.0f, 1.f)) {
      *update_material = true;
    }
    ColorPicker3("base color", material_param->base_color.v, update_material);

    if (material_param->base_color_tex_id != uint32_t(-1)) {
      ImGui::Text("use texture");
    }

    if (ImGui::DragFloat("metallic", &(material_param->metallic), 0.01f, 0.f,
                         1.f)) {
      *update_material = true;
    }
    ImGui::TreePop();
  }
  if (ImGui::TreeNodeEx("Subusuface", ImGuiTreeNodeFlags_NoAutoOpenOnLog)) {
    if (ImGui::DragFloat("subsurface", &(material_param->subsurface), 0.01f,
                         0.0f, 1.f)) {
      *update_material = true;
    }
    if (ImGui::DragFloat3("subsurface radius",
                          material_param->subsurface_radius.v, 0.01f, 0.0f)) {
      *update_material = true;
    }
    if (ImGui::DragFloat3("subsurface color",
                          material_param->subsurface_color.v, 0.01f, 0.0f,
                          1.f)) {
      *update_material = true;
    }
    ColorPicker3("subsurface color", material_param->subsurface_color.v,
                 update_material);

    if (material_param->subsurface_color_tex_id != uint32_t(-1)) {
      ImGui::Text("use texture");
    }

    ImGui::TreePop();
  }
  if (ImGui::TreeNodeEx("Specular", ImGuiTreeNodeFlags_NoAutoOpenOnLog)) {
    if (ImGui::DragFloat("specular", &(material_param->specular), 0.01f, 0.0f,
                         1.f)) {
      *update_material = true;
    }
    if (ImGui::DragFloat("specular tint", &(material_param->specular_tint),
                         0.01f, 0.0f, 1.f)) {
      *update_material = true;
    }
    if (ImGui::DragFloat("roughness", &(material_param->roughness), 0.01f, 0.0f,
                         1.f)) {
      *update_material = true;
    }
    if (ImGui::DragFloat("ior", &(material_param->ior), 0.01f, 0.0f)) {
      *update_material = true;
    }
    if (ImGui::DragFloat("anisotropic", &(material_param->anisotropic), 0.01f,
                         0.0f, 1.f)) {
      *update_material = true;
    }
    if (ImGui::DragFloat("anisotropic rotation",
                         &(material_param->anisotropic_rotation), 0.01f, 0.0f,
                         1.f)) {
      *update_material = true;
    }
    ImGui::TreePop();
  }
  if (ImGui::TreeNodeEx("Sheen", ImGuiTreeNodeFlags_NoAutoOpenOnLog)) {
    if (ImGui::DragFloat("sheen", &(material_param->sheen), 0.01f, 0.0f, 1.f)) {
      *update_material = true;
    }
    if (ImGui::DragFloat("sheen tint", &(material_param->sheen_tint), 0.01f,
                         0.0f, 1.f)) {
      *update_material = true;
    }
    ImGui::TreePop();
  }
  if (ImGui::TreeNodeEx("Clearcoat", ImGuiTreeNodeFlags_NoAutoOpenOnLog)) {
    if (ImGui::DragFloat("clearcoat", &(material_param->clearcoat), 0.01f, 0.0f,
                         1.f)) {
      *update_material = true;
    }
    if (ImGui::DragFloat("clearcoat roughness",
                         &(material_param->clearcoat_roughness), 0.01f, 0.0f,
                         1.f)) {
      *update_material = true;
    }
    ImGui::TreePop();
  }
  if (ImGui::TreeNodeEx("Transmission", ImGuiTreeNodeFlags_NoAutoOpenOnLog)) {
    if (ImGui::DragFloat("transmission", &(material_param->transmission), 0.01f,
                         0.0f, 1.f)) {
      *update_material = true;
    }
    if (ImGui::DragFloat("transmission roughness",
                         &(material_param->transmission_roughness), 0.01f, 0.0f,
                         1.f)) {
      *update_material = true;
    }
    ImGui::TreePop();
  }
}
static void HairBsdfUI(ImGuiParameter *imgui_parameter,
                       pbrlab::HairBsdfParameter *material_param,
                       bool *update_material) {
  if (ImGui::TreeNodeEx("Hair Color", ImGuiTreeNodeFlags_DefaultOpen)) {
    auto &current_hair_coloring = imgui_parameter->current_hair_coloring;
    const std::vector<std::string> &hair_coloring_names =
        imgui_parameter->hair_coloring_names;

    pbrlab::HairBsdfParameter::ColoringHair hair_coloring_type =
        pbrlab::HairBsdfParameter::ColoringHair(material_param->coloring_hair);

    current_hair_coloring =
        hair_coloring_names.at(size_t(hair_coloring_type)).c_str();

    if (ImGui::BeginCombo("Hair Coloring", current_hair_coloring)) {
      for (size_t n = 0; n < hair_coloring_names.size(); n++) {
        bool is_selected =
            (current_hair_coloring == hair_coloring_names[n].c_str());

        if (ImGui::Selectable(hair_coloring_names[n].c_str(), is_selected)) {
          current_hair_coloring = hair_coloring_names[n].c_str();
          hair_coloring_type    = pbrlab::HairBsdfParameter::ColoringHair(n);
          material_param->coloring_hair = hair_coloring_type;
          *update_material              = true;
        }
        if (is_selected) {
          ImGui::SetItemDefaultFocus();  // You may set the initial focus when
        }
      }
      ImGui::EndCombo();
    }

    if (hair_coloring_type == pbrlab::HairBsdfParameter::kRGB) {
      if (ImGui::DragFloat3("base color", material_param->base_color.v, 0.01f,
                            0.0f, 1.f)) {
        *update_material = true;
      }
      ColorPicker3("base color", material_param->base_color.v, update_material);
    } else if (hair_coloring_type == pbrlab::HairBsdfParameter::kMelanin) {
      if (ImGui::DragFloat("melanin", &(material_param->melanin), 0.01f, 0.0f,
                           1.f)) {
        *update_material = true;
      }
      if (ImGui::DragFloat("melanin redness",
                           &(material_param->melanin_redness), 0.01f, 0.0f,
                           1.f)) {
        *update_material = true;
      }
      if (ImGui::DragFloat("melanin randomize",
                           &(material_param->melanin_randomize), 0.05f, 0.0f,
                           1.f)) {
        *update_material = true;
      }
    } else {
      assert(false);
    }

    ImGui::TreePop();
  }
  if (ImGui::TreeNodeEx("Other", ImGuiTreeNodeFlags_DefaultOpen)) {
    if (ImGui::DragFloat("roughness", &(material_param->roughness), 0.01f, 0.0f,
                         1.f)) {
      *update_material = true;
    }
    if (ImGui::DragFloat("anisotropic roughness",
                         &(material_param->azimuthal_roughness), 0.01f, 0.0f,
                         1.f)) {
      *update_material = true;
    }
    if (ImGui::DragFloat("ior", &(material_param->ior), 0.01f, 0.0f, 1.f)) {
      *update_material = true;
    }
    if (ImGui::DragFloat("shift", &(material_param->shift), 0.01f, 0.0f, 1.f)) {
      *update_material = true;
    }
  }
  if (ImGui::TreeNodeEx("Tint", ImGuiTreeNodeFlags_DefaultOpen)) {
    if (ImGui::DragFloat3("specular tint", material_param->specular_tint.v,
                          0.01f, 0.0f, 1.f)) {
      *update_material = true;
    }
    ColorPicker3("specular tint", material_param->specular_tint.v,
                 update_material);
    if (ImGui::DragFloat3("second_specular tint",
                          material_param->second_specular_tint.v, 0.01f, 0.0f,
                          1.f)) {
      *update_material = true;
    }
    ColorPicker3("second_specular tint", material_param->second_specular_tint.v,
                 update_material);
    if (ImGui::DragFloat3("transmission tint",
                          material_param->transmission_tint.v, 0.01f, 0.0f,
                          1.f)) {
      *update_material = true;
    }

    ColorPicker3("transmission tint", material_param->transmission_tint.v,
                 update_material);
  }
}

static void MaterialUI(ImGuiParameter *imgui_parameter, pbrlab::Scene *scene,
                       EditQueue *edit_queue, bool *rerender) {
  bool update_material = false;
  std::vector<pbrlab::MaterialParameter> *materials =
      scene->FetchMeshMaterialParameters();
  ImGui::Begin("Material");

  if (materials->size() == 0) {
    ImGui::Text("No materials");
    ImGui::End();
    return;
  }
  if (ImGui::TreeNodeEx("Edit Material", ImGuiTreeNodeFlags_DefaultOpen)) {
    std::vector<std::string> &mtl_names = imgui_parameter->mtl_names;
    mtl_names.resize(materials->size());
    for (size_t i = 0; i < materials->size(); i++) {
      const std::string name = pbrlab::GetMaterialName((*materials)[i]);

      mtl_names[i] = name;
    }

    // Editing material selector
    size_t &mtl_idx = imgui_parameter->mtl_idx;

    auto &current_material = imgui_parameter->current_material;

    if (current_material == nullptr) {
      current_material = mtl_names.at(0).c_str();
    }

    if (ImGui::BeginCombo("material", current_material)) {
      for (size_t n = 0; n < mtl_names.size(); n++) {
        bool is_selected = (current_material == mtl_names[n].c_str());

        if (ImGui::Selectable(mtl_names[n].c_str(), is_selected)) {
          current_material = mtl_names[n].c_str();
          mtl_idx          = n;
        }
        if (is_selected) {
          ImGui::SetItemDefaultFocus();  // You may set the initial focus when
        }
      }
      ImGui::EndCombo();
    }

    if (materials->size() <= mtl_idx) {
      ImGui::End();
      return;
    }

    ImGui::Separator();

    pbrlab::MaterialParameter material = {};
    if (!edit_queue->FetchIfExit(materials->data() + mtl_idx, &material)) {
      material = materials->at(mtl_idx);
    }

    pbrlab::MaterialParameterType mtl_type =
        pbrlab::MaterialParameterType(material.index());

    const std::vector<std::string> &mtl_type_names =
        imgui_parameter->mtl_type_names;
    auto &current_material_type = imgui_parameter->current_material_type;

    current_material_type = mtl_type_names.at(size_t(mtl_type)).c_str();

    if (ImGui::BeginCombo("material type", current_material_type)) {
      for (size_t n = 0; n < mtl_type_names.size(); n++) {
        bool is_selected = (current_material_type == mtl_type_names[n].c_str());

        if (ImGui::Selectable(mtl_type_names[n].c_str(), is_selected)) {
          current_material_type = mtl_type_names[n].c_str();

          pbrlab::MaterialParameterType tmp_mtl_type =
              pbrlab::MaterialParameterType(n);
          if (tmp_mtl_type != mtl_type) {
            mtl_type        = tmp_mtl_type;
            update_material = true;
            if (mtl_type == pbrlab::kCyclesPrincipledBsdfParameter) {
              material = pbrlab::CyclesPrincipledBsdfParameter();
            } else if (mtl_type == pbrlab::kHairBsdfParameter) {
              material = pbrlab::HairBsdfParameter();
            } else {
              assert(false);
            }
          }
        }
        if (is_selected) {
          ImGui::SetItemDefaultFocus();  // You may set the initial focus when
        }
      }
      ImGui::EndCombo();
    }

    if (mtl_type == pbrlab::kCyclesPrincipledBsdfParameter) {
      pbrlab::CyclesPrincipledBsdfParameter &cycles_material =
          mpark::get<pbrlab::kCyclesPrincipledBsdfParameter>(material);
      CyclesPrincipledBsdfUI(&cycles_material, &update_material);
    } else if (mtl_type == pbrlab::kHairBsdfParameter) {
      pbrlab::HairBsdfParameter &hair_material =
          mpark::get<pbrlab::kHairBsdfParameter>(material);
      HairBsdfUI(imgui_parameter, &hair_material, &update_material);
    } else {
      assert(false);
    }

    if (update_material) {
      *rerender = true;
      edit_queue->Push(&material, &(materials->at(mtl_idx)));
    }
    ImGui::TreePop();
  }

  ImGui::End();
}

void GLWindow::DrawImguiUI(void) {
  // start
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  bool rerender = false;
  // draw some window
  MainUI(&gui_param_, &rerender);
  MaterialUI(&(gui_param_.imgui_parameter), &(gui_param_.pRenderItem->scene),
             &(gui_param_.pRenderItem->edit_queue), &rerender);

  // end
  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

  if (rerender) {
    RequestRerender(&gui_param_);
  }
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
