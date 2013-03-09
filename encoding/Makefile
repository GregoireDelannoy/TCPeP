CC = gcc
CFLAGS = -Wall -g

VFLAGS = --track-origins=yes --leak-check=full --show-reachable=yes

OBJ = galois_field.o matrix.o  packet.o tun.o coding.o utils.o protocol.o
HDR = galois_field.h  matrix.h  packet.h  structures.h  utils.h tun.h coding.h protocol.h

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
