CC = gcc
CFLAGS = -Wall -g

OBJ = galois_field.o matrix.o  packet.o

%.o: %.c
	$(CC) $(CFLAGS) -c $<

all:$(OBJ) examples.o test.o
	gcc -g -Wall $(OBJ) examples.o -o examples
	gcc -g -Wall $(OBJ) test.o -o test

run:all
	./examples

runMem:all
	valgrind --leak-check=full ./examples

test:all
	./test

testMem:all
	valgrind --leak-check=full ./test

clean:
	rm examples test *.o
