#ifndef PBRLAB_GUI_PARAMETER_H_
#define PBRLAB_GUI_PARAMETER_H_

struct GuiParameter {
  int width;
  int height;

  float prev_mouse_x = 0.0f, prev_mouse_y = 0.0f;
  bool mouse_left_pressed   = false;
  bool mouse_midd_lePressed = false;
  bool mouse_righ_tPressed  = false;

  float eye[3], lookat[3], up[3];
};

#endif  // PBRLAB_GUI_PARAMETER_H_
