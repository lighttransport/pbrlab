#ifndef PBRLAB_iMGUI_PARAMETER_H_
#define PBRLAB_iMGUI_PARAMETER_H_

//#include <uchar.h>

#include "material-param.h"

struct ImGuiParameter {
  size_t mtl_idx = 0;
  std::vector<std::string> mtl_names;

  const char *current_material = nullptr;

  const std::vector<std::string> mtl_type_names = {"Cycles Principled BSDF",
                                                   "Hair BSDF"};
  const char *current_material_type             = nullptr;

  const char *current_hair_coloring                  = nullptr;
  const std::vector<std::string> hair_coloring_names = {"RGB", "Melanin"};
};

#endif  // PBRLAB_iMGUI_PARAMETER_H_
