#ifndef PBRLAB_GUI_PARAMETER_H_
#define PBRLAB_GUI_PARAMETER_H_
#include <memory>
#include <mutex>
#include <vector>

struct ImageBuffer {
  std::vector<float> buffer;
  size_t width    = size_t(-1);
  size_t height   = size_t(-1);
  size_t channel  = size_t(-1);
  bool has_update = false;
  std::mutex mtx;
};

struct GuiParameter {
  int width;
  int height;

  float prev_mouse_x = 0.0f, prev_mouse_y = 0.0f;
  bool mouse_left_pressed   = false;
  bool mouse_midd_lePressed = false;
  bool mouse_righ_tPressed  = false;

  float eye[3], lookat[3], up[3];

  size_t current_buffer_id = size_t(-1);
  std::vector<std::shared_ptr<ImageBuffer>> pImageBuffers;
};

#endif  // PBRLAB_GUI_PARAMETER_H_
