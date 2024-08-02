#include <iostream>
template<typename T>
T fastexp_fib(int n) {
  T fk = 1;
  T fkm1 = 0;
  T two = 2;  // wasteful
  unsigned mask = 1 << ((sizeof(int) * 8 - __builtin_clz(n)) - 1);
  while (mask != 1) {
		// We have fk=f(k) and fk=f(k-1), and k becomes
    // either 2k or 2k+1, depending on n&1.
    // We have the equations:
    //
    // f(2k) = f(2k+1) - f(2k-1)
    // f(2k+1) = 4f(k)^2 - f(k-1)^2 + 2*(-1)^k
    // f(2k-1) = f(k)^2 + f(k-1)^2
    fk *= fk;
    fkm1 *= fkm1;
    bool evenk = !(n & mask);
    mask >>= 1;
    bool evenn = !(n & mask);
    
    T fktmp = fk;
    fk <<= 2;
    fk -= fkm1;
		if (evenk) {
			fk += two;
		} else {
			fk -= two;
		}
    // f2kp1 is now stored in fk
   
		fkm1 += fktmp;  // fkm1 now holds f(2k-1) 
    if (!evenn) {
      // We need fk<-f(2k+1), fkm1<-f(2k).
      fkm1 = fk - fkm1;  // grossly inefficient
      //fk = f2kp1;  // already true
    } else {
      // We need fk<-f(2k), fkm1<-f(2k-1).
      // fkm1 = f2km1;  // already true
      fk -= fkm1;
    }
  } 

  return fk;
}

