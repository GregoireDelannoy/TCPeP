#include "utils.h"

/**************************************************************************
 * do_debug: prints debugging stuff (doh!)                                                                *
 **************************************************************************/
void do_debug(char *msg, ...){
    va_list argp;
    
    if(DEBUG) {
        va_start(argp, msg);
        vfprintf(stdout, msg, argp);
        va_end(argp);
    }
}

/**************************************************************************
 * my_err: prints custom error messages on stderr.                                                *
 **************************************************************************/
void my_err(char *msg, ...) {
    va_list argp;
    
    va_start(argp, msg);
    vfprintf(stderr, msg, argp);
    va_end(argp);
}

/**************************************************************************
 * cread: read routine that checks for errors and exits if an error is        *
 *                returned.                                                                                                             *
 **************************************************************************/
int cread(int fd, uint8_t *buf, int n){
    int nread;

    if((nread=read(fd, buf, n)) < 0){
        perror("Reading data");
        exit(1);
    }
    return nread;
}

// Send to UDP socket
int udpSend(int fd, uint8_t *buf, int n, struct sockaddr* remote){
    int ret = sendto(fd, buf, n, 0, remote, 16);
    if( n != ret){
        perror("in udpSend() : sent != than n");
        return -1;
    }
    return ret;
}

/**************************************************************************
 * cwrite: write routine that checks for errors and exits if an error is    *
 *                 returned.                                                                                                            *
 **************************************************************************/
int cwrite(int fd, uint8_t *buf, int n){
    int nwrite, totalWrite = 0;

    while(totalWrite<n){
        if((nwrite=write(fd, buf, n)) < 0){
            perror("Writing data");
            exit(1);
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
        last = malloc(sizeof(struct timeval));
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
