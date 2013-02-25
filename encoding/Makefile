galois_field.o:galois_field.c
	gcc -c -g -Wall galois_field.c -o galois_field.o

matrix.o:matrix.c
	gcc -c -g -Wall matrix.c -o matrix.o

examples.o:examples.c
	gcc -c -g -Wall examples.c -o examples.o

test.o:test.c
	gcc -c -g -Wall test.c -o test.o

all:examples.o galois_field.o test.o matrix.o
	gcc -g -Wall examples.o galois_field.o matrix.o -o examples
	gcc -g -Wall test.o galois_field.o matrix.o -o test

run:all
	./examples

test:all
	./test

testMem:all
	valgrind --leak-check=full ./test

clean:
	rm examples *.o
