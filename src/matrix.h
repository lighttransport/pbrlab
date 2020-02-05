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
  static void LookAt(float m[4][4], float eye[3], float lookat[3], float up[3]);
  static void Inverse(float m[4][4]);
  static void Mult(float dst[4][4], float m0[4][4], float m1[4][4]);
  static void MultV(float dst[3], float m[4][4], float v[3]);
};

}  // namespace pbrlab

#endif  // PBRLAB_MATRIX_H_
