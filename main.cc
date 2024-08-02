#include "num.h"
#include "zphi.h"
#include "fastexp.h"
#include <iostream>
#include <iomanip>
#include <cassert>
#include <gmp.h>
#include <gmpxx.h>


int main() {
  //std::cout << fastexp_fib<num<uint8_t>>(100) << std::endl;
  // std::cout << fastexp_fib<num<uint64_t>>(100) << std::endl;
  for (int i = 1; i < 10; ++i) {
    std::cout << fastexp_fib<int>(i) << std::endl;
  }
  //fastexp_fib<num<uint64_t>>(3'300'000ULL);
  //phi<num<uint64_t>> x;
  // x.pow(1'700'000ULL).b;
  //fastexp_fib<mpz_class>(190'500'000ULL);
  //mpz_class n ("199000000");
  //mpz_class x = mpz_class::fibonacci(n);

}

void zphi_test() {
  // zphi<num<uint64_t>> x;
  // x.pow(1'700'000ULL).b;
  //zphi<mpz_class> x;
  //x.pow(48'000'000ULL).b;
  //std::cout << x.pow(1'700'000).b.sgn() << std::endl;
  //mpz_class n ("199000000");
  //mpz_class x = mpz_class::fibonacci(n);
}


void test() {
  /*auto a = num<uint8_t>::from_string("1983924214061919432247806074196061");
  auto b = num<uint8_t>::from_string("3210056809456107725247980776292056");

  auto a = num<uint8_t>::from_string("4654683669341605951078583513940491481410644317152775761745276688300776515962523401661021345087983971670690839477");
  auto b = num<uint8_t>::from_string("7531436383873795315009506236096188648036856522778750817396763797198414070984881727501992372613765176393353441442");
  std::cout << "a: " << a.dump() << std::endl;
  std::cout << "b: " << b.dump() << std::endl;
  b += a;
  std::cout << "s: " << b.dump() << std::endl;
  std::cout << b << std::endl;
  return 0;*/
  constexpr unsigned int n = 100 + 1;
  std::array<num<uint64_t>, n> fibs;
  fibs[0] = 0;
  fibs[1] = 1;
  
  for (int i = 2; i < n; ++i) {
    fibs[i] = fibs[i - 1];
    fibs[i] += fibs[i - 2];
  }
  std::cout << fibs[n - 1] << std::endl;

  for (int i = n - 1; i>=2; --i) {
    std::cout << "i = " << i << ", " << fibs[i] << " - " << fibs[i - 1] << " = ";
    fibs[i] -= fibs[i - 1];
    std::cout << fibs[i] << " (" << fibs[i].dump() << ")" << std::endl;
    assert(fibs[i] == fibs[i - 2]);
  }

  num<uint8_t> m = 1;
  for (int i = 2; i < n; ++i) {
    m *= num<uint8_t>(i);
    std::cout << m << std::endl;
  }

  zphi<num<uint64_t>> x;
  auto z = x.pow(10);
  std::cout << z.a << std::endl;

}
