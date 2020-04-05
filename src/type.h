#ifndef PBRLAB_TYPE_H_
#define PBRLAB_TYPE_H_

#define NANORT_USE_CPP11_FEATURE
#include "nanort.h"

namespace pbrlab {
using float3 = nanort::real3<float>;

template <typename T = float>
class real2 {
public:
  real2() {}
  real2(T x) {
    v[0] = x;
    v[1] = x;
  }
  real2(T xx, T yy) {
    v[0] = xx;
    v[1] = yy;
  }
  explicit real2(const T *p) {
    v[0] = p[0];
    v[1] = p[1];
  }

  inline T x() const { return v[0]; }
  inline T y() const { return v[1]; }

  real2 operator*(T f) const { return real2(x() * f, y() * f); }
  real2 operator-(const real2 &f2) const {
    return real2(x() - f2.x(), y() - f2.y());
  }
  real2 operator*(const real2 &f2) const {
    return real2(x() * f2.x(), y() * f2.y());
  }
  real2 operator+(const real2 &f2) const {
    return real2(x() + f2.x(), y() + f2.y());
  }
  real2 &operator+=(const real2 &f2) {
    v[0] += f2.x();
    v[1] += f2.y();
    return (*this);
  }
  real2 operator/(const real2 &f2) const {
    return real2(x() / f2.x(), y() / f2.y());
  }
  real2 operator-() const { return real2(-x(), -y()); }
  T operator[](int i) const { return v[i]; }
  T &operator[](int i) { return v[i]; }

  T v[2];
  // T pad;  // for alignment (when T = float)
};

template <typename T>
inline real2<T> operator*(T f, const real2<T> &v) {
  return real2<T>(v.x() * f, v.y() * f);
}

template <typename T>
inline real2<T> vneg(const real2<T> &rhs) {
  return real2<T>(-rhs.x(), -rhs.y());
}

template <typename T>
inline T vlength(const real2<T> &rhs) {
  return std::sqrt(rhs.x() * rhs.x() + rhs.y() * rhs.y());
}

template <typename T>
inline real2<T> vnormalize(const real2<T> &rhs) {
  real2<T> v = rhs;
  T len      = vlength(rhs);
  if (std::fabs(len) > std::numeric_limits<T>::epsilon()) {
    T inv_len = static_cast<T>(1.0) / len;
    v.v[0] *= inv_len;
    v.v[1] *= inv_len;
  }
  return v;
}

using float2 = real2<float>;

}  // namespace pbrlab
#endif  // PBRLAB_TYPE_H_
