#include <cmath>
#include <cassert>
#include <memory>
#include <vector>
#include <limits>
#include <cstring>
#include <iostream>

#define TINYBVH_IMPLEMENTATION
#include "tinybvh/tiny_bvh.h"

#include "raytracer/mesh.h"
#include "raytracer/raytracer_tinybvh_impl.h"

namespace pbrlab {
namespace raytracer {

const constexpr float kMaxRayDistance = std::numeric_limits<float>::infinity();

TinyBVHRaytracer::TinyBVHRaytracer(void) {
  global_bvh_.reset(new tinybvh::BVH());
}

TinyBVHRaytracer::~TinyBVHRaytracer() {
  // Cleanup handled by unique_ptr destructors
}

uint32_t TinyBVHRaytracer::CreateLocalScene(void) {
  const uint32_t local_scene_id = uint32_t(local_scenes_.size());
  local_scenes_.emplace_back();
  local_scenes_.back().bvh.reset(new tinybvh::BVH());
  return local_scene_id;
}

uint32_t TinyBVHRaytracer::CreateInstanceFromLocalScene(
    uint32_t local_scene_id, const float transform[4][4]) {
  
  InstanceData instance;
  instance.local_scene_id = local_scene_id;
  
  // Copy transform matrix
  for (size_t i = 0; i < 4; i++) {
    for (size_t j = 0; j < 4; j++) {
      if (transform == nullptr) {
        instance.transform[i][j] = (i == j) ? 1.0f : 0.0f;
      } else {
        instance.transform[i][j] = transform[i][j];
      }
    }
  }
  
  const uint32_t instance_id = create_instance_counter_;
  instances_.push_back(instance);
  create_instance_counter_++;
  
  global_scene_needs_rebuild_ = true;
  
  return instance_id;
}

uint32_t TinyBVHRaytracer::AddTriangleMeshToLocalScene(
    const uint32_t local_scene_id, const float* vertices,
    const uint32_t num_vertices, const uint32_t* faces,
    const uint32_t num_faces) {

  const uint32_t triangle_mesh_id = static_cast<uint32_t>(triangle_meshes_.size());

  triangle_meshes_.emplace_back();
  TriangleMesh& triangle_mesh = triangle_meshes_.back();
  triangle_mesh.num_vertices = num_vertices;
  triangle_mesh.num_faces = num_faces;
  triangle_mesh.vertices = vertices;
  triangle_mesh.faces = faces;

  TinyBVHLocalScene& local_scene = local_scenes_.at(local_scene_id);
  const uint32_t local_geom_id = uint32_t(local_scene.mesh_data_ids.size());
  local_scene.mesh_data_ids.emplace_back(kTriangleMesh, triangle_mesh_id);
  
  // Add triangles to local scene vertex buffer
  for (uint32_t face_idx = 0; face_idx < num_faces; face_idx++) {
    uint32_t i0 = faces[face_idx * 3 + 0];
    uint32_t i1 = faces[face_idx * 3 + 1];
    uint32_t i2 = faces[face_idx * 3 + 2];
    
    // Add triangle vertices (tinybvh expects triangles as 3 consecutive vertices)
    tinybvh::bvhvec4 v0{vertices[i0 * 4 + 0], vertices[i0 * 4 + 1], vertices[i0 * 4 + 2], 0.0f};
    tinybvh::bvhvec4 v1{vertices[i1 * 4 + 0], vertices[i1 * 4 + 1], vertices[i1 * 4 + 2], 0.0f};
    tinybvh::bvhvec4 v2{vertices[i2 * 4 + 0], vertices[i2 * 4 + 1], vertices[i2 * 4 + 2], 0.0f};
    
    local_scene.triangle_vertices.push_back(v0);
    local_scene.triangle_vertices.push_back(v1);
    local_scene.triangle_vertices.push_back(v2);
  }
  
  local_scene.needs_rebuild = true;
  global_scene_needs_rebuild_ = true;

  return local_geom_id;
}

uint32_t TinyBVHRaytracer::AddCubicBezierCurveMeshToLocalScene(
    const uint32_t local_scene_id, const float* vertices_thickness,
    const uint32_t num_vertices, const uint32_t* indices,
    const uint32_t num_segments) {
    
  // For now, we don't support curves in tinybvh, so this is a stub
  // In a real implementation, you'd need to tessellate curves into triangles
  std::cerr << "Warning: Cubic Bezier curve meshes not yet supported with tinybvh backend" << std::endl;
  
  const uint32_t cubic_bezier_curve_mesh_id = uint32_t(cubic_bezier_curve_meshes_.size());
  cubic_bezier_curve_meshes_.emplace_back();
  CubicBezierCurveMesh& curve_mesh = cubic_bezier_curve_meshes_.back();
  curve_mesh.num_vertices = num_vertices;
  curve_mesh.num_segments = num_segments;
  curve_mesh.vertices_thickness = vertices_thickness;
  curve_mesh.indices = indices;

  TinyBVHLocalScene& local_scene = local_scenes_.at(local_scene_id);
  const uint32_t local_geom_id = uint32_t(local_scene.mesh_data_ids.size());
  local_scene.mesh_data_ids.emplace_back(kCubicBezierCurveMesh, cubic_bezier_curve_mesh_id);

  return local_geom_id;
}

void TinyBVHRaytracer::RebuildScene() {
  if (!global_scene_needs_rebuild_) return;
  
  global_triangle_vertices_.clear();
  triangle_to_instance_map_.clear();
  triangle_to_local_geom_map_.clear();
  triangle_to_local_prim_map_.clear();
  
  // Build local scene BVHs first
  for (auto& local_scene : local_scenes_) {
    if (local_scene.needs_rebuild && !local_scene.triangle_vertices.empty()) {
      const uint32_t num_triangles = uint32_t(local_scene.triangle_vertices.size() / 3);
      local_scene.bvh->Build(local_scene.triangle_vertices.data(), num_triangles);
      local_scene.needs_rebuild = false;
    }
  }
  
  // Now build global scene by collecting all transformed triangles
  for (size_t instance_idx = 0; instance_idx < instances_.size(); instance_idx++) {
    const InstanceData& instance = instances_[instance_idx];
    const TinyBVHLocalScene& local_scene = local_scenes_[instance.local_scene_id];
    
    // Transform and add all triangles from this instance
    for (size_t tri_idx = 0; tri_idx < local_scene.triangle_vertices.size(); tri_idx += 3) {
      for (int i = 0; i < 3; i++) {
        const tinybvh::bvhvec4& v = local_scene.triangle_vertices[tri_idx + i];
        
        // Apply transform matrix
        tinybvh::bvhvec4 tv;
        tv.x = instance.transform[0][0] * v.x + instance.transform[0][1] * v.y + 
               instance.transform[0][2] * v.z + instance.transform[0][3];
        tv.y = instance.transform[1][0] * v.x + instance.transform[1][1] * v.y + 
               instance.transform[1][2] * v.z + instance.transform[1][3];
        tv.z = instance.transform[2][0] * v.x + instance.transform[2][1] * v.y + 
               instance.transform[2][2] * v.z + instance.transform[2][3];
        tv.w = 0.0f;
        
        global_triangle_vertices_.push_back(tv);
      }
      
      // Map global triangle back to instance data
      const uint32_t global_tri_id = uint32_t(triangle_to_instance_map_.size());
      triangle_to_instance_map_.push_back(uint32_t(instance_idx));
      // For now, just map to first geometry in the local scene
      triangle_to_local_geom_map_.push_back(0);
      triangle_to_local_prim_map_.push_back(uint32_t(tri_idx / 3));
    }
  }
  
  // Build global BVH
  if (!global_triangle_vertices_.empty()) {
    const uint32_t num_triangles = uint32_t(global_triangle_vertices_.size() / 3);
    global_bvh_->Build(global_triangle_vertices_.data(), num_triangles);
  }
  
  global_scene_needs_rebuild_ = false;
}

bool TinyBVHRaytracer::GetSceneAABB(float* bmin, float* bmax) const {
  if (global_triangle_vertices_.empty()) return false;
  
  bmin[0] = bmin[1] = bmin[2] = std::numeric_limits<float>::max();
  bmax[0] = bmax[1] = bmax[2] = -std::numeric_limits<float>::max();
  
  for (const auto& v : global_triangle_vertices_) {
    bmin[0] = std::min(bmin[0], v.x);
    bmin[1] = std::min(bmin[1], v.y);
    bmin[2] = std::min(bmin[2], v.z);
    bmax[0] = std::max(bmax[0], v.x);
    bmax[1] = std::max(bmax[1], v.y);
    bmax[2] = std::max(bmax[2], v.z);
  }
  
  return true;
}

static void Normalize(float* v) {
  const float inv_norm = 1.0f / std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
  v[0] *= inv_norm;
  v[1] *= inv_norm;
  v[2] *= inv_norm;
}

TraceResult TinyBVHRaytracer::FirstHitTrace1(const float* ray_org, const float* ray_dir,
                                             const float min_t, const float max_t) const {
  const_cast<TinyBVHRaytracer*>(this)->RebuildScene();
  
  TraceResult result;
  
  if (global_triangle_vertices_.empty()) {
    return result;
  }
  
  tinybvh::Ray ray;
  ray.O = tinybvh::bvhvec3(ray_org[0], ray_org[1], ray_org[2]);
  ray.D = tinybvh::bvhvec3(ray_dir[0], ray_dir[1], ray_dir[2]);
  ray.hit.t = std::min(max_t, kMaxRayDistance);
  
  // Intersect with global BVH
  int32_t prim_id = global_bvh_->Intersect(ray);
  
  if (prim_id >= 0 && ray.hit.t >= min_t && ray.hit.t < max_t) {
    // We have a hit
    result.t = ray.hit.t;
    result.u = ray.hit.u;
    result.v = ray.hit.v;
    result.prim_id = uint32_t(prim_id);
    
    // Map back to instance/geometry IDs
    if (prim_id < triangle_to_instance_map_.size()) {
      result.instance_id = triangle_to_instance_map_[prim_id];
      result.geom_id = triangle_to_local_geom_map_[prim_id];
    }
    
    // Calculate geometric normal from triangle vertices
    const uint32_t tri_base = prim_id * 3;
    if (tri_base + 2 < global_triangle_vertices_.size()) {
      const tinybvh::bvhvec4& v0 = global_triangle_vertices_[tri_base + 0];
      const tinybvh::bvhvec4& v1 = global_triangle_vertices_[tri_base + 1];
      const tinybvh::bvhvec4& v2 = global_triangle_vertices_[tri_base + 2];
      
      // Compute triangle normal via cross product
      float edge1[3] = {v1.x - v0.x, v1.y - v0.y, v1.z - v0.z};
      float edge2[3] = {v2.x - v0.x, v2.y - v0.y, v2.z - v0.z};
      
      result.normal_g[0] = edge1[1] * edge2[2] - edge1[2] * edge2[1];
      result.normal_g[1] = edge1[2] * edge2[0] - edge1[0] * edge2[2];
      result.normal_g[2] = edge1[0] * edge2[1] - edge1[1] * edge2[0];
      
      Normalize(result.normal_g);
    }
  }
  
  return result;
}

bool TinyBVHRaytracer::AnyHit1(const float* ray_org, const float* ray_dir,
                               const float min_t, const float max_t) const {
  const_cast<TinyBVHRaytracer*>(this)->RebuildScene();
  
  if (global_triangle_vertices_.empty()) {
    return false;
  }
  
  tinybvh::Ray ray;
  ray.O = tinybvh::bvhvec3(ray_org[0], ray_org[1], ray_org[2]);
  ray.D = tinybvh::bvhvec3(ray_dir[0], ray_dir[1], ray_dir[2]);
  ray.hit.t = std::min(max_t, kMaxRayDistance);
  
  int32_t prim_id = global_bvh_->Intersect(ray);
  
  return (prim_id >= 0 && ray.hit.t >= min_t && ray.hit.t < max_t);
}

}  // namespace raytracer
}  // namespace pbrlab