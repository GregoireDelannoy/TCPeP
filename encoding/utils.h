#ifndef _UTILS_
#define _UTILS_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <stdarg.h>


#include <stdint.h>
#include "matrix.h"
#include "packet.h"

#define min(a,b) a<b?a:b
#define max(a,b) a>b?a:b

#define true 1==1
#define false 1==0
#define DEBUG true

void do_debug(char *msg, ...);

void my_err(char *msg, ...);

#endif


