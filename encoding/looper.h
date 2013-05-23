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
