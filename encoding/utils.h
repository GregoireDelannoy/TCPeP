#ifndef _UTILS_
#define _UTILS_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <stdarg.h>
#include <stdint.h>
#include <sys/socket.h>
#include <arpa/inet.h> 
#include <sys/select.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <math.h>

#define min(a,b) a<b?a:b
#define max(a,b) a>b?a:b

#define true 1==1
#define false 1==0
#define DEBUG false

#define BLKSIZE 127 // Block size (in number of packets)
#define PACKETSIZE 1300 // Maximum payload size (in bytes)

void do_debug(char *msg, ...);

void my_err(char *msg, ...);

int cread(int fd, uint8_t *buf, int n);

int cwrite(int fd, uint8_t *buf, int n);

int udpSend(int fd, uint8_t *buf, int n, struct sockaddr* remote);

int isSooner(struct timeval a, struct timeval b);

void addUSec(struct timeval *a, long t);

#endif


