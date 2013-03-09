#include "utils.h"

/**************************************************************************
 * do_debug: prints debugging stuff (doh!)                                                                *
 **************************************************************************/
void do_debug(char *msg, ...){
    
    va_list argp;
    
    if(DEBUG) {
        va_start(argp, msg);
        vfprintf(stderr, msg, argp);
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
