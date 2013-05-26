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

#ifndef _LOOPER_
#define _LOOPER_

#include "utils.h"
#include "protocol.h"

#define SO_ORIGINAL_DST 80

/* buffer for reading from sockets, must be >= 1500 */
#define BUFSIZE 4096
#define CLIENT 0
#define PROXY 1

typedef struct globalstate_t{ // Contains information that needs to be passed from main to init_network to loop
    int tcpListenerPort;
    int udpPort;
    int tcpListenerSock_fd;
    int udpSock_fd;
    
    int cliproxy;
    
    char *remote_ip;
    
    struct sockaddr_in remote; // The proxy UDP endpoint, if we are client. NULL otherwise.
} globalstate;

void initializeNetwork(globalstate* state);
void infiniteWaitLoop(globalstate* state);

void globalStateInit(globalstate* state);
void globalStateFree(globalstate* state);

#endif
