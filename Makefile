default: termray

termray.o: termray.c
	gcc -c -fopenmp -O3 termray.c -o termray.o

termray: termray.o
	gcc termray.o -lm -o termray

clean:
	-rm -f termray.o
	-rm -f termray
