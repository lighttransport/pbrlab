#include "render-tile.h"

#include <assert.h>

#include <algorithm>

namespace pbrlab {

RenderTile::RenderTile(const uint32_t width_, const uint32_t height_,
                       const uint32_t sx_, const uint32_t tx_,
                       const uint32_t sy_, const uint32_t ty_)
    : width(width_),
      height(height_),
      sx(std::min(sx_, tx_)),
      tx(std::max(sx_, tx_)),
      sy(std::min(sy_, ty_)),
      ty(std::max(sy_, ty_)) {
  if (width == 0 || height == 0) {
    assert(false);
  }
  if (tx - sx == 0 || ty - sy == 0) {
    assert(false);
  }
  if (tx > width || ty > height) {
    assert(false);
  }
}

bool CreateTiles(const uint32_t width, const uint32_t height,
                 const uint32_t tile_width, const uint32_t tile_height,
                 std::vector<std::unique_ptr<RenderTile>>* render_tiles) {
  render_tiles->clear();
  for (uint32_t i = 0; i < height; i += tile_height) {
    for (uint32_t j = 0; j < width; j += tile_width) {
      render_tiles->emplace_back(
          new RenderTile(width, height, j, std::min(j + tile_width, width), i,
                         std::min(i + tile_height, height)));
    }
  }
  return true;
}

}  // namespace pbrlab
