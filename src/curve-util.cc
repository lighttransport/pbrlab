#include "curve-util.h"

#include "type.h"

namespace pbrlab {

static void CatmullRomToCubicBezierEnd(
    float3 Q[4],            // control points of cubic bezier
    const float3 P[3],      // control points of catmull-rom
    const float tau = 0.5f  // catmull-rom tightness
) {
  const float tau3 = tau / 3.0f;
  Q[0]             = P[1];
  Q[1]             = tau3 * (P[2] - P[0]) + P[1];
  Q[2] = (-tau3) * P[0] + (2.0f / 3.0f) * P[1] + ((tau + 1.0f) / 3.0f) * P[2];
  Q[3] = P[2];
}

static void GetSegmentThicknessEnd(
    float Q[4],             // control points of cubic bezier
    const float P[3],       // control points of catmull-rom
    const float tau = 0.5f  // catmull-rom tightness
) {
  const float tau3 = tau / 3.0f;
  Q[0]             = P[1];
  Q[1]             = tau3 * (P[2] - P[0]) + P[1];
  Q[2] = (-tau3) * P[0] + (2.0f / 3.0f) * P[1] + ((tau + 1.0f) / 3.0f) * P[2];
  Q[3] = P[2];
}

static void CatmullRomToCubicBezierStart(
    float3 Q[4],            // control points of cubic bezier
    const float3 P[3],      // control points of catmull-rom
    const float tau = 0.5f  // catmull-rom tightness
) {
  const float tau3 = tau / 3.0f;
  Q[0]             = P[0];
  Q[1] = ((tau + 1.0f) / 3.0f) * P[0] + (2.0f / 3.0f) * P[1] - tau3 * P[2];
  Q[2] = tau3 * (P[0] - P[2]) + P[1];
  Q[3] = P[1];
}

static void GetSegmentThicknessStart(
    float Q[4],             // control points of cubic bezier
    const float P[3],       // control points of catmull-rom
    const float tau = 0.5f  // catmull-rom tightness
) {
  const float tau3 = tau / 3.0f;
  Q[0]             = P[0];
  Q[1] = ((tau + 1.0f) / 3.0f) * P[0] + (2.0f / 3.0f) * P[1] - tau3 * P[2];
  Q[2] = tau3 * (P[0] - P[2]) + P[1];
  Q[3] = P[1];
}

// for in-between points
static void CatmullRomToCubicBezier(
    float3 Q[4],            // control points of cubic bezier
    const float3 P[4],      // control points of catmull-rom Pi-1,Pi,Pi+1,Pi+2
    const float tau = 0.5f  // catmull-rom tightness
) {
  const float tau3 = tau / 3.0f;
  Q[0]             = P[1];
  Q[1]             = tau3 * (P[2] - P[0]) + P[1];
  Q[2]             = tau3 * (P[1] - P[3]) + P[2];
  Q[3]             = P[2];
}
static void GetSegmentThickness(
    float Q[4],             // control points of cubic bezier
    const float P[4],       // radius of catmull-rom Pi-1,Pi,Pi+1,Pi+2
    const float tau = 0.5f  // catmull-rom tightness
) {
  const float tau3 = tau / 3.0f;
  Q[0]             = P[1];
  Q[1]             = tau3 * (P[2] - P[0]) + P[1];
  Q[2]             = tau3 * (P[1] - P[3]) + P[2];
  Q[3]             = P[2];
}

bool ToCubicBezierCurve(const std::vector<float> &cvs,
                        const std::vector<float> &cv_radiuss,
                        std::vector<float> *bezier_vertices,
                        std::vector<float> *bezier_radiuss) {
  if (cvs.empty() || cv_radiuss.empty()) {
    return false;
  }

  if ((cvs.size() % 3) != 0) {
    return false;
  }

  if (bezier_vertices->size() % 12 != 0) {
    return false;
  }
  if (bezier_radiuss->size() % 4 != 0) {
    return false;
  }
  if (bezier_vertices->size() != bezier_radiuss->size() * 3) {
    return false;
  }

  const size_t num_cvs      = cvs.size() / 3;
  const size_t num_segments = num_cvs - 1;

  if (num_cvs < 3) {
    return false;
  }

  if (num_cvs != cv_radiuss.size()) {
    return false;
  }

  // root
  {
    float3 q[4];
    float3 P[3];
    P[0][0] = cvs[0];
    P[0][1] = cvs[1];
    P[0][2] = cvs[2];
    P[1][0] = cvs[3];
    P[1][1] = cvs[4];
    P[1][2] = cvs[5];
    P[2][0] = cvs[6];
    P[2][1] = cvs[7];
    P[2][2] = cvs[8];
    CatmullRomToCubicBezierStart(q, P);

    for (int i = 0; i < 4; i++) {
      for (int j = 0; j < 3; j++) {
        bezier_vertices->emplace_back(q[i][j]);
      }
    }

    float thickness[4];
    GetSegmentThicknessStart(thickness, cv_radiuss.data());

    for (int i = 0; i < 4; i++) bezier_radiuss->emplace_back(thickness[i]);
  }

  // In-between points
  for (size_t s = 1; s < num_segments - 1; s++) {
    size_t seg_idx = s - 1;
    float3 q[4];
    float3 P[4];
    P[0][0] = cvs[3 * seg_idx + 0];
    P[0][1] = cvs[3 * seg_idx + 1];
    P[0][2] = cvs[3 * seg_idx + 2];
    P[1][0] = cvs[3 * seg_idx + 3];
    P[1][1] = cvs[3 * seg_idx + 4];
    P[1][2] = cvs[3 * seg_idx + 5];
    P[2][0] = cvs[3 * seg_idx + 6];
    P[2][1] = cvs[3 * seg_idx + 7];
    P[2][2] = cvs[3 * seg_idx + 8];
    P[3][0] = cvs[3 * seg_idx + 9];
    P[3][1] = cvs[3 * seg_idx + 10];
    P[3][2] = cvs[3 * seg_idx + 11];
    CatmullRomToCubicBezier(q, P);

    for (int i = 0; i < 4; i++) {
      for (int j = 0; j < 3; j++) {
        bezier_vertices->emplace_back(q[i][j]);
      }
    }

    float thickness[4];
    GetSegmentThickness(thickness, cv_radiuss.data() + seg_idx);

    for (int i = 0; i < 4; i++) bezier_radiuss->emplace_back(thickness[i]);
  }

  // End point
  if (num_segments > 1) {
    float3 q[4];
    float3 P[3];
    P[0][0] = cvs[3 * (num_segments - 2) + 0];
    P[0][1] = cvs[3 * (num_segments - 2) + 1];
    P[0][2] = cvs[3 * (num_segments - 2) + 2];
    P[1][0] = cvs[3 * (num_segments - 2) + 3];
    P[1][1] = cvs[3 * (num_segments - 2) + 4];
    P[1][2] = cvs[3 * (num_segments - 2) + 5];
    P[2][0] = cvs[3 * (num_segments - 2) + 6];
    P[2][1] = cvs[3 * (num_segments - 2) + 7];
    P[2][2] = cvs[3 * (num_segments - 2) + 8];

    CatmullRomToCubicBezierEnd(q, P);

    for (int i = 0; i < 4; i++) {
      for (int j = 0; j < 3; j++) {
        bezier_vertices->emplace_back(q[i][j]);
      }
    }

    float thickness[4];
    GetSegmentThicknessEnd(thickness, cv_radiuss.data() + num_segments - 2);

    for (int i = 0; i < 4; i++) bezier_radiuss->emplace_back(thickness[i]);
  }

  return true;
}

}  // namespace pbrlab
