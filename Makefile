PCC=clang++

md5_brute: brute.cpp md5.c md5.h Makefile
	${PCC} -O3 -Wall -std=c++11 brute.cpp md5.c -o $@
