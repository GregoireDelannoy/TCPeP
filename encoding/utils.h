/* Copyright 2013 Gregoire Delannoy
 * 
 * This file is a part of TCPeP.
 * 
 * TCPeP is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 */

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
#include <signal.h>

#define min(a,b) a<b?a:b
#define max(a,b) a>b?a:b

#define true 1==1
#define false 1==0
#define DEBUG false

#define BLKSIZE 127 // Block size (in number of packets)
#define PACKETSIZE 1380 // Maximum payload size (in bytes)

#define REGULATOR 5000 // Make the regulator function return true every REGULATOR milliseconds

void do_debug(char *msg, ...);

void my_err(char *msg, ...);

int cread(int fd, uint8_t *buf, int n);

int cwrite(int fd, uint8_t *buf, int n);

int udpSend(int fd, uint8_t *buf, int n, struct sockaddr* remote);

int isSooner(struct timeval a, struct timeval b);

void addUSec(struct timeval *a, long t);

int regulator();

#endif


