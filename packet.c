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

#include "packet.h"

void dataPacketToBuffer(datapacket p, uint8_t* buffer, int* size){
    uint8_t tmp8;
    uint16_t tmp16;
    uint32_t tmp32;
    
    tmp16 = htons(p.blockNo);
    memcpy(buffer, &tmp16, 2);
    tmp8 = p.packetNumber;
    memcpy(buffer + 2, &tmp8, 1);
    tmp32 = htonl(p.seqNo);
    memcpy(buffer + 3, &tmp32, 4);
    
    memcpy(buffer + 7, p.payloadAndSize, p.size);
    
    (*size) = 7 + p.size;
}

datapacket* bufferToData(uint8_t* buffer, int size){
    uint8_t tmp8;
    uint16_t tmp16;
    uint32_t tmp32;
    datapacket* p = malloc(sizeof(datapacket));
    
    memcpy(&tmp16, buffer, 2);
    p->blockNo = htons(tmp16);
    memcpy(&tmp8, buffer + 2, 1);
    p->packetNumber = tmp8;
    memcpy(&tmp32, buffer + 3, 4);
    p->seqNo = ntohl(tmp32);
    
    p->payloadAndSize = malloc((size - 7) * sizeof(uint8_t));
    memcpy(p->payloadAndSize, buffer + 7, size - 7);
    p->size = size - 7;
    
    return p;
}

void ackPacketToBuffer(ackpacket p, uint8_t* buffer, int* size){
    int i;
    uint8_t tmp8;
    uint16_t tmp16;
    uint32_t tmp32;
    
    tmp16 = htons(p.ack_currBlock);
    memcpy(buffer, &tmp16, 2);
    tmp32 = htonl(p.ack_seqNo);
    memcpy(buffer + 2, &tmp32, 4);
    tmp16 = htons(p.ack_loss);
    memcpy(buffer + 6, &tmp16, 2);
    tmp16 = htons(p.ack_total);
    memcpy(buffer + 8, &tmp16, 2);
    for(i = 0; i < DOFS_LENGTH; i++){
        tmp8 = p.ack_dofs[i];
        memcpy(buffer + 10 + i, &tmp8, 1);
    }
    
    
    (*size) = 10 + DOFS_LENGTH;
}

ackpacket* bufferToAck(uint8_t* buffer, int size){
    int i;
    if(size != 10 + DOFS_LENGTH){
        printf("Buffer to ack => size is not what was expected. DIE.\n");
        exit(1);
    }
    uint8_t tmp8;
    uint16_t tmp16;
    uint32_t tmp32;
    ackpacket* p = malloc(sizeof(ackpacket));
    p->ack_dofs = malloc(DOFS_LENGTH * sizeof(uint8_t));
    
    memcpy(&tmp16, buffer, 2);
    p->ack_currBlock = ntohs(tmp16);
    memcpy(&tmp32, buffer + 2, 4);
    p->ack_seqNo = ntohl(tmp32);
    memcpy(&tmp16, buffer + 6, 2);
    p->ack_loss = ntohs(tmp16);
    memcpy(&tmp16, buffer + 8, 2);
    p->ack_total = ntohs(tmp16);
    
    for(i = 0; i < DOFS_LENGTH; i++){
        memcpy(&tmp8, buffer + 10 + i, 1);
        p->ack_dofs[i] = tmp8;
    }
    
    return p;
}

void dataPacketPrint(datapacket p){
    printf("DataPacket :\n");
    printf("\tBlock No = %u\n", p.blockNo);
    if((p.packetNumber & BITMASK_FLAG) == FLAG_CLEAR){
        printf("\tClear Packet\n");
    } else if((p.packetNumber & BITMASK_FLAG) == FLAG_CODED){
        printf("\tCoded Packet\n");
    } else {
        printf("\tIs neither clear or coded... ?!?\n");
    }
    printf("\tPacket No = %u\n", (p.packetNumber & BITMASK_NO));
    
    printf("\tSeq No = %u\n", p.seqNo);
    printf("\tSize = %d\n", p.size);
    
    printf("\tPayload start : ");
    int i;
    for(i=0;i<(min(p.size, 10));i++){
        printf("%2x ", p.payloadAndSize[i]);
    }
    printf("\n");
}

void ackPacketPrint(ackpacket p){
    int i;
    printf("AckPacket :\n");
    printf("\tcurrBlock = %u\n", p.ack_currBlock);
    printf("\tAck Seq No = %u\n", p.ack_seqNo);
    for(i = 0; i < DOFS_LENGTH; i++){
        printf("\tdofs for block %i = %u\n", i, p.ack_dofs[i]);
    }
    printf("\tloss = %u\n", p.ack_loss);
    printf("\ttotal = %u\n", p.ack_total);
}
