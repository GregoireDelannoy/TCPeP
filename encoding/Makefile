CC = gcc
CFLAGS = -Wall -g

OBJ = galois_field.o matrix.o test.o examples.o

%.o: %.c
	$(CC) $(CFLAGS) -c $<

all:$(OBJ)
	gcc -g -Wall examples.o galois_field.o matrix.o -o examples
	gcc -g -Wall test.o galois_field.o matrix.o -o test

run:all
	./examples

test:all
	./test

testMem:all
	valgrind --leak-check=full ./test

clean:
	rm examples test *.o
