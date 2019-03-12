#include "rpi.h"

// See https://graphics.stanford.edu/~seander/bithacks.html#IntegerLog
unsigned int_log2(unsigned v) {
  static const unsigned b[] = {0x2, 0xC, 0xF0, 0xFF00, 0xFFFF0000};
  static const unsigned S[] = {1, 2, 4, 8, 16};

  unsigned r = 0;
  for (int i = 4; i >= 0; i--) {
    if (v & b[i]) {
      v >>= S[i];
      r |= S[i];
    }
  }
  return r;
}
