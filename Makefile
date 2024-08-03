CXX=clang++
CXXFLAGS=-O3 -std=c++2a -stdlib=libc++ -fexperimental-library

all: main

num.o: num.cc num.h

main.o: main.cc zphi.h fastexp.h

main: main.o num.o
	$(CXX) $(CXXFLAGS) -lgmpxx -lgmp -o $@ main.o num.o

clean:
	rm main *.o
