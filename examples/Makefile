CC=mpicc #gcc
CFLAGS=-lmpi
RUN=mpirun -np 2
fscount: main.c
	${CC} main.c -g -o wcount ${CFLAGS}
run: wcount
	${RUN} ./wcount
