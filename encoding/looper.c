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

#include "looper.h"
#include <time.h>

void handleIncomingTcpConnected(int sock_fd, muxstate* mux);
void handleIncomingTcpListener(int sock_fd, muxstate** muxTable, int* muxTableLength, struct sockaddr_in remote);
void handleIncomingUdp(int sock_fd, muxstate** muxTable, int* muxTableLength, int cliproxy);

void initializeNetwork(globalstate* state){
    struct sockaddr_in local;
    int optval = 1;
    
    // Create UDP Socket
    if((state->udpSock_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket()");
        exit(1);
    }

    if(state->cliproxy == CLIENT) {
        /* Client, try to connect to PROXY */

        /* assign the destination address */
        memset(&(state->remote), 0, sizeof((state->remote)));
        (state->remote).sin_family = AF_INET;
        (state->remote).sin_addr.s_addr = inet_addr(state->remote_ip);
        (state->remote).sin_port = htons(state->udpPort);

        memset(&local, 0, sizeof(local));
        local.sin_family = AF_INET;
        local.sin_addr.s_addr = htonl(INADDR_ANY);
        local.sin_port = htons(0); // Bind on any port
        if (bind(state->udpSock_fd, (struct sockaddr*) &local, sizeof(local)) < 0) {
            perror("bind()");
            exit(1);
        }
        
        // Create the TCP listener socket
        if ( (state->tcpListenerSock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            perror("socket()");
            exit(1);
        }
        if(setsockopt(state->tcpListenerSock_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&optval, sizeof(optval)) < 0) {
            perror("setsockopt()");
            exit(1);
        }
        memset(&local, 0, sizeof(local));
        local.sin_family = AF_INET;
        local.sin_addr.s_addr = htonl(INADDR_ANY);
        local.sin_port = htons(state->tcpListenerPort);
        if (bind(state->tcpListenerSock_fd, (struct sockaddr*) &local, sizeof(local)) < 0) {
            perror("bind()");
            exit(1);
        }
        if (listen(state->tcpListenerSock_fd, 10) < 0) {
            perror("listen()");
            exit(1);
        }
    } else {
        /* PROXY, wait for connections */
        /* avoid EADDRINUSE error on bind() */
        if(setsockopt(state->udpSock_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&optval, sizeof(optval)) < 0) {
            perror("setsockopt()");
            exit(1);
        }
        
        memset(&local, 0, sizeof(local));
        local.sin_family = AF_INET;
        local.sin_addr.s_addr = htonl(INADDR_ANY);
        local.sin_port = htons(state->udpPort);
        if (bind(state->udpSock_fd, (struct sockaddr*) &local, sizeof(local)) < 0) {
            perror("bind()");
            exit(1);
        }
    }
}

void sendControlPacket(muxstate mux, uint8_t type, int udpSock){
    uint8_t buffer[100];
    int bufLen;
    
    bufferToMuxed(NULL, buffer, 0, &bufLen, mux, type);
    udpSend(udpSock, buffer, bufLen, (struct sockaddr*)&(mux.udpRemote));
}

void infiniteWaitLoop(globalstate* state){
    uint8_t buffer[BUFSIZE];
    int selectReturnValue, maxfd, dstLen, i, j, nwrite;
    fd_set rd_set;
    struct timeval currentTime, timeOut;
    muxstate** muxTable = malloc(sizeof(muxstate*));
    *muxTable = NULL;
    int muxTableLength = 0;
    
    while(1) {
        /* Setting timeout values + max fd */
        maxfd = 0;
        FD_ZERO(&rd_set);
        if(state->cliproxy == CLIENT){
            FD_SET(state->tcpListenerSock_fd, &rd_set);
            maxfd = max(maxfd, state->tcpListenerSock_fd);
        }
        FD_SET(state->udpSock_fd, &rd_set);
        maxfd = max(maxfd, state->udpSock_fd);
        
        gettimeofday(&currentTime, NULL);
        timeOut.tv_sec = currentTime.tv_sec + 10000000;
        timeOut.tv_usec = 0;
        for(i = 0; i < muxTableLength; i++){
            if(((*muxTable)[i].state == STATE_OPENED_DUPLEX) || ((*muxTable)[i].state == STATE_OPENED_SIMPLEX)){
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
        
        /* Select */
        do_debug("\n\n~~~~~~~~~~\nEntering select with TO = %u,%u\n", timeOut.tv_sec, timeOut.tv_usec);
        selectReturnValue = select(maxfd + 1, &rd_set, NULL, NULL, &timeOut);
        do_debug("Select has returned\n");
        
        if (selectReturnValue < 0 && errno == EINTR){ // If the program was woken up due to an interruption, continue
            continue;
        } else if (selectReturnValue < 0) { // It was another error... Print it
            perror("select()");
        }

        // Check which encoders might be in timeOut
        gettimeofday(&currentTime, NULL);
        for(i = 0; i<muxTableLength;i++){
            if(isSooner((*muxTable)[i].encoderState->nextTimeout, currentTime)){
                do_debug("Mux #%d has timed out\n", i);
                onTimeOut((*muxTable)[i].encoderState);
            }
        }

        if(FD_ISSET(state->udpSock_fd, &rd_set)) {
            do_debug("Incoming UDP data\n");
            handleIncomingUdp(state->udpSock_fd, muxTable, &muxTableLength, state->cliproxy);
        }
        
        if((state->cliproxy == CLIENT) && (FD_ISSET(state->tcpListenerSock_fd, &rd_set))){
            do_debug("Incoming TCP on the listener socket\n");
            handleIncomingTcpListener(state->tcpListenerSock_fd, muxTable, &muxTableLength, state->remote);
        }
        
        for(i = 0; i<muxTableLength;i++){
            if(FD_ISSET((*muxTable)[i].sock_fd, &rd_set)){
                do_debug("Incoming TCP on Mux #%d client socket\n", i);
                handleIncomingTcpConnected((*muxTable)[i].sock_fd, &((*muxTable)[i]));
            }
        }
        
        //Send data and acks
        for(i = 0; i<muxTableLength;i++){
            //DEBUG :
            if(regulator()){
                printf("Sending for mux#%d :\n", i);
                printMux((*muxTable)[i]);
            }
            
            // Send data to the application
            if(((*muxTable)[i].state != STATE_CLOSEAWAITING) && ((*muxTable)[i].decoderState->nDataToSend > 0)){
                nwrite = cwrite((*muxTable)[i].sock_fd, (*muxTable)[i].decoderState->dataToSend, (*muxTable)[i].decoderState->nDataToSend);
                do_debug("Sent %d decoded bytes to the application\n", nwrite);
                free((*muxTable)[i].decoderState->dataToSend);
                
                if(nwrite != (*muxTable)[i].decoderState->nDataToSend){ // Error while sending to the application ; we'd better close that mux
                    printf("Error while sending to the application for mux #%d ; closing\n", i);
                    sendControlPacket((*muxTable)[i], TYPE_CLOSE, state->udpSock_fd);
                    removeMux(i, muxTable, &muxTableLength);
                    i--; // Go back in the for loop, because of the sliding induced by removeMux();
                    break;
                }
                
                (*muxTable)[i].decoderState->dataToSend = 0;
                (*muxTable)[i].decoderState->nDataToSend = 0;
            }
            
            // Send ACKs
            for(j = 0; j < (*muxTable)[i].decoderState->nAckToSend; j++){
                bufferToMuxed((*muxTable)[i].decoderState->ackToSend[j], buffer, (*muxTable)[i].decoderState->ackToSendSize[j], &dstLen, (*muxTable)[i], TYPE_ACK);
                nwrite = udpSend(state->udpSock_fd, buffer, dstLen, (struct sockaddr*)&((*muxTable)[i].udpRemote));
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
            
            // If there is no data to send and we are still in SIMPLEX, send an EMPTY packet + set timeout
            if(((*muxTable)[i].state == STATE_OPENED_SIMPLEX) && (*muxTable)[i].encoderState->nDataToSend == 0){
                do_debug("No data to send and state simplex => Send a TYPE_EMPTY\n");
                sendControlPacket((*muxTable)[i], TYPE_EMPTY, state->udpSock_fd);
            }
            
            // Send coded data packets from the encoder
            if(((*muxTable)[i].state == STATE_OPENED_DUPLEX) || ((*muxTable)[i].state == STATE_CLOSEAWAITING) || ((*muxTable)[i].state == STATE_OPENED_SIMPLEX)){
                for(j = 0; j < (*muxTable)[i].encoderState->nDataToSend; j++){
                    bufferToMuxed((*muxTable)[i].encoderState->dataToSend[j], buffer, (*muxTable)[i].encoderState->dataToSendSize[j], &dstLen, (*muxTable)[i], TYPE_DATA);
                    nwrite = udpSend(state->udpSock_fd, buffer, dstLen, (struct sockaddr*)&((*muxTable)[i].udpRemote));
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
            }
            
            if(((*muxTable)[i].state == STATE_CLOSEAWAITING)){
                printf("Mux #%d is awaiting close\n", i);
                if(!((*muxTable)[i].encoderState->isOutstandingData)){
                    printf("Destroying Mux %d after CLOSEAWAITING\n", i);
                    sendControlPacket((*muxTable)[i], TYPE_CLOSE, state->udpSock_fd);
                    removeMux(i, muxTable, &muxTableLength);
                    i--; // Go back in the for loop, because of the sliding induced by removeMux();
                } else {
                    printf("Still outstanding data to send ; trigger a timeout.\n");
                    onTimeOut((*muxTable)[i].encoderState);
                }
            }
        }
    }
}

void handleIncomingUdp(int sock_fd, muxstate** muxTable, int* muxTableLength, int cliproxy){
    struct sockaddr_in localConnect, remoteConnect, udpRemote;
    int destinationLen, nread, nMux, newSock;
    muxstate currentMux;
    uint8_t type;
    uint8_t buffer[BUFSIZE], tmp[BUFSIZE];
    
    memset(&udpRemote, 0, sizeof(udpRemote));
    destinationLen = sizeof(udpRemote);
    udpRemote.sin_family = AF_INET;
    
    nread = recvfrom(sock_fd, buffer, BUFSIZE, 0, (struct sockaddr*)&udpRemote, (socklen_t*)&destinationLen);
    
    do_debug("Received %d bytes from UDP socket from %s:%d\n", nread, inet_ntoa(udpRemote.sin_addr), ntohs(udpRemote.sin_port));
    if(muxedToBuffer(buffer, tmp, nread, &destinationLen, &currentMux, &type)){
        nMux = assignMux(currentMux.sport, currentMux.dport, currentMux.remote_ip, currentMux.randomId, -1, muxTable, muxTableLength, udpRemote);
        do_debug("Assigned to mux #%d\n", nMux);
        
        if((cliproxy == PROXY) && ((*muxTable)[nMux].state == STATE_INIT)){
            do_debug("No regular TCP socket yet.\n");
            newSock = socket(AF_INET, SOCK_STREAM, 0);
            
            memset(&localConnect, 0, sizeof(localConnect));
            localConnect.sin_family = AF_INET;
            localConnect.sin_addr.s_addr = htonl(INADDR_ANY);
            localConnect.sin_port = htons(0); // Bind on any port
            if (bind(newSock, (struct sockaddr*) &localConnect, sizeof(localConnect)) < 0) {
                perror("bind()");
            }
            memset(&remoteConnect, 0, sizeof(remoteConnect));
            remoteConnect.sin_family = AF_INET;
            remoteConnect.sin_addr.s_addr = htonl(currentMux.remote_ip);
            remoteConnect.sin_port = htons(currentMux.dport);
            printf("Connecting to %s:%d\n", inet_ntoa(remoteConnect.sin_addr), currentMux.dport);
            if (connect(newSock, (struct sockaddr*) &remoteConnect, sizeof(remoteConnect)) < 0) {
                perror("connect()");
                
                printf("Error on connect => we send back a close()\n");
                sendControlPacket((*muxTable)[nMux], TYPE_CLOSE, sock_fd);
                removeMux(nMux, muxTable, muxTableLength);
            } else {
                (*muxTable)[nMux].sock_fd = newSock;
                
                do_debug("Connected successfully\n");
                (*muxTable)[nMux].state = STATE_OPENED_DUPLEX;
            }
        } else if(type == TYPE_DATA && (((*muxTable)[nMux].state == STATE_OPENED_DUPLEX) || ((*muxTable)[nMux].state == STATE_OPENED_SIMPLEX))){
            do_debug("TYPE_DATA\n");
            (*muxTable)[nMux].state = STATE_OPENED_DUPLEX;
            // Pass to the decoder
            handleInCoded((*muxTable)[nMux].decoderState, tmp, destinationLen);
        } else if(type == TYPE_ACK && (((*muxTable)[nMux].state == STATE_OPENED_DUPLEX) || ((*muxTable)[nMux].state == STATE_OPENED_SIMPLEX))){
            do_debug("TYPE_ACK\n");
            (*muxTable)[nMux].state = STATE_OPENED_DUPLEX;
            // Pass to the encoder
            onAck((*muxTable)[nMux].encoderState, tmp, destinationLen);
        } else if(type == TYPE_CLOSE){
            do_debug("TYPE_CLOSE ; closing mux #%d.\n", nMux);
            removeMux(nMux, muxTable, muxTableLength);
        } else if(type == TYPE_EMPTY){
            do_debug("TYPE_EMPTY on an already existing mux; nothing to do for mux #%d.\n", nMux);
        } else {
            do_debug("Received packet (%u) did not make sense for mux #%d => Send a TYPE_CLOSE\n", type, nMux);
            sendControlPacket((*muxTable)[nMux], TYPE_CLOSE, sock_fd);
            removeMux(nMux, muxTable, muxTableLength);
        }
    } else {
        do_debug("Received a bogus UDP packet.\n");
    }
}

void handleIncomingTcpListener(int sock_fd, muxstate** muxTable, int* muxTableLength, struct sockaddr_in remote){
    struct sockaddr_in sourceAccept, destinationAccept;
    uint16_t sport; uint16_t dport; uint32_t dip;
    int newSock, nMux;
    
    // Accept the new connection, as a new Mux
    memset(&sourceAccept, 0, sizeof(sourceAccept));
    int sourceLen = sizeof(sourceAccept);
    sourceAccept.sin_family = AF_INET;
    if((newSock = accept(sock_fd, (struct sockaddr*) &sourceAccept, (socklen_t *)&sourceLen)) < 0){
        perror("accept()");
    }
    printf("Accepted connection from %s:%d ", inet_ntoa(sourceAccept.sin_addr), ntohs(sourceAccept.sin_port));
    // Read the original informations
    memset(&destinationAccept, 0, sizeof(destinationAccept));
    int destinationLen = sizeof(destinationAccept);
    destinationAccept.sin_family = AF_INET;
    getsockopt(newSock, SOL_IP, SO_ORIGINAL_DST, (struct sockaddr *) &destinationAccept, (socklen_t *)&destinationLen);
    
    printf("to %s:%d\n", inet_ntoa(destinationAccept.sin_addr), ntohs(destinationAccept.sin_port));
    
    sport = ntohs(sourceAccept.sin_port);
    dport = ntohs(destinationAccept.sin_port);
    dip = ntohl(destinationAccept.sin_addr.s_addr);
    
    srand(time(NULL)); // Initialize the PRNG to a random value
    nMux = assignMux(sport, dport, dip, (uint16_t)random(), newSock, muxTable, muxTableLength, remote);
    do_debug("Assigned to mux #%d\n", nMux);
    (*muxTable)[nMux].state = STATE_OPENED_SIMPLEX;
    
    (*muxTable)[nMux].encoderState->nextTimeout.tv_sec = 0;
    (*muxTable)[nMux].encoderState->nextTimeout.tv_usec = 50000;
}

void handleIncomingTcpConnected(int sock_fd, muxstate* mux){
    uint8_t buffer[BUFSIZE];
    int nread;
    
    if((nread = cread(mux->sock_fd, buffer, BUFSIZE)) == 0){
        do_debug("In handleIncomingTcpConnected : read has returned %d ; close the socket and flag for closing\n", nread);
        mux->state = STATE_CLOSEAWAITING;
    } else {
        do_debug("Mux has received %d bytes from the TCP socket\n", nread);
        // Pass it to the encoder
        handleInClear(mux->encoderState, buffer, nread);
    }
}

void globalStateInit(globalstate* state){
    state->tcpListenerPort = 0;
    state->udpPort = 0;
    state->cliproxy = -1;
    state->remote_ip = calloc(16, sizeof(char)); // 16 chars = notation for quad-dot IPv4
    state->tcpListenerSock_fd = 0;
    state->udpSock_fd = 0;
}

void globalStateFree(globalstate* state){
    free(state->remote_ip);
}
