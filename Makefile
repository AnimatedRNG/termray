default: termray

termray: termray.c
	gcc -O3 -fopenmp -lm termray.c -o termray

clean:
	-rm -f termray
