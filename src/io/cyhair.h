#ifndef PBRLAB_CYHAIR_H_
#define PBRLAB_CYHAIR_H_

#include "type.h"

namespace pbrlab {

struct CyHairHeader {
  char magic[4];
  unsigned int num_strands;
  unsigned int total_points;
  unsigned int flags;
  unsigned int default_segments;
  float default_thickness;
  float default_transparency;
  float default_color[3];
  char infomation[88];
};

class CyHair {
public:
  CyHair(void);

  ~CyHair(void);

  /// Load CyHair data from a file.
  bool Load(const std::string &filepath);

  CyHairHeader header_;

  // Raw CyHair values
  std::vector<unsigned short> segments_;
  std::vector<float> points_;  // xyz
  std::vector<float> thicknesses_;
  std::vector<float> transparencies_;
  std::vector<float> colors_;  // rgb
  unsigned int flags_;
  unsigned int num_strands_;
  unsigned int total_points_;
  int default_segments_;
  float default_thickness_;
  float default_transparency_;
  float default_color_[3];
  int pad0;

  // Processed CyHair values
  std::vector<unsigned int> strand_offsets_;
};

bool LoadCyHair(const std::string &filepath, const bool is_y_up,
                std::vector<std::vector<float>> *vertices,
                std::vector<std::vector<float>> *thicknesses);
}  // namespace pbrlab

#endif  // PBRLAB_CYHAIR_H_
