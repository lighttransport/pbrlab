#ifndef PBRLAB_MATRIX_H_
#define PBRLAB_MATRIX_H_

namespace pbrlab {

// TODO namespace
class Matrix {
public:
  Matrix();
  ~Matrix();

  static void Copy(const float src[4][4], float out[4][4]);
  static void Print(float m[4][4]);
  static void LookAt(const float eye[3], const float lookat[3],
                     const float up[3], float m[4][4]);
  static void Inverse(float m[4][4]);
  static void Mult(const float m0[4][4], const float m1[4][4], float dst[4][4]);
  static void MultV(const float v[3], const float m[4][4], float dst[3]);
};

}  // namespace pbrlab

#endif  // PBRLAB_MATRIX_H_
