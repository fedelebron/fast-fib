#include "num.h"
#include "zphi.h"
#include "fastexp.h"
#include <iostream>
#include <iomanip>
#include <cassert>
#include <gmp.h>
#include <gmpxx.h>
#include <sstream>

void check() {
  for (int i = 1; i < 1'000'000; i *= 10) {
    auto g = mpz_class(mpz_class::fibonacci(i)).get_str();
    std::ostringstream s;
    s << fastexp_fib<num<uint64_t>>(i);
    auto f = s.str();
    if (g != f) {
      std::cerr << "Failed for i = " << i << ", GMP says:\n"
                << g << "\nI say:\n"
                << f << std::endl;
    } else { std::cout << "Passed check for " << i << std::endl; }
  }

}

void bench() {
	fastexp_fib<num<uint64_t>>(11'000'000);
}

int main() {
  //check();
  bench();
}
