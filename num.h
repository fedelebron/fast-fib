#include <vector>
#include <cstdint>
#include <ostream>
#include <compare>
#include <utility>
#include <string_view>
#include <string>

template<typename T = uint64_t>
struct num {
  using chunk_t = T;
  num() = default;
  num(T);
  num(std::string_view);
  template<typename TT>
  friend std::ostream& operator<<(std::ostream&, const num<TT>&);
  template<typename TT>
  friend std::strong_ordering operator<=>(const num<TT>&, const num<TT>&);

  static int chunk_bits();
  int size() const;
  num<T>& operator+=(const num<T>& other);
  num<T>& operator-=(const num<T>& other);
  num<T>& operator*=(const num<T>& other);
  num<T>& operator<<=(int);
  bool operator==(const num<T>&) const = default;

  bool get_bit(int) const;
  void set_bit(int, bool);
  T get_limb() const;

  num<T>& sub(const num<T>& other);

  std::pair<num<T>, num<T>> quot_rem(const num<T>&) const;
  std::string dump() const;

  void shrink();

  std::vector<chunk_t> chunks;
};
  
template<typename T>
num<T> operator+(const num<T>&, const num<T>&);
template<typename T>
num<T> operator-(const num<T>&, const num<T>&);
template<typename T>
num<T> operator*(const num<T>&, const num<T>&);
