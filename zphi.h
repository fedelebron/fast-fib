#include <cassert>
#include <cstdint>

// Z[x]/<x^2-x-1>, where Z is represented by the type T.
template<typename T>
struct zphi {
  T a = 1;
  T b = 0;
  zphi pow(uint64_t n) {
    zphi base = *this, res;
    while (n) {
      if (n & 1) {
        res = res * base;
        --n;
      }
      base = base * base;
      n >>= 1;
    }
    return res;
  }
};

template<typename T>
zphi<T> operator*(const zphi<T>& x, const zphi<T>& y) {
  if (&x == &y) {
    T a2 = x.a * x.a;
    T b2 = x.b * x.b;
    T ab = x.a * x.b;
    ab <<= (unsigned long)1;
    return {.a = a2 + ab, .b = a2 + b2};
  }
  T ag = x.a * y.a;
  T ad = x.a * y.b;
  T bg = x.b * y.a;
  T bd = x.b * y.b;
  bd += ag;
  ag += ad;
  ag += bg;
  return zphi{.a = std::move(ag), .b = std::move(bd)};
}
