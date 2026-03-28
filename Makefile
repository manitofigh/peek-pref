all:
	gcc -O2 -o pkp -Wall ./pkp.c

asm:
	gcc -S -o pkp.asm pkp.c

clean:
	rm -rf ./pkp
