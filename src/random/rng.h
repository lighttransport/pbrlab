#ifndef PBRLAB_RNG_H_
#define PBRLAB_RNG_H_
#include <assert.h>
#include <math.h>
#include <stdint.h>

namespace pbrlab {

// PCG32 code / (c) 2014 M.E. O'Neill / pcg-random.org
// Licensed under Apache License 2.0 (NO WARRANTY, etc. see website)
// http://www.pcg-random.org/
typedef struct {
  uint64_t state;
  uint64_t inc;
} pcg32_state_t;

inline uint32_t pcg32_random(pcg32_state_t* rng) {
  uint64_t oldstate   = rng->state;
  rng->state          = oldstate * uint64_t(6364136223846793005) + rng->inc;
  uint32_t xorshifted = uint32_t(((oldstate >> 18u) ^ oldstate) >> 27u);
  uint32_t rot        = oldstate >> 59u;
  uint32_t ret =
      (xorshifted >> rot) | (xorshifted << ((-static_cast<int>(rot)) & 31));

  return ret;
  // return float(double(ret) / double(4294967296.0));
}

inline void pcg32_srandom(pcg32_state_t* rng, uint64_t initstate,
                          uint64_t initseq) {
  rng->state = 0U;
  rng->inc   = (initseq << 1U) | 1U;
  pcg32_random(rng);
  rng->state += initstate;
  pcg32_random(rng);
}

//
// Simple random number generator class
//
class RNG {
public:
  explicit RNG(uint64_t initstate, uint64_t initseq) {
    pcg32_srandom(&state_, initstate, initseq);
  }

  ~RNG() = default;

  ///
  /// Draw random number[0.0, 1.0)
  ///
  inline float Draw() {
    // https://github.com/wjakob/pcg32/blob/master/pcg32.h

    union {
      uint32_t u;
      float f;
    } x;
    x.u = (pcg32_random(&state_) >> 9) | 0x3f800000u;
    if (((x.f - 1.0f) < 0.0f) || ((x.f - 1.0f) > 1.0f)) {
      assert(false);
      return 0.0f;
    }
    return x.f - 1.0f;
  }

private:
  pcg32_state_t state_;
};

}  // namespace pbrlab

#endif  // PBRLAB_RNG_H_
