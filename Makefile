PCC=clang++

brute: brute.cpp config.h Makefile
	${PCC} -O3 -march=native -mtune=native -Wall -std=c++11 brute.cpp -o $@
