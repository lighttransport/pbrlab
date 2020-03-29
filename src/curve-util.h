#ifndef PBRLAB_CURVE_UTIL_H_
#define PBRLAB_CURVE_UTIL_H_

#include <stdint.h>

#include <vector>

namespace pbrlab {

///
/// Convert catmull-rom spline curve to cubic Bezier curve.
///
bool ToCubicBezierCurve(const std::vector<float> &cvs,         // xyz * n
                        const std::vector<float> &cv_radiuss,  // n
                        std::vector<float> *bezier_vertices,   // xyz * m
                        std::vector<float> *bezier_radiuss);   // m

}  // namespace pbrlab

#endif  // NANORT_EXAMPLE_CURVE_UTIL_H_
