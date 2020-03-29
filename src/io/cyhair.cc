#include "io/cyhair.h"

// TODO logger
namespace pbrlab {

CyHair::CyHair(void)
    : flags_(0),
      num_strands_(0),
      total_points_(0),
      default_segments_(-1),
      default_thickness_(0.01f),
      default_transparency_(1.0f) {
  default_color_[0] = 0.5f;
  default_color_[1] = 0.5f;
  default_color_[2] = 0.5f;
}

CyHair::~CyHair(void) {}

bool CyHair::Load(const std::string &filepath) {
  FILE *fp = fopen(filepath.c_str(), "rb");
  if (!fp) {
    return false;
  }

  assert(sizeof(CyHairHeader) == 128);
  CyHairHeader header;

  if (1 != fread(&header, 128, 1, fp)) {
    fclose(fp);
    return false;
  }
  if (memcmp(header.magic, "HAIR", 4) != 0) {
    fclose(fp);
    return false;
  }

  flags_                = header.flags;
  default_thickness_    = header.default_thickness;
  default_transparency_ = header.default_transparency;
  default_segments_     = static_cast<int>(header.default_segments);
  default_color_[0]     = header.default_color[0];
  default_color_[1]     = header.default_color[1];
  default_color_[2]     = header.default_color[2];

  const bool has_segments     = flags_ & 0x1;
  const bool has_points       = flags_ & 0x2;
  const bool has_thickness    = flags_ & 0x4;
  const bool has_transparency = flags_ & 0x8;
  const bool has_color        = flags_ & 0x10;

  num_strands_  = header.num_strands;
  total_points_ = header.total_points;

  if (!has_points) {
    // LOG_ERROR("No point data in CyHair.");
    return false;
  }

  if ((default_segments_ < 1) && (!has_segments)) {
    // LOG_ERROR("No valid segment information in CyHair.");
    return false;
  }

  // First read all strand data from a file.
  if (has_segments) {
    segments_.resize(num_strands_);
    if (1 !=
        fread(&segments_[0], sizeof(unsigned short) * num_strands_, 1, fp)) {
      // LOG_ERROR("Failed to read CyHair segments data.");
      fclose(fp);
      return false;
    }
  }

  if (has_points) {
    // LOG_INFO("[CyHair] Has points.");
    points_.resize(3 * total_points_);
    size_t n = fread(&points_[0], total_points_ * sizeof(float) * 3, 1, fp);
    if (1 != n) {
      // LOG_ERROR("Failed to read CyHair points data.");
      fclose(fp);
      return false;
    }
  }
  if (has_thickness) {
    // LOG_INFO("[CyHair] Has thickness.");
    thicknesses_.resize(total_points_);
    if (1 != fread(&thicknesses_[0], total_points_ * sizeof(float), 1, fp)) {
      //// LOG_ERROR("Failed to read CyHair thickness data.");
      fclose(fp);
      return false;
    }

    // print thickness of the first strand.
    size_t n = segments_[1];
    for (size_t i = 0; i < n; i++) {
      // LOG_INFO("[CyHair] thickness[{}] = {}", i, thicknesses_[i]);
    }
  }

  if (has_transparency) {
    // LOG_INFO("[CyHair] Has transparency.");
    transparencies_.resize(total_points_);
    if (1 != fread(&transparencies_[0], total_points_ * sizeof(float), 1, fp)) {
      // LOG_ERROR("Failed to read CyHair transparencies data.");
      fclose(fp);
      return false;
    }
  }

  if (has_color) {
    // LOG_INFO("[CyHair] Has color.");
    colors_.resize(3 * total_points_);
    if (1 != fread(&colors_[0], total_points_ * sizeof(float) * 3, 1, fp)) {
      // LOG_ERROR("Failed to read CyHair colors data.");
      fclose(fp);
      return false;
    }
  }

  // Build strand offset table.
  strand_offsets_.resize(num_strands_);
  strand_offsets_[0] = 0;
  for (size_t i = 1; i < num_strands_; i++) {
    int num_segments = segments_.empty() ? default_segments_ : segments_[i - 1];
    strand_offsets_[i] =
        strand_offsets_[i - 1] + static_cast<unsigned int>(num_segments + 1);
  }

  fclose(fp);
  return true;
}

bool LoadCyHair(const std::string &filepath, const bool is_y_up,
                std::vector<std::vector<float>> *vertices,
                std::vector<std::vector<float>> *thicknesses) {
  CyHair cyhair;

  const bool ret = cyhair.Load(filepath);
  if (!ret) {
    // LOG_ERROR("[CyHair] Failed to load CyHair file [{}]", filepath);
    return false;
  }

  // LOG_INFO("[CyHair] num strand is {}", cyhair.num_strands_);
  size_t offset = 0;
  for (size_t strand_id = 0; strand_id < cyhair.num_strands_; ++strand_id) {
    const size_t num_segment = cyhair.segments_.empty()
                                   ? size_t(cyhair.default_segments_)
                                   : cyhair.segments_.at(strand_id);

    const size_t num_vertices = num_segment + 1;
    if (num_vertices < 2) {
      // LOG_WARN("strand is broken");
      continue;
    }

    vertices->emplace_back();
    std::vector<float> &_vertices = vertices->back();
    thicknesses->emplace_back();
    std::vector<float> &_thicknesses = thicknesses->back();

    for (size_t v_id = 0; v_id < num_vertices; ++v_id) {
      if (is_y_up) {
        _vertices.emplace_back(cyhair.points_.at((offset + v_id) * 3 + 0));
        _vertices.emplace_back(cyhair.points_.at((offset + v_id) * 3 + 1));
        _vertices.emplace_back(cyhair.points_.at((offset + v_id) * 3 + 2));
      } else {
        _vertices.emplace_back(cyhair.points_.at((offset + v_id) * 3 + 0));
        _vertices.emplace_back(cyhair.points_.at((offset + v_id) * 3 + 2));
        _vertices.emplace_back(cyhair.points_.at((offset + v_id) * 3 + 1));
      }

      _thicknesses.emplace_back(cyhair.thicknesses_.at(offset + v_id));
    }

    offset += num_vertices;
  }
  // LOG_INFO("[CyHair] finish loading cyhair file [{}]", filepath);
  return true;
}

}  // namespace pbrlab
