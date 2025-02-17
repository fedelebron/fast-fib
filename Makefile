CXX=clang++
CXXFLAGS=-O3 -march=native -std=c++2a -stdlib=libc++ -fexperimental-library -Wall -Wextra -pedantic -Wshadow -Wfatal-errors
#CXXFLAGS=-O0 -std=c++2a -stdlib=libc++ -fexperimental-library -Wall -Wextra -pedantic -Wshadow -Wfatal-errors -fsanitize=memory -fsanitize-memory-track-origins -g 

all: main

num.o: num.cc num.h

main.o: main.cc zphi.h fastexp.h

main: main.o num.o
	$(CXX) $(CXXFLAGS) -lgmpxx -lgmp -o $@ main.o num.o

clean:
	rm main *.o
