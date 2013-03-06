CC = gcc
CFLAGS = -Wall -g -pg

OBJ = galois_field.o matrix.o  packet.o
HDR = galois_field.h  matrix.h  packet.h  structures.h  utils.h

%.o: %.c $(HDR)
	$(CC) $(CFLAGS) -c $<

all:$(OBJ) examples.o test.o
	gcc $(CFLAGS) $(OBJ) examples.o -o examples
	gcc $(CFLAGS) $(OBJ) test.o -o test

run:all
	./examples

runMem:all
	valgrind --track-origins=yes --leak-check=full ./examples

test:all
	./test

testMem:all
	valgrind --leak-check=full ./test

clean:
	rm examples test *.o
