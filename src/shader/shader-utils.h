#ifndef PBRLAB_SHADER_UTILS_H_
#define PBRLAB_SHADER_UTILS_H_
#include <math.h>

#include "material-param.h"
#include "pbrlab_math.h"
#include "ray.h"
#include "scene.h"
#include "type.h"

namespace pbrlab {
struct SurfaceInfo {
  float u;
  float v;

  uint32_t instance_id = static_cast<uint32_t>(-1);  // instance id
  uint32_t geom_id     = static_cast<uint32_t>(-1);  // local geom id
  uint32_t prim_id     = static_cast<uint32_t>(-1);  // primitive id

  float3 global_position;

  float3 normal_s;
  float3 normal_g;
  float3 tangent;    // TODO optional
  float3 bitangent;  // TODO optional

  // TODO texcoord

  enum FaceDirection { kFront, kBack, kAmbiguous };
  FaceDirection face_direction;

  const MaterialParameter* material_param = nullptr;
};

// https://graphics.pixar.com/library/OrthonormalB/paper.pdf
inline void BranchlessONB(const float3& n, float3* x, float3* y) {
  const float sign = copysignf(1.0f, n[2]);
  const float a    = -1.0f / (sign + n[2]);
  const float b    = n[0] * n[1] * a;
  *x = float3(1.0f + sign * n[0] * n[0] * a, sign * b, -sign * n[0]);
  *y = float3(b, sign + n[1] * n[1] * a, -n[1]);
}

inline bool CheckONB(const float3& ex, const float3& ey, const float3& ez) {
  bool ret        = true;
  const float eps = kEps;
  ret             = (ret && (std::abs(vlength(ex) - 1.0f) < eps));
  ret             = (ret && (std::abs(vlength(ey) - 1.0f) < eps));
  ret             = (ret && (std::abs(vlength(ez) - 1.0f) < eps));

  ret = (ret && (vlength(vcross(ex, ey) - ez)) < eps);
  ret = (ret && (vlength(vcross(ey, ez) - ex)) < eps);
  ret = (ret && (vlength(vcross(ez, ex) - ey)) < eps);

  return ret;
}

inline void GrobalToShadingLocal(const float3& ex, const float3& ey,
                                 const float3& ez, float mat[4][4]) {
  // v' = v * M

  mat[0][0] = ex[0];
  mat[0][1] = ey[0];
  mat[0][2] = ez[0];
  mat[0][3] = 0.0f;

  mat[1][0] = ex[1];
  mat[1][1] = ey[1];
  mat[1][2] = ez[1];
  mat[1][3] = 0.0f;

  mat[2][0] = ex[2];
  mat[2][1] = ey[2];
  mat[2][2] = ez[2];
  mat[2][3] = 0.0f;

  mat[3][0] = 0.0f;
  mat[3][1] = 0.0f;
  mat[3][2] = 0.0f;
  mat[3][3] = 1.0f;
}

inline void ShadingLocalToGlobal(const float3& ex, const float3& ey,
                                 const float3& ez, float mat[4][4]) {
  // v' = v * M

  mat[0][0] = ex[0];
  mat[0][1] = ex[1];
  mat[0][2] = ex[2];
  mat[0][3] = 0.0f;

  mat[1][0] = ey[0];
  mat[1][1] = ey[1];
  mat[1][2] = ey[2];
  mat[1][3] = 0.0f;

  mat[2][0] = ez[0];
  mat[2][1] = ez[1];
  mat[2][2] = ez[2];
  mat[2][3] = 0.0f;

  mat[3][0] = 0.0f;
  mat[3][1] = 0.0f;
  mat[3][2] = 0.0f;
  mat[3][3] = 1.0f;
}

inline bool ShadowRay(const Scene& scene, const float3& pos, const float3& dir,
                      const float dist) {
  Ray ray;
  ray.ray_org = pos;
  ray.ray_dir = dir;

  ray.min_t = kEps;
  ray.max_t = std::max(ray.min_t, dist - kEps);

  return scene.AnyHit1(ray);
}
}  // namespace pbrlab
#endif  // PBRLAB_SHADER_UTILS_H_
