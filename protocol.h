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

#ifndef _PROTOCOL_
#define _PROTOCOL_
#include "utils.h"
#include "packet.h"
#include "decoding.h"
#include "encoding.h"

#define TYPE_DATA 0x00
#define TYPE_ACK 0x01
#define TYPE_CLOSE 0x02
#define TYPE_EMPTY 0x03
#define TYPE_READ_CLOSED 0x04
#define TYPE_READ_CLOSED_ACK 0x05
#define TYPE_WRITE_CLOSED 0x06
#define TYPE_WRITE_CLOSED_ACK 0x07
#define TYPE_NO_OUTSTANDING_DATA 0x08
#define TYPE_NO_OUTSTANDING_DATA_ACK 0x09

#define STATE_OPENED_SIMPLEX 0x02
#define STATE_OPENED_DUPLEX 0x03
#define STATE_INIT 0x04

#define SOCKET_INIT 0x00
#define SOCKET_OPENED 0x01
#define SOCKET_CLOSED_NOT_ACKNOWLDGED 0x02
#define SOCKET_CLOSED_ACKNOWLDGED 0x03

#define STATE_RETRANSMIT_TIMEOUT 500000

typedef struct muxstate_t {
    int sock_fd;    // local TCP socket
    
    // From the perspective of the client :
    uint16_t sport; // Source port (client application TCP socket)
    uint16_t dport; // Destination port (Server application)
    uint32_t remote_ip; // Remote IP Address (Server Address)
    struct sockaddr_in udpRemote; // Remote UDP endpoint : either client system or proxy system
    
    uint16_t randomId; // Random connection identifier
    
    // Encoder and decoder structures
    encoderstate* encoderState;
    decoderstate* decoderState;

    // State of the connection
    int state;
    
    // State of the TCP socket on both endpoints
    int localSocketReadState;
    int localSocketWriteState;
    int remoteSocketReadState;
    int remoteSocketWriteState;
    struct timeval lastWriteSent;
    
    // State of the remaining data
    int remoteOutstandingData;
    int localOutstandingData;
    struct timeval lastOutstandingSent;
    
} muxstate;

int assignMux(uint16_t sport, uint16_t dport, uint32_t remote_ip, uint16_t randomId, int sock_fd, muxstate** statesTable, int* tableLength, struct sockaddr_in udpRemoteAddr);
void removeMux(int i, muxstate** statesTable, int* tableLength);

void bufferToMuxed(uint8_t* src, uint8_t* dst, int srcLen, int* dstLen, muxstate mux, uint8_t type);

int muxedToBuffer(uint8_t* src, uint8_t* dst, int srcLen, int* dstLen, muxstate* mux, uint8_t* type);

void printMux(muxstate mux);
#endif



