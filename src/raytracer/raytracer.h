#ifndef PBRLAB_RAYTRACER_H_
#define PBRLAB_RAYTRACER_H_

#include <stdint.h>

#include <memory>

namespace pbrlab {
struct TraceResult {
  float normal_g[3]    = {1.0f, 0.0f, 0.0f};
  float t              = 1.0f;
  float u              = 0.0f;
  float v              = 0.0f;
  uint32_t instance_id = static_cast<uint32_t>(-1);  // instance id
  uint32_t geom_id     = static_cast<uint32_t>(-1);  // local geom id
  uint32_t prim_id     = static_cast<uint32_t>(-1);  // primitive id
};

namespace raytracer {

enum GeometryType { kTriangleMesh = 0, kCubicBezierCurveMesh, kNone };

struct TraceOption {
  // TODO
};

class Raytracer {
public:
  Raytracer(void);
  ~Raytracer();

  /**
   * @fn
   * @brief register new triangle mesh. the buffer is shared
   * @param[in] vertices vertices(xyz1 * num_vertices. padded)
   * @param[in] num_vertices num of vertices
   * @param[in] faces faces id (id0id1id2 * num_faces)
   * @param[in] num_faces num of faces
   * @param[in] transform 4 x 4 matrix(sim3)
   * @param[out] instance_id Triangle's instance id
   * @param[out] local_scene_id the scene id include only this triangle mesh
   * @param[out] local_geom_id geom id in the local scene
   */
  void RegisterNewTriangleMesh(const float* vertices,
                               const uint32_t num_vertices,
                               const uint32_t* faces, const uint32_t num_faces,
                               const float transform[4][4],
                               uint32_t* instance_id, uint32_t* local_scene_id,
                               uint32_t* local_geom_id);
  /**
   * @fn
   * @brief register new cubic bezier curve mesh. the buffer is shared
   * @param[in] vertices_thickness 4(xyz+thickness) * num vertices_
   * @param[in] num_vertices num of vertices
   * @param[in] indices indices
   * @param[in] num_segments number of segment
   * @param[in] transform 4 x 4 matrix(sim3)
   * @param[out] instance_id Triangle's instance id
   * @param[out] local_scene_id the scene id include only this triangle mesh
   * @param[out] local_geom_id geom id in the local scene
   */
  void RegisterNewCubicBezierCurveMesh(
      const float* vertices_thickness, const uint32_t num_vertices,
      const uint32_t* indices, const uint32_t num_segments,
      const float transform[4][4], uint32_t* instance_id,
      uint32_t* local_scene_id, uint32_t* local_geom_id);

  /**
   * @fn
   * @brief get scene aabb
   * @param[out]  bmin aabb min
   * @param[out]  bmax aabb max
   */
  bool GetSceneAABB(float* bmin, float* bmax) const;

  /**
   * @fn
   * @brief get first hit
   * @param[in]  ray_org ray origin
   * @param[in]  ray_dir ray direction
   * @param[in]  min_t ray min distance
   * @param[in]  max_t ray max distance
   */
  TraceResult FirstHitTrace1(const float* ray_org, const float* ray_dir,
                             const float min_t, const float max_t) const;

  /**
   * @fn
   * @brief get any hit
   * @param[in]  ray_org ray origin
   * @param[in]  ray_dir ray direction
   * @param[in]  min_t ray min distance
   * @param[in]  max_t ray max distance
   */
  bool AnyHit1(const float* ray_org, const float* ray_dir, const float min_t,
               const float max_t) const;

private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};
}  // namespace raytracer
}  // namespace pbrlab

#endif  // PBRLAB_RAYTRACER_H_
