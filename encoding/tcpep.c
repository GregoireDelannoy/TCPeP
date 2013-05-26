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
#include "looper.h"

char *progname;

void usage(void) {
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "-h: Print this message\n");
    fprintf(stderr, "-P: Proxy mode\n");
    fprintf(stderr, "-C <proxy IP address>: Client mode\n");
    fprintf(stderr, "-t <TCP port to listen on> (client only)\n");
    fprintf(stderr, "-u <UDP port to use>\n");
    exit(1);
}


int main(int argc, char *argv[]) {
    int option;
    globalstate* globalState = malloc(sizeof(globalstate));
    globalStateInit(globalState);
    
    /* Check command line options */
    progname = argv[0];
    while((option = getopt(argc, argv, "hPp:C:t:u:")) > 0) {
        switch(option) {
            case 'h':
                usage();
                break;
            case 'P':
                globalState->cliproxy = PROXY;
                break;
            case 'C':
                globalState->cliproxy = CLIENT;
                strncpy(globalState->remote_ip,optarg,15);
                break;
            case 't':
                globalState->tcpListenerPort = atoi(optarg);
                break;
            case 'u':
                globalState->udpPort = atoi(optarg);
                break;
            default:
                my_err("Unknown option %c\n", option);
                usage();
        }
    }

    argv += optind;
    argc -= optind;

    if(argc > 0) {
        my_err("Too many options!\n");
        usage();
    }

    if(globalState->cliproxy < 0) {
        my_err("Must specify client or PROXY mode!\n");
        usage();
    } else if((globalState->cliproxy == CLIENT)&&(globalState->remote_ip[0] == '\0')) {
        my_err("Must specify PROXY address!\n");
        usage();
    } else if(globalState->udpPort == 0 || (globalState->cliproxy == CLIENT && globalState->tcpListenerPort == 0 )){
        my_err("Must specify port numbers\n");
        usage();
    }

    /* Initialize the network : create and bind sockets */
    initializeNetwork(globalState);
    
    /* Start the infinite loop */
    infiniteWaitLoop(globalState);
    
    // Will never be reached, kept for compliance
    globalStateFree(globalState);
    free(globalState);
    return(0);
}
