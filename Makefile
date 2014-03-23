all:
	mpicc -o electleader electleader.c -std=c11

clean:
	rm -f electleader

