#include "num.h"
#include <cassert>
#include <ostream>
#include <iostream>
#include <compare>
#include <limits>
#include <bitset>
#include <string>
#include <format>
#include <span>
#include <array>
#include <sstream>

std::ostream&
operator<<( std::ostream& dest, __uint128_t value )
{
    std::ostream::sentry s( dest );
    if ( s ) {
        __uint128_t tmp = value;
        char buffer[ 128 ];
        char* d = std::end( buffer );
        do
        {
            -- d;
            *d = "0123456789"[ tmp % 10 ];
            tmp /= 10;
        } while ( tmp != 0 );
        int len = std::end( buffer ) - d;
        if ( dest.rdbuf()->sputn( d, len ) != len ) {
            dest.setstate( std::ios_base::badbit );
        }
    }
    return dest;
}
uint64_t max_pow_10_in_bytes[] = {
  0ull,
  100ull,
  10000ull,
  10000000ull,
  1000000000ull,
  1000000000000ull,
  100000000000000ull,
  10000000000000000ull,
  10000000000000000000ull
};

const char* fmt_str[] = {
 "",
 "{:02}",
 "{:04}",
 "{:07}",
 "{:08}",
 "{:011}",
 "{:014}",
 "{:016}",
 "{:019}"
};

template<typename T>
using Limbs = std::vector<T>;

template<typename T>
using LimbSpan = std::span<T>;

template<typename T>
using DoubleLimb = std::conditional_t<2 * sizeof(T) <= sizeof(unsigned), unsigned,
                       std::conditional_t<2 * sizeof(T) <= sizeof(uint64_t), uint64_t,
                       __uint128_t>>;

template<typename T>
bool add_limbs(LimbSpan<T> x, LimbSpan<const T> y, int shift = 0) {
  int m = y.size();
  bool carry = false; 
  int j = shift;
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

template<typename T>
void shrink_limbs(std::vector<T>& v) {
  int i = v.size();
  while (--i >= 0) {
    if (!v[i]) {
      v.pop_back();
      continue;
    }
    return;
  }
}


template<typename T>
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

template<typename T>
std::string show_limbs(LimbSpan<T> s) {
  std::ostringstream ss;
  num<std::remove_const_t<T>> x;
  x.chunks.assign(s.begin(), s.end());
  ss << x;
  return ss.str();
}

template<typename T>
std::string dump_limbs(LimbSpan<T> s) {
  std::ostringstream ss;
  num<std::remove_const_t<T>> x;
  x.chunks.assign(s.begin(), s.end());
  return x.dump();
}

template<typename T>
int num<T>::size() const {
  return chunks.size();
}

template<typename T>
int num<T>::chunk_bits() {
  return sizeof(T) << 3;
}

template<typename T>
num<T>& num<T>::operator+=(const num<T>& other) {
  chunks.resize(std::max(size(), other.size()));
  if (add_limbs(std::span{chunks}, std::span{other.chunks})) {
    chunks.push_back(1);
  }
  return *this;
}

template<typename T>
num<T>& num<T>::operator-=(const num<T>& other) {
  return sub(other);
}

template<typename T>
void num<T>::shrink() {
  shrink_limbs(chunks);
}

template<typename T>
num<T>& num<T>::sub(const num<T>& other) {
  // assumes self >= other
  bool borrow = false;
  int i;
  for (i = 0; i < other.size(); ++i) {
    auto y = other.chunks[i];
    auto x = chunks[i];
    T z;
    bool new_borrow;
    if (x == 0 && borrow) {
      z = std::numeric_limits<T>::max() - y;
      new_borrow = true;
    } else if (x - borrow >= y) {
      z = x - T{borrow} - y;
      new_borrow = false;
    } else {
      // Note y cannot be zero, since otherwise 
      // we fall into the previous condition.
      z = (std::numeric_limits<T>::max() - y) + T{1} - T{borrow} + x;
      new_borrow = true;
    }
    chunks[i] = z;
    borrow = new_borrow;
  }

  if (borrow) {
    bool found = false;
    for (; i < size(); ++i) {
      if (!chunks[i]) continue;
      chunks[i] -= 1;
      found = true;
      break;
    }
    assert (found);
  }

  shrink();
  return *this;
}

// multiply-add with carry
template<typename T>
T mac(T a, T b, T c, DoubleLimb<T>& acc) {
  acc += static_cast<DoubleLimb<T>>(a);
  acc += static_cast<DoubleLimb<T>>(b) * static_cast<DoubleLimb<T>>(c);
  T new_carry = static_cast<T>(acc);
  acc >>= sizeof(T) * 8;
  return new_carry;
}

// a += b * c
template<typename T>
void mac_limb(LimbSpan<T> a, LimbSpan<const T> b, T c) {
  if (c == 0) return;
  int n = b.size();
  //assert (a.size() >= n + 2);

  DoubleLimb<T> carry = 0;
  for (int i = 0; i < n; ++i) {
    a[i] = mac(a[i], b[i], c, carry);
  }

  const T final_carry = carry;
  assert ((carry >> (sizeof(T) * 8)) == 0);
  assert (add_limbs(a, {&final_carry, 1}, n) == 0);
  //assert (add_limbs(a.subspan(n), {&final_carry, 1}) == 0);
}

template<typename T>
void mac3(LimbSpan<T> acc, LimbSpan<const T> b, LimbSpan<const T> c) {
  // todo: optimize when b or c have least significant zero limbs
 auto [x, y] = b.size() < c.size() ? std::tie(b, c) : std::tie(c, b);
 // now x is the not-longer of the two
 for (int i = 0; i < x.size(); ++i) {
   mac_limb(acc.subspan(i), y, x[i]);
 } 
}

template<typename T>
std::vector<T> long_multiplication(std::span<const T> a, std::span<const T> b) {
  std::vector<T> new_limbs(a.size() + b.size() + 1);
  mac3(std::span{new_limbs}, a, b);
  shrink_limbs(new_limbs);
  return new_limbs;
}


template<typename T>
num<T>& num<T>::operator*=(const num<T>& other) {
  auto a = std::span<const T>{chunks};
  auto b = std::span<const T>{other.chunks};
#if defined(LONG_MULTIPLICATION)
	chunks = long_multiplication(a, b);
#elif defined(KARATSUBA)
  chunks = karatsuba(a, b);
#endif
  return *this;
}

template<typename T>
std::strong_ordering operator<=>(const num<T>& a, const num<T>& b) {
  int n = a.size(), m = b.size();
  if (n > m) {
    return std::strong_ordering::greater;
  } else if (n < m) {
    return std::strong_ordering::less;
  }
  for (int i = n - 1; i >= 0; --i) {
    if (a.chunks[i] > b.chunks[i]) {
      return std::strong_ordering::greater;
    } else if (a.chunks[i] < b.chunks[i]) {
      return std::strong_ordering::less;
    }
  }
  return std::strong_ordering::equivalent;
}

template<typename T>
bool num<T>::get_bit(int i) const {
  int j = i % (sizeof(T) << 3);
  int k = i / (sizeof(T) << 3);
  return chunks[k] & (T{1} << j);
}

template<typename T>
void num<T>::set_bit(int i, bool b) {
  int j = i % (sizeof(T) << 3);
  int k = i / (sizeof(T) << 3);
  while (k >= size()) chunks.push_back(0);
  if (b) chunks[k] |= (T{1} << j);
  else chunks[k] &= ~(T{1} << j);
}

template<typename T>
num<T>& num<T>::operator<<=(int i) {
  int n = size() - 1;
  int N = chunk_bits();
  int a = (N - 1 + i) % N + 1;
  int b = N - a;
  int d = (i / N) + 1;
  if (i % N == 0) --d;
  for (int i = 0; i < d; ++i) chunks.emplace_back(0);
  for (int i = n; i + d - 1 >= 0; --i) {
    auto x = i >= 0 ? chunks[i] : 0;
    chunks[i + d] = ((chunks[i + d] >> a) << a) | (x >> b);
    chunks[i + d - 1] = (x & ((T{1} << b) - 1)) << a;
  }
  shrink();
  return *this;
}

template<typename T>
std::pair<num<T>, num<T>> num<T>::quot_rem(const num<T>& d) const {
  assert (d.size() != 0);
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


template<typename T>
num<T>::num() = default;

template<typename T>
num<T>::num(T limb) {
  if (limb) chunks = {limb};
}

template<typename T>
num<T>::num(std::string_view v) {
  // super slow, don't care
  num<T> b = 10;
  for (int i = 0; i < v.size(); ++i) {
    char c = v[i];
    if (c > '9' || c < '0') continue;
    *this *= b;
    *this += num(c - '0');
  }
}

template<typename T>
std::string num<T>::dump() const {
  std::ostringstream o;
  o << '{';
  for (int i = 0; i < size(); ++i) {
    if (i > 0) o << ", ";
    o << +chunks[i];
  }
  o << '}';
  return o.str();
}

template<typename T>
T num<T>::get_limb() const {
  assert (chunks.size() <= 1);
  if (chunks.empty()) return 0;
  return chunks[0];
}

template<typename T>
num<T> operator+(const num<T>& x, const num<T>& y) {
  num<T> r = x;
  r += y;
  return r;
}
template<typename T>
num<T> operator-(const num<T>& x, const num<T>& y) {
  num<T> r = x;
  r -= y;
  return r;
}
template<typename T>
num<T> operator*(const num<T>& x, const num<T>& y) {
  num<T> r = x;
  r *= y;
  return r;
}

#define INSTANTIATE(T) \
  template bool add_limbs(LimbSpan<T>, LimbSpan<const T>, int shift = 0); \
  template std::ostream& operator<<(std::ostream&, const num<T>&); \
  template std::strong_ordering operator<=>(const num<T>&, const num<T>&); \
  template num<T>& num<T>::operator+=(const num<T>&); \
  template num<T>& num<T>::operator-=(const num<T>&); \
  template num<T>& num<T>::operator<<=(int); \
  template num<T>& num<T>::operator*=(const num<T>&); \
  template num<T> operator+(const num<T>&, const num<T>&); \
  template num<T> operator-(const num<T>&, const num<T>&); \
  template num<T> operator*(const num<T>&, const num<T>&); \
  template num<T>& num<T>::sub(const num<T>&); \
  template std::pair<num<T>, num<T>> num<T>::quot_rem(const num<T>&) const; \
  template num<T>::num(); \
  template num<T>::num(T); \
  template num<T>::num(std::string_view); \
  template std::string num<T>::dump() const; \
  template bool num<T>::get_bit(int) const; \
  template void num<T>::set_bit(int, bool)

INSTANTIATE(uint8_t);
INSTANTIATE(uint16_t);
INSTANTIATE(uint32_t);
INSTANTIATE(uint64_t);
