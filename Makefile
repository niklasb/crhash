CC=clang
PCC=clang++
CFLAGS=-O3 -march=native -mtune=native -Wall
CPPFLAGS=${CFLAGS} -std=c++11

brute: brute.cpp config.h md5.o md5_compress.o Makefile
	${PCC} ${CPPFLAGS} -o $@ \
		brute.cpp md5.o md5_compress.o

md5_compress.o: md5_compress.c Makefile
	${CC} ${CFLAGS} -c -o $@ $<

#md5_compress.o: md5_compress_x86-64.S Makefile
#	${PCC} ${CPPFLAGS} -c -o $@ $<

#md5_compress.o: md5_compress_x86.S Makefile
#	${PCC} ${CPPFLAGS} -c -o $@ $<

md5.o: md5.cpp md5.h Makefile
	${PCC} ${CPPFLAGS} -c -o $@ $<
