CC = gcc
CFLAGS = -Wall -g

VFLAGS = --track-origins=yes --leak-check=full --show-reachable=yes

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
	valgrind $(VFLAGS) ./examples

test:all
	./test

testMem:all
	valgrind $(VFLAGS) ./test

clean:
	rm examples test *.o
