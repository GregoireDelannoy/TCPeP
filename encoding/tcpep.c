#include "utils.h"
#include "protocol.h"

#define SO_ORIGINAL_DST 80

/* buffer for reading from tun interface, must be >= 1500 */
#define BUFSIZE 2000     
#define CLIENT 0
#define PROXY 1

char *progname;

/**************************************************************************
 * usage: prints usage and exits.                                                                                 *
 **************************************************************************/
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
    int option, i, j, nread, nwrite, nMux;
    int tcpListenerPort = 0, udpPort = 0;
    struct sockaddr_in local, remote, sourceAccept, destinationAccept, localConnect, remoteConnect;
    char remote_ip[16] = ""; /* dotted quad IP string */
    int udpSock_fd, tcpListenerSock_fd = 0, optval = 1;
    int cliproxy = -1; /* must be specified on cmd line */
    uint8_t buffer[BUFSIZE], tmp[BUFSIZE];
    int selectReturnValue, maxfd;
    fd_set rd_set;
    int newSock;
    muxstate currentMux;
    uint8_t type;
    int dstLen;
    muxstate** muxTable = malloc(sizeof(muxstate*));
    *muxTable = NULL;
    int muxTableLength = 0;
    uint16_t sport; uint16_t dport; uint32_t dip;
    struct timeval currentTime, timeOut;
    
    progname = argv[0];
    
    /* Check command line options */
    while((option = getopt(argc, argv, "hPp:C:t:u:")) > 0) {
        switch(option) {
            case 'h':
                usage();
                break;
            case 'P':
                cliproxy = PROXY;
                break;
            case 'C':
                cliproxy = CLIENT;
                strncpy(remote_ip,optarg,15);
                break;
            case 't':
                tcpListenerPort = atoi(optarg);
                break;
            case 'u':
                udpPort = atoi(optarg);
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

    if(cliproxy < 0) {
        my_err("Must specify client or PROXY mode!\n");
        usage();
    } else if((cliproxy == CLIENT)&&(*remote_ip == '\0')) {
        my_err("Must specify PROXY address!\n");
        usage();
    } else if(udpPort == 0 || (cliproxy == CLIENT && tcpListenerPort == 0 )){
        my_err("Must specify port numbers\n");
        usage();
    }
   
    // Create UDP Socket
    if ( (udpSock_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket()");
        exit(1);
    }

    if(cliproxy == CLIENT) {
        /* Client, try to connect to PROXY */

        /* assign the destination address */
        memset(&remote, 0, sizeof(remote));
        remote.sin_family = AF_INET;
        remote.sin_addr.s_addr = inet_addr(remote_ip);
        remote.sin_port = htons(udpPort);

        memset(&local, 0, sizeof(local));
        local.sin_family = AF_INET;
        local.sin_addr.s_addr = htonl(INADDR_ANY);
        local.sin_port = htons(0); // Bind on any port
        if (bind(udpSock_fd, (struct sockaddr*) &local, sizeof(local)) < 0) {
            perror("bind()");
            exit(1);
        }
        
        udpSend(udpSock_fd, HELO_MSG, 5, (struct sockaddr *)&remote);

        do_debug("CLIENT: Connected to PROXY %s\n", inet_ntoa(remote.sin_addr));
        
        // Create the TCP listener socket
        if ( (tcpListenerSock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            perror("socket()");
            exit(1);
        }
        if(setsockopt(tcpListenerSock_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&optval, sizeof(optval)) < 0) {
            perror("setsockopt()");
            exit(1);
        }
        memset(&local, 0, sizeof(local));
        local.sin_family = AF_INET;
        local.sin_addr.s_addr = htonl(INADDR_ANY);
        local.sin_port = htons(tcpListenerPort);
        if (bind(tcpListenerSock_fd, (struct sockaddr*) &local, sizeof(local)) < 0) {
            perror("bind()");
            exit(1);
        }
        if (listen(tcpListenerSock_fd, 10) < 0) {
            perror("listen()");
            exit(1);
        }
    } else {
        /* PROXY, wait for connections */
        /* avoid EADDRINUSE error on bind() */
        if(setsockopt(udpSock_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&optval, sizeof(optval)) < 0) {
            perror("setsockopt()");
            exit(1);
        }
        
        memset(&local, 0, sizeof(local));
        local.sin_family = AF_INET;
        local.sin_addr.s_addr = htonl(INADDR_ANY);
        local.sin_port = htons(udpPort);
        if (bind(udpSock_fd, (struct sockaddr*) &local, sizeof(local)) < 0) {
            perror("bind()");
            exit(1);
        }
        
        /* wait for connection request */
        memset(&remote, 0, sizeof(remote));
        int destinationLen = sizeof(remote);
        remote.sin_family = AF_INET;
        do_debug("Starting recvfrom...\n");
        recvfrom(udpSock_fd, buffer, BUFSIZE, 0, (struct sockaddr*)&remote, (socklen_t*)&destinationLen);
        if(0 != memcmp(buffer, HELO_MSG, 4)){
            do_debug("Received an error HELO message.\n");
            exit(-1);
        }

        do_debug("PROXY: Client connected from %s:%d\n", inet_ntoa(remote.sin_addr), ntohs(remote.sin_port));
    }
    
    while(1) {
        maxfd = 0;
        FD_ZERO(&rd_set);
        if(cliproxy == CLIENT){
            FD_SET(tcpListenerSock_fd, &rd_set);
            maxfd = max(maxfd, tcpListenerSock_fd);
        }
        FD_SET(udpSock_fd, &rd_set);
        maxfd = max(maxfd, udpSock_fd);
        
        gettimeofday(&currentTime, NULL);
        timeOut.tv_sec = currentTime.tv_sec + 10000000;
        timeOut.tv_usec = 0;
        for(i = 0; i<muxTableLength;i++){
            if(!((*muxTable)[i].closeAwaiting)){
                FD_SET((*muxTable)[i].sock_fd, &rd_set);
                maxfd = max(maxfd, (*muxTable)[i].sock_fd);
            }
            
            // Get the first to timeout
            if(isSooner((*muxTable)[i].encoderState->nextTimeout, timeOut)){
                do_debug("TO for mux #%d (%d,%d) is sooner !\n", i, (int)(*muxTable)[i].encoderState->nextTimeout.tv_sec, (int)(*muxTable)[i].encoderState->nextTimeout.tv_usec);
                timeOut.tv_sec = (*muxTable)[i].encoderState->nextTimeout.tv_sec;
                timeOut.tv_usec = (*muxTable)[i].encoderState->nextTimeout.tv_usec;
            }
        }

        
        if(timeOut.tv_sec - currentTime.tv_sec > 0){
            timeOut.tv_sec = timeOut.tv_sec - currentTime.tv_sec;
            if(timeOut.tv_usec - currentTime.tv_usec > 0){
                timeOut.tv_usec = timeOut.tv_usec - currentTime.tv_usec;
            } else {
                timeOut.tv_usec = 0;
            }
        } else {
            timeOut.tv_sec = 0;
        }
        
        do_debug("\n\n~~~~~~~~~~\nEntering select with TO = %u,%u\n", timeOut.tv_sec, timeOut.tv_usec);
        selectReturnValue = select(maxfd + 1, &rd_set, NULL, NULL, &timeOut);
        
        if (selectReturnValue < 0 && errno == EINTR){ // If the program was woken up due to an interruption, continue
            continue;
        } else if (selectReturnValue < 0) { // If it was another error, die.
            perror("select()");
            exit(1);
        }

        // Check which encoders might be in timeOut
        gettimeofday(&currentTime, NULL);
        for(i = 0; i<muxTableLength;i++){
            if(isSooner((*muxTable)[i].encoderState->nextTimeout, currentTime)){
                onTimeOut((*muxTable)[i].encoderState);
            }
        }


        if(FD_ISSET(udpSock_fd, &rd_set)) {
            nread = cread(udpSock_fd, buffer, BUFSIZE);
            do_debug("Received %d bytes from UDP socket\n", nread);
            muxedToBuffer(buffer, tmp, nread, &dstLen, &currentMux, &type);
            nMux = assignMux(currentMux.sport, currentMux.dport, currentMux.remote_ip, 0, muxTable, &muxTableLength);
            do_debug("Assigned to mux #%d\n", nMux);
            
            if((cliproxy == PROXY) && ((*muxTable)[nMux].sock_fd == 0) && type == TYPE_OPEN){
                do_debug("No regular TCP socket yet.\n");
                newSock = socket(AF_INET, SOCK_STREAM, 0);
                
                memset(&localConnect, 0, sizeof(localConnect));
                localConnect.sin_family = AF_INET;
                localConnect.sin_addr.s_addr = htonl(INADDR_ANY);
                localConnect.sin_port = htons(0); // Bind on any port
                if (bind(newSock, (struct sockaddr*) &localConnect, sizeof(localConnect)) < 0) {
                    perror("bind()");
                    exit(1);
                }
                memset(&remoteConnect, 0, sizeof(remoteConnect));
                remoteConnect.sin_family = AF_INET;
                remoteConnect.sin_addr.s_addr = htonl(currentMux.remote_ip);
                remoteConnect.sin_port = htons(currentMux.dport);
                do_debug("Connecting to %s:%d\n", inet_ntoa(remoteConnect.sin_addr), currentMux.dport);
                if (connect(newSock, (struct sockaddr*) &remoteConnect, sizeof(remoteConnect)) < 0) {
                    perror("connect()");
                    
                    do_debug("Error on connect => we send back a close()\n");
                    bufferToMuxed(buffer, tmp, 0, &dstLen, (*muxTable)[i], TYPE_CLOSE);
                    udpSend(udpSock_fd, tmp, dstLen, (struct sockaddr*)&remote);
                    removeMux(nMux, muxTable, &muxTableLength);
                } else {
                    (*muxTable)[nMux].sock_fd = newSock;
                }
            } else if(type == TYPE_DATA){
                do_debug("TYPE_DATA\n");
                // Pass to the decoder
                handleInCoded((*muxTable)[nMux].decoderState, tmp, dstLen);
            } else if(type == TYPE_ACK){
                do_debug("TYPE_ACK\n");
                // Pass to the encoder
                onAck((*muxTable)[nMux].encoderState, tmp, dstLen);
            } else if(type == TYPE_CLOSE){
                do_debug("TYPE_CLOSE ; closing mux.\n");
                close((*muxTable)[nMux].sock_fd);
                removeMux(nMux, muxTable, &muxTableLength);
            }
        }
        
        if((cliproxy == CLIENT) && (FD_ISSET(tcpListenerSock_fd, &rd_set))){
            // Accept the new connection, as a new Mux
            memset(&sourceAccept, 0, sizeof(sourceAccept));
            int sourceLen = sizeof(sourceAccept);
            sourceAccept.sin_family = AF_INET;
            if((newSock = accept(tcpListenerSock_fd, (struct sockaddr*) &sourceAccept, (socklen_t *)&sourceLen)) < 0){
                perror("accept()");
                exit(1);
            }
            do_debug("Accepted connection from %s:%d ", inet_ntoa(sourceAccept.sin_addr), ntohs(sourceAccept.sin_port));
            // Read the original informations
            memset(&destinationAccept, 0, sizeof(destinationAccept));
            int destinationLen = sizeof(destinationAccept);
            destinationAccept.sin_family = AF_INET;
            getsockopt(newSock, SOL_IP, SO_ORIGINAL_DST, (struct sockaddr *) &destinationAccept, (socklen_t *)&destinationLen);
            
            do_debug("to %s:%d\n", inet_ntoa(destinationAccept.sin_addr), ntohs(destinationAccept.sin_port));
            
            sport = ntohs(sourceAccept.sin_port);
            dport = ntohs(destinationAccept.sin_port);
            dip = ntohl(destinationAccept.sin_addr.s_addr);
            
            nMux = assignMux(sport, dport, dip, newSock, muxTable, &muxTableLength);
            do_debug("Assigned to mux #%d ; sending a TYPE_OPEN\n", nMux);
            
            bufferToMuxed(buffer, tmp, 0, &dstLen, (*muxTable)[nMux], TYPE_OPEN);
            udpSend(udpSock_fd, tmp, dstLen, (struct sockaddr*)&remote);
        }
        
        for(i = 0; i<muxTableLength;i++){
            if(FD_ISSET((*muxTable)[i].sock_fd, &rd_set)){
                if((nread = cread((*muxTable)[i].sock_fd, buffer, BUFSIZE)) <= 0){
                    do_debug("Mux #%d is flagged for closing\n", i);
                    close((*muxTable)[i].sock_fd);
                    (*muxTable)[i].closeAwaiting = true;
                } else {
                    do_debug("Mux #%d has received %d bytes from the TCP socket\n", i, nread);
                    // Pass it to the encoder
                    handleInClear((*muxTable)[i].encoderState, buffer, nread);
                }
            }
        }
        
        //Send data and acks
        for(i = 0; i<muxTableLength;i++){
            do_debug("Sending for mux#%d :\n", i);
            // Send data to the application
            if((*muxTable)[i].decoderState->nDataToSend > 0){
                nwrite = cwrite((*muxTable)[i].sock_fd, (*muxTable)[i].decoderState->dataToSend, (*muxTable)[i].decoderState->nDataToSend);
                do_debug("Sent %d decoded bytes to the application\n", nwrite);
                free((*muxTable)[i].decoderState->dataToSend);
                (*muxTable)[i].decoderState->dataToSend = 0;
                (*muxTable)[i].decoderState->nDataToSend = 0;
            }
            
            // Send ACKs
            for(j = 0; j < (*muxTable)[i].decoderState->nAckToSend; j++){
                bufferToMuxed((*muxTable)[i].decoderState->ackToSend[j], buffer, (*muxTable)[i].decoderState->ackToSendSize[j], &dstLen, (*muxTable)[i], TYPE_ACK);
                nwrite = udpSend(udpSock_fd, buffer, dstLen, (struct sockaddr*)&remote);
                do_debug("Sent a %d bytes ACK\n", nwrite);
            }
            // Free
            for(j = 0; j< (*muxTable)[i].decoderState->nAckToSend;j++){
                free((*muxTable)[i].decoderState->ackToSend[j]);
            }
            if((*muxTable)[i].decoderState->nAckToSend > 0){
                free((*muxTable)[i].decoderState->ackToSend);
                (*muxTable)[i].decoderState->ackToSend = 0;
                free((*muxTable)[i].decoderState->ackToSendSize);
                (*muxTable)[i].decoderState->ackToSendSize = 0;
                (*muxTable)[i].decoderState->nAckToSend = 0;
            }
            
            // Send coded data packets from the encoder
            for(j = 0; j < (*muxTable)[i].encoderState->nDataToSend; j++){
                bufferToMuxed((*muxTable)[i].encoderState->dataToSend[j], buffer, (*muxTable)[i].encoderState->dataToSendSize[j], &dstLen, (*muxTable)[i], TYPE_DATA);
                nwrite = udpSend(udpSock_fd, buffer, dstLen, (struct sockaddr*)&remote);
                do_debug("Sent a %d bytes DATA packet\n", nwrite);
            }
            // Free
            for(j = 0; j< (*muxTable)[i].encoderState->nDataToSend;j++){
                free((*muxTable)[i].encoderState->dataToSend[j]);
            }
            if((*muxTable)[i].encoderState->nDataToSend > 0){
                free((*muxTable)[i].encoderState->dataToSend);
                (*muxTable)[i].encoderState->dataToSend = 0;
                free((*muxTable)[i].encoderState->dataToSendSize);
                (*muxTable)[i].encoderState->dataToSendSize = 0;
                (*muxTable)[i].encoderState->nDataToSend = 0;
            }
            
            if((*muxTable)[i].closeAwaiting && !((*muxTable)[i].encoderState->isOutstandingData)){
                do_debug("Closing Mux %d\n", i);
                bufferToMuxed(buffer, tmp, 0, &dstLen, (*muxTable)[i], TYPE_CLOSE);
                udpSend(udpSock_fd, tmp, dstLen, (struct sockaddr*)&remote);
                removeMux(i, muxTable, &muxTableLength);
                i--; // Go back in the for loop, because of the sliding induced by removeMux();
            }
        }
    }
    
    return(0);
}
