#ifndef PBRLAB_RAY_H_
#define PBRLAB_RAY_H_

#include "pbrlab_math.h"
#include "type.h"

namespace pbrlab {

struct Ray {
  float3 ray_dir;
  float3 ray_org;
  float min_t = 0.0f;
  float max_t = kInf;
};

}  // namespace pbrlab
#endif  // PBRLAB_RAY_H_
