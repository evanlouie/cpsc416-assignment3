all:
	mpicc -o electleader electleader.c -std=c99

clean:
	rm -f electleader

