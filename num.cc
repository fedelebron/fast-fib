#include "num.h"

#include <array>
#include <bitset>
#include <cassert>
#include <compare>
#include <format>
#include <iostream>
#include <limits>
#include <ostream>
#include <span>
#include <sstream>
#include <string>

#define KARATSUBA_THRESHOLD 100

std::ostream& operator<<(std::ostream& dest, __uint128_t value) {
  std::ostream::sentry s(dest);
  if (s) {
    __uint128_t tmp = value;
    char buffer[128];
    char* d = std::end(buffer);
    do {
      --d;
      *d = "0123456789"[tmp % 10];
      tmp /= 10;
    } while (tmp != 0);
    int len = std::end(buffer) - d;
    if (dest.rdbuf()->sputn(d, len) != len) {
      dest.setstate(std::ios_base::badbit);
    }
  }
  return dest;
}
uint64_t max_pow_10_in_bytes[] = {0ull,
                                  100ull,
                                  10000ull,
                                  10000000ull,
                                  1000000000ull,
                                  1000000000000ull,
                                  100000000000000ull,
                                  10000000000000000ull,
                                  10000000000000000000ull};

const char* fmt_str[] = {"",       "{:02}",  "{:04}",  "{:07}", "{:08}",
                         "{:011}", "{:014}", "{:016}", "{:019}"};

template <typename T>
using Limbs = std::vector<T>;

template <typename T>
using LimbSpan = std::span<T>;

template <typename T>
using DoubleLimb =
    std::conditional_t<2 * sizeof(T) <= sizeof(unsigned), unsigned,
                       std::conditional_t<2 * sizeof(T) <= sizeof(uint64_t),
                                          uint64_t, __uint128_t>>;

template <typename T>
std::string show_limbs(LimbSpan<T> s) {
  std::ostringstream ss;
  num<std::remove_const_t<T>> x;
  x.chunks.assign(s.begin(), s.end());
  ss << x;
  return ss.str();
}

template <typename T>
std::string dump_limbs(LimbSpan<T> s) {
  std::ostringstream o;
  o << '{';
  for (unsigned int i = 0; i < s.size(); ++i) {
    if (i > 0) o << ", ";
    o << +s[i];
  }
  o << '}';
  return o.str();
}

template <typename T>
void shrink_limbs(std::vector<T>& v) {
  int i = v.size() - 1;
  while (i >= 0 && v[i] == 0) --i;
  v.erase(v.begin() + i + 1, v.end());
}


template <typename T>
bool add_limbs(LimbSpan<T> x, LimbSpan<const T> y, unsigned int shift = 0) {
  int m = y.size();
  bool carry = false;
  unsigned int j = shift;
  for (int i = 0; i < m; ++i, ++j) {
    T b = y[i];
    T a = x[j];
    a += b;
    bool new_carry = a < b;
    a += carry;
    new_carry |= (a < carry);
    x[j] = a;
    carry = new_carry;
  }
  while (carry && j < x.size()) {
    carry = ++x[j++] == 0;
  }
  return carry;
}

template <typename T>
std::vector<T> add(LimbSpan<const T> x, LimbSpan<const T> y) {
  std::vector<T> ret(x.begin(), x.end());
  ret.resize(std::max(x.size(), y.size()) + 1);
  assert (!add_limbs<T>(ret, y));
  shrink_limbs(ret);
  return ret;
}

template <typename T>
void sub_limbs(LimbSpan<T> x, LimbSpan<const T> y) {
  // assumes self >= other
  bool borrow = false;
  unsigned int i;
  for (i = 0; i < y.size(); ++i) {
    auto yy = y[i];
    auto xx = x[i];
    T z;
    bool new_borrow;
    if (xx == 0 && borrow) {
      z = std::numeric_limits<T>::max() - yy;
      new_borrow = true;
    } else if (xx - borrow >= yy) {
      z = xx - T{borrow} - yy;
      new_borrow = false;
    } else {
      // Note y cannot be zero, since otherwise
      // we fall into the previous condition.
      z = (std::numeric_limits<T>::max() - yy) + T{1} - T{borrow} + xx;
      new_borrow = true;
    }
    x[i] = z;
    borrow = new_borrow;
  }

  if (borrow) {
    bool found = false;
    for (; i < x.size(); ++i) {
      found = x[i] != 0;
      --x[i];
      if (found) {
        break;
      }
    }
    assert (found);
  }
}

template <typename T>
std::ostream& operator<<(std::ostream& o, const num<T>& w) {
  if (w.chunks.empty()) {
    o << '0';
    return o;
  }
  std::string buf;
  num<T> d = {{T(max_pow_10_in_bytes[sizeof(T)])}};
  auto x = w;
  while (x >= d) {
    auto [q, r] = x.quot_rem(d);

    T val = r.size() ? r.chunks[0] : 0;
    buf = std::vformat(fmt_str[sizeof(T)], std::make_format_args(val)) + buf;
    x = q;
  }
  if (x.size()) buf = std::to_string(x.chunks[0]) + buf;
  o << buf;
  return o;
}

template <typename T>
int num<T>::size() const {
  return chunks.size();
}

template <typename T>
std::strong_ordering cmp_limbs(std::span<const T> a, std::span<const T> b) {
  int n = a.size(), m = b.size();
  if (n > m) {
    return std::strong_ordering::greater;
  } else if (n < m) {
    return std::strong_ordering::less;
  }
  for (int i = n - 1; i >= 0; --i) {
    if (a[i] > b[i]) {
      return std::strong_ordering::greater;
    } else if (a[i] < b[i]) {
      return std::strong_ordering::less;
    }
  }
  return std::strong_ordering::equivalent;
}

template <typename T>
int num<T>::chunk_bits() {
  return sizeof(T) << 3;
}

template <typename T>
num<T>& num<T>::operator+=(const num<T>& other) {
  chunks.resize(std::max(size(), other.size()));
  if (add_limbs<T>(chunks, other.chunks)) {
    chunks.push_back(1);
  }
  return *this;
}

template <typename T>
num<T>& num<T>::operator-=(const num<T>& other) {
  return sub(other);
}

template <typename T>
void num<T>::shrink() {
  shrink_limbs(chunks);
}

template <typename T>
num<T>& num<T>::sub(const num<T>& other) {
  // assumes self >= other
  sub_limbs<T>(chunks, other.chunks);
  shrink();
  return *this;
}

template <typename T>
std::vector<T> mul(std::span<const T> a, std::span<const T> b);

template <typename T>
T add(T a, T b, T& carry) {
  DoubleLimb<T> c = DoubleLimb<T>{a} + b;
  carry = c >> (sizeof(T) * 8);
  return static_cast<T>(c);
}

template <typename T>
T mul(T a, T b, T&out) {
  DoubleLimb<T> res = static_cast<DoubleLimb<T>>(a) * static_cast<DoubleLimb<T>>(b);
  out = res >> (sizeof(T) * 8);
  return static_cast<T>(res);
}

template <typename T>
std::vector<T> conv_multiplication(std::span<const T> a, std::span<const T> b) {
  // Slightly better grade school multiplication algorithm.
  // In c = a * b, we write to c[i] once, looping over a[j] * b[i - j] forall j.
  // We keep track of c[i + 1]'s incoming sum (`next`) via carries out of c[i],
  // as well as the carry out of c[i + 1] (`next_carry`).
  // The idea is from V8's bigint implementation.
  std::vector<T> c;
  c.resize(a.size() + b.size());
  T next, x, next_carry = 0, carry = 0, high;
  if (a.size() == 0 || b.size() == 0) return c; 

  c[0] = mul(a[0], b[0], next);
  size_t i = 1;
  auto body = [&](int min, int max) {
    for (int j = min; j <= max; ++j) {
      T low = mul(a[j], b[i - j], high);
      T carrybit;
      x = add(x, low, carrybit);
      carry += carrybit;
      next = add(next, high, carrybit);
      next_carry += carrybit;
    }
    c[i] = x;
  };
  if (i < b.size()) {
    x = next;
    next = 0;
    body(0, 1);
    ++i;
  }

  for (; i < b.size(); i++) {
    x = add(next, carry, carry);
    next = next_carry + carry;
    carry = 0;
    next_carry = 0;
    body(0, i);
  }


  size_t loop_end = a.size() + b.size() - 2;
  for (; i <= loop_end; i++) {
    int max_x_index = std::min(i, a.size() - 1);
    int max_y_index = b.size() - 1;
    int min_x_index = i - max_y_index;
    x = add(next, carry, carry);
    next = next_carry + carry;
    carry = 0;
    next_carry = 0;
    body(min_x_index, max_x_index);
  }

  c[i] = add(next, carry, carry);
  assert (carry == 0);
  while (i >= 0 && c[i] == 0) --i;
  c.erase(c.begin() + i + 1, c.end());
  return c;
}

template <typename T>
std::vector<T> karatsuba(std::span<const T> a, std::span<const T> b) {
  // b is shorter or equal in length to a.
  int llen = b.size() / 2;
  auto a0 = a.subspan(0, llen);
  auto a1 = a.subspan(llen);
  auto b0 = b.subspan(0, llen);
  auto b1 = b.subspan(llen);

  auto z0 = mul(a0, b0);
  auto z2 = mul(a1, b1);

  auto tmp = add(a0, a1);
  auto z3 = add(b0, b1);
  z3 = mul<T>(z3, tmp);  // z3 = (a0 + a1) * (b0 + b1)
  sub_limbs<T>(z3, z0);
  sub_limbs<T>(z3, z2);

  int rsize =
      2 + std::max(z0.size(),
            std::max(z3.size() + llen,
                     z2.size() + 2 * llen));
  std::vector<T> result(z0);
  result.resize(rsize);
  add_limbs<T>(result, z3, llen);
  add_limbs<T>(result, z2, 2 * llen);

  shrink_limbs(result);

  return result;
}

template <typename T>
std::vector<T> mul(std::span<const T> a, std::span<const T> b) {
  if (a.size() < b.size()) {
    std::swap(a, b);
  }
  int minlen = b.size();
  if (minlen < KARATSUBA_THRESHOLD) {
    //return long_multiplication(a, b);
    return conv_multiplication(a, b);
  } else {
    return karatsuba(a, b);
  }
}

template <typename T>
num<T>& num<T>::operator*=(const num<T>& other) {
  chunks = mul<T>(chunks, other.chunks);
  return *this;
}

template <typename T>
std::strong_ordering operator<=>(const num<T>& a, const num<T>& b) {
  return cmp_limbs<T>(a.chunks, b.chunks);
}

template <typename T>
bool num<T>::get_bit(int i) const {
  int j = i % (sizeof(T) << 3);
  int k = i / (sizeof(T) << 3);
  return chunks[k] & (T{1} << j);
}

template <typename T>
void num<T>::set_bit(int i, bool b) {
  int j = i % (sizeof(T) << 3);
  int k = i / (sizeof(T) << 3);
  while (k >= size()) chunks.push_back(0);
  if (b)
    chunks[k] |= (T{1} << j);
  else
    chunks[k] &= ~(T{1} << j);
}

template <typename T>
num<T>& num<T>::operator<<=(int k) {
  int n = size() - 1;
  int N = chunk_bits();
  int a = (N - 1 + k) % N + 1;
  int b = N - a;
  int d = (k / N) + 1;
  if (k % N == 0) --d;
  for (int i = 0; i < d; ++i) chunks.emplace_back(0);
  for (int i = n; i + d - 1 >= 0; --i) {
    auto x = i >= 0 ? chunks[i] : 0;
    chunks[i + d] = ((chunks[i + d] >> a) << a) | (x >> b);
    chunks[i + d - 1] = (x & ((T{1} << b) - 1)) << a;
  }
  shrink();
  return *this;
}

template <typename T>
std::pair<num<T>, num<T>> num<T>::quot_rem(const num<T>& d) const {
  assert(d.size() != 0);
  num<T> q, r;
  int N = chunk_bits() * size();
  for (int i = N - 1; i >= 0; --i) {
    r <<= 1;
    bool b = get_bit(i);
    r.set_bit(0, b);
    if (r >= d) {
      r -= d;
      q.set_bit(i, true);
    }
  }
  return {q, r};
}

template <typename T>
num<T>::num(T limb) {
  if (limb) chunks = {limb};
}

template <typename T>
num<T>::num(std::string_view v) {
  // super slow, don't care
  num<T> b = 10;
  for (unsigned int i = 0; i < v.size(); ++i) {
    char c = v[i];
    if (c > '9' || c < '0') continue;
    *this *= b;
    *this += num(c - '0');
  }
}

template <typename T>
std::string num<T>::dump() const {
  return dump_limbs<const T>(chunks);
}

template <typename T>
T num<T>::get_limb() const {
  assert(chunks.size() <= 1);
  if (chunks.empty()) return 0;
  return chunks[0];
}

template <typename T>
num<T> operator+(const num<T>& x, const num<T>& y) {
  num<T> r = x;
  r += y;
  return r;
}
template <typename T>
num<T> operator-(const num<T>& x, const num<T>& y) {
  num<T> r = x;
  r -= y;
  return r;
}
template <typename T>
num<T> operator*(const num<T>& x, const num<T>& y) {
  num<T> r = x;
  r *= y;
  return r;
}

#define INSTANTIATE(T)                                                      \
  template bool add_limbs(LimbSpan<T>, LimbSpan<const T>, unsigned int0);   \
  template std::ostream& operator<<(std::ostream&, const num<T>&);          \
  template std::strong_ordering operator<=>(const num<T>&, const num<T>&);  \
  template num<T>& num<T>::operator+=(const num<T>&);                       \
  template num<T>& num<T>::operator-=(const num<T>&);                       \
  template num<T>& num<T>::operator<<=(int);                                \
  template num<T>& num<T>::operator*=(const num<T>&);                       \
  template num<T> operator+(const num<T>&, const num<T>&);                  \
  template num<T> operator-(const num<T>&, const num<T>&);                  \
  template num<T> operator*(const num<T>&, const num<T>&);                  \
  template num<T>& num<T>::sub(const num<T>&);                              \
  template std::pair<num<T>, num<T>> num<T>::quot_rem(const num<T>&) const; \
  template num<T>::num(T);                                                  \
  template num<T>::num(std::string_view);                                   \
  template std::string num<T>::dump() const;                                \
  template bool num<T>::get_bit(int) const;                                 \
  template void num<T>::set_bit(int, bool)

INSTANTIATE(uint8_t);
INSTANTIATE(uint16_t);
INSTANTIATE(uint32_t);
INSTANTIATE(uint64_t);
