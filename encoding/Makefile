CC = gcc
CFLAGS = -Wall -g -lm -O0

VFLAGS = --track-origins=yes --leak-check=full --show-reachable=yes -v

OBJ = galois_field.o matrix.o  packet.o encoding.o decoding.o utils.o protocol.o
HDR = galois_field.h  matrix.h  packet.h  utils.h encoding.h decoding.h protocol.h

%.o: %.c $(HDR)
	$(CC) $(CFLAGS) -c $<

all:$(OBJ) tcpep.o test.o
	gcc $(CFLAGS) $(OBJ) tcpep.o -o tcpep
	gcc $(CFLAGS) $(OBJ) test.o -o test

run:all
	./tcpep

runMem:all
	valgrind $(VFLAGS) ./tcpep

test:all
	./test

testMem:all
	valgrind $(VFLAGS) ./test

clean:
	rm tcpep test *.o
