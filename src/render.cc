#include "render.h"

#include <atomic>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>
#include <tuple>
#include <vector>

#include "light-manager.h"
#include "pbrlab-util.h"
#include "random/rng.h"
#include "render-tile.h"
#include "scene.h"
#include "shader/shader-utils.h"
#include "shader/shader.h"

namespace pbrlab {
void Normalize(float* v);

float3 GetRadiance(const Ray& input_ray, const Scene& scene, const RNG& rng);

float3 GetRadiance(const Ray& input_ray, const Scene& scene, const RNG& rng) {
  Ray ray = input_ray;
  float3 contribution(0.0f);
  float3 throuput(1.0f);
  float bsdf_sampling_pdf = 0.f;

  for (uint32_t depth = 0;; depth++) {
    if (IsBlack(throuput)) break;
    const TraceResult trace_result = scene.TraceFirstHit1(ray);

    if (trace_result.instance_id == static_cast<uint32_t>(-1)) {
      // TODO
      break;
    }

    SurfaceInfo surface_info =
        TraceResultToSufaceInfo(ray, scene, trace_result);

    {
      if (surface_info.face_direction == SurfaceInfo::kFront) {
        float3 emission(0.f);
        float pdf_area                    = 0.f;
        const LightManager* light_manager = scene.GetLightManager();
        const bool has_emission           = light_manager->ImplicitAreaLight(
            trace_result.instance_id, trace_result.geom_id,
            trace_result.prim_id, &emission, &pdf_area);
        if (has_emission) {
          const float area_to_solid_angle =
              abs((trace_result.t * trace_result.t) /
                  vdot(surface_info.normal_s, ray.ray_dir));

          const float weight =
              (depth == 0)
                  ? 1.0f
                  : PowerHeuristicWeight(bsdf_sampling_pdf,
                                         pdf_area * area_to_solid_angle);
          contribution = contribution + weight * emission * throuput;
        }
      }
    }

    // start russian roulette
    const float russian_roulette_p = SpectrumNorm(throuput);
    if (russian_roulette_p < rng.Draw()) break;
    throuput = throuput * float3(1.0f / russian_roulette_p);
    // end russian roulette

    // start shadeing and sampling nextray
    float3 next_ray_dir;
    float3 r_throuput;
    float3 d_contribute;
    float _bsdf_sampling_pdf;
    Shader(scene, -ray.ray_dir, rng, &surface_info, &next_ray_dir, &r_throuput,
           &d_contribute, &_bsdf_sampling_pdf);

    contribution      = contribution + throuput * d_contribute;
    throuput          = r_throuput * throuput;
    bsdf_sampling_pdf = _bsdf_sampling_pdf;

    ray.ray_org = surface_info.global_position;
    ray.ray_dir = next_ray_dir;
    ray.min_t   = 1e-3f;
    ray.max_t   = kInf;
    // end shadeing and sampling nextray
  }
  return contribution;
}

static bool PrepareRendering(
    const Scene& scene, const uint32_t width, const uint32_t height,
    const uint32_t num_sample, RenderLayer* layer,
    std::vector<std::unique_ptr<RenderTile>>* render_tiles,
    std::vector<std::unique_ptr<std::atomic<uint32_t>>>*
        finished_jobs_counters) {
  // clear layer
  layer->Resize(width, height);
  layer->Clear();

  // TODO scene commit if it's nesessary
  (void)scene;

  // prepare render tile
  {
    const uint32_t kTileWidth  = 64;
    const uint32_t kTileHeight = 64;

    CreateTiles(uint32_t(layer->width), uint32_t(layer->height), kTileWidth,
                kTileHeight, render_tiles);
  }

  // prepare finish jobs counter
  {
    finished_jobs_counters->clear();
    finished_jobs_counters->reserve(num_sample);
    for (size_t i = 0; i < num_sample; ++i) {
      finished_jobs_counters->emplace_back(new std::atomic<uint32_t>(0));
    }
    return true;
  }
}

static uint32_t RenderingTile(const Scene& scene, const uint32_t num_sample,
                              const RenderTile& render_tile, const RNG& rng,
                              RenderLayer* layer,
                              std::atomic<uint32_t>* finished_jobs_counter) {
  assert(render_tile.sx < render_tile.tx && render_tile.tx <= layer->width);
  assert(render_tile.sy < render_tile.ty && render_tile.ty <= layer->height);

  float bmax[3], bmin[3];
  scene.FetchSceneAABB(bmin, bmax);

  // fov = 30 deg
  float horizontal_screen_size;
  float vertical_screen_size;
  if (bmax[0] - bmin[0] > bmax[1] - bmin[1]) {
    horizontal_screen_size = bmax[0] - bmin[0];
    vertical_screen_size =
        horizontal_screen_size * float(layer->height) / float(layer->width);
  } else {
    vertical_screen_size = bmax[1] - bmin[1];
    horizontal_screen_size =
        vertical_screen_size * float(layer->width) / float(layer->height);
  }

  const float x_center   = (bmax[0] + bmin[0]) * 0.5f;
  const float y_center   = (bmax[1] + bmin[1]) * 0.5f;
  const float z_center   = bmax[2] + horizontal_screen_size * 0.5f * sqrtf(3.f);
  const float ray_org[3] = {x_center, y_center, z_center};
  const float x_corner =
      (bmax[0] + bmin[0]) * 0.5f - horizontal_screen_size * 0.5f;
  const float y_corner =
      (bmax[1] + bmin[1]) * 0.5f + vertical_screen_size * 0.5f;
  const float z_corner = bmax[2];
  const float dx       = horizontal_screen_size / float(layer->width);
  const float dy       = vertical_screen_size / float(layer->height);

  for (uint32_t sample = 0; sample < num_sample; ++sample) {
    for (uint32_t y = render_tile.sy; y < render_tile.ty; y++) {
      for (uint32_t x = render_tile.sx; x < render_tile.tx; x++) {
        float target[3]  = {x_corner + dx * (x + rng.Draw()),
                           y_corner - dy * (y + rng.Draw()), z_corner};
        float ray_dir[3] = {target[0] - ray_org[0], target[1] - ray_org[1],
                            target[2] - ray_org[2]};
        Normalize(ray_dir);

        Ray ray;
        ray.ray_dir = float3(ray_dir);
        ray.ray_org = float3(ray_org);

        const float3 radiance = GetRadiance(ray, scene, rng);

        {
          std::lock_guard<std::mutex> lock_tile(render_tile.mtx);
          layer->rgba[(y * layer->width + x) * 4 + 0] += radiance[0];
          layer->rgba[(y * layer->width + x) * 4 + 1] += radiance[1];
          layer->rgba[(y * layer->width + x) * 4 + 2] += radiance[2];
          layer->rgba[(y * layer->width + x) * 4 + 3] += 1.0f;

          layer->count[y * layer->width + x]++;
        }
      }
    }
  }

  (*finished_jobs_counter)++;
  return finished_jobs_counter->load();
}

bool Render(const Scene& scene, const uint32_t width, const uint32_t height,
            const uint32_t num_sample,
            const std::atomic_bool& cancel_render_flag, RenderLayer* layer,
            std::atomic_size_t* finish_pass) {
  std::vector<std::unique_ptr<RenderTile>> render_tiles;
  std::vector<std::unique_ptr<std::atomic<uint32_t>>> finished_jobs_counters;
  PrepareRendering(scene, width, height, num_sample, layer, &render_tiles,
                   &finished_jobs_counters);

  const size_t num_tiles = render_tiles.size();

  const uint32_t num_threads =
      std::max(1U, std::thread::hardware_concurrency());
  std::vector<std::thread> workers;
  std::atomic<size_t> next_job_id(0);

  std::mutex mtx;
  *finish_pass = 0;

  const size_t num_jobs = num_tiles * num_sample;

  for (uint32_t thread_id = 0; thread_id < num_threads; ++thread_id) {
    workers.emplace_back([&, thread_id]() {
      RNG rng(thread_id, 1234567890);
      size_t job_id = 0;
      while ((job_id = next_job_id++) < num_jobs && !cancel_render_flag) {
        const uint32_t tile_id = uint32_t(job_id % num_tiles);
        const uint32_t sample  = uint32_t(job_id / num_tiles);
        const uint32_t counter = RenderingTile(
            scene, /*num_sample TODO*/ 1, *(render_tiles[tile_id]), rng, layer,
            finished_jobs_counters[sample].get());

        if (counter == num_tiles) {
          std::lock_guard<std::mutex> lock(mtx);
          if (sample >= *finish_pass) {
            // TODO more accurate way
            *finish_pass = sample + 1;
            printf("finish pass %lu\n", finish_pass->load());
          }
        }
      }
    });
  }

  for (auto& worker : workers) {
    worker.join();
  }

  return true;
}

void Normalize(float* v) {
  const float inv_norm =
      1.0f / std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
  v[0] *= inv_norm;
  v[1] *= inv_norm;
  v[2] *= inv_norm;
}

}  // namespace pbrlab
