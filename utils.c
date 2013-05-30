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

#include "utils.h"

void do_debug(char *msg, ...){
    va_list argp;
    
    if(DEBUG) {
        va_start(argp, msg);
        vfprintf(stdout, msg, argp);
        va_end(argp);
    }
}

void my_err(char *msg, ...) {
    va_list argp;
    
    va_start(argp, msg);
    vfprintf(stderr, msg, argp);
    va_end(argp);
}

int cread(int fd, uint8_t *buf, int n){
    int nread;

    if((nread=read(fd, buf, n)) < 0){
        perror("in utils.c : Reading data");
        return -1;
    }
    return nread;
}

int udpSend(int fd, uint8_t *buf, int n, struct sockaddr* remote){
    int ret = sendto(fd, buf, n, 0, remote, 16);
    if( n != ret){
        perror("in udpSend() : sent != n");
        return -1;
    }
    return ret;
}

int cwrite(int fd, uint8_t *buf, int n){
    int nwrite, totalWrite = 0;

    while(totalWrite<n){
        if((nwrite=write(fd, buf, n)) < 0){
            perror("In utils.c: Writing data");
            return(-1);
        } else {
            totalWrite += nwrite;
        }
    }
    return totalWrite;
}

// Returns true if a < b
int isSooner(struct timeval a, struct timeval b){
    int ret;
    
    if(a.tv_sec == 0 && a.tv_usec == 0){
        ret = false;
    } else if(b.tv_sec == 0 && b.tv_usec == 0){ // Value of 0 => time = infinity !
        ret = true;
    }else if(a.tv_sec > b.tv_sec){
        ret = false;
    } else if(a.tv_sec < b.tv_sec){
        ret = true;
    } else if(a.tv_usec > b.tv_usec){
        ret = false;
    } else {
        ret = true;
    }
    
    return ret;
}

void addUSec(struct timeval *a, long t){
    a->tv_usec += t;
    while(a->tv_usec > 1000000){
        a->tv_sec ++;
        a->tv_usec -= 1000000;
    }
}

int regulator(){
    static struct timeval *last = 0;
    struct timeval current, tmp;
    int ret;
    gettimeofday(&current, NULL);
    
    if(last == 0){
        last = malloc(sizeof(struct timeval)); // Note : that will cause a memory leak of 16 bytes. Not dependant on anything.
        gettimeofday(last, NULL);
    }

    tmp.tv_sec = last->tv_sec;
    tmp.tv_usec = last->tv_usec;
    addUSec(&tmp, 1000 * REGULATOR);
    if(isSooner(tmp, current)){
        ret = true;
        last->tv_sec = current.tv_sec;
        last->tv_usec = current.tv_usec;
    } else {
        ret = false;
    }
    
    return ret;
}
