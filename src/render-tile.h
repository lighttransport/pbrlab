#ifndef PBRLAB_RENDER_TILE_H_
#define PBRLAB_RENDER_TILE_H_
#include <stdint.h>

#include <memory>
#include <mutex>
#include <vector>

#include "random/rng.h"

namespace pbrlab {
struct RenderTile {
  RenderTile() = delete;
  explicit RenderTile(const uint32_t width_, const uint32_t height_,
                      const uint32_t sx_, const uint32_t tx_,
                      const uint32_t sy_, const uint32_t ty_);

  uint32_t width, height;
  uint32_t sx, tx, sy, ty;

  mutable std::mutex mtx;
};

bool CreateTiles(const uint32_t width, const uint32_t height,
                 const uint32_t tile_width, const uint32_t tile_height,
                 std::vector<std::unique_ptr<RenderTile>>* render_tiles);

}  // namespace pbrlab

#endif  // PBRLAB_RENDER_TILE_H_
