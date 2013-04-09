CC = gcc
CFLAGS = -Wall -lm -Ofast

VFLAGS = --track-origins=no --leak-check=full --show-reachable=yes

OBJ = galois_field.o matrix.o  packet.o encoding.o decoding.o utils.o protocol.o
HDR = galois_field.h  matrix.h  packet.h  utils.h encoding.h decoding.h protocol.h

%.o: %.c $(HDR)
	$(CC) $(CFLAGS) -c $<

all:$(OBJ) tcpep.o test.o
	$(CC) $(CFLAGS) $(OBJ) tcpep.o -o tcpep
	$(CC) $(CFLAGS) $(OBJ) test.o -o test

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
