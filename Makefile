.PHONY: all clean

CXX=g++ -std=c++20 -O2

all: FO-prover

FO-prover: main.o fdpll.o skolem.o combination.o parser.o
	$(CXX) parser.o combination.o skolem.o fdpll.o main.o -o FO-prover

main.o: fdpll.h main.cpp
	$(CXX) main.cpp -c -o main.o

fdpll.o: skolem.h fdpll.h fdpll.cpp
	$(CXX) fdpll.cpp -c -o fdpll.o

skolem.o: parser.h skolem.h skolem.cpp
	$(CXX) skolem.cpp -c -o skolem.o

combination.o: combination.h combination.cpp
	$(CXX) combination.cpp -c -o combination.o

parser.o: parser.h parser.cpp
	$(CXX) parser.cpp -c -o parser.o

clean:
	rm FO-prover *.o