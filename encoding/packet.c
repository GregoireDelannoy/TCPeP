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
    uint8_t tmp8;
    uint16_t tmp16;
    uint32_t tmp32;
    
    tmp16 = htons(p.ack_currBlock);
    memcpy(buffer, &tmp16, 2);
    tmp8 = p.ack_currDof;
    memcpy(buffer + 2, &tmp8, 1);
    tmp32 = htonl(p.ack_seqNo);
    memcpy(buffer + 3, &tmp32, 4);
    
    (*size) = 7;
}

ackpacket* bufferToAck(uint8_t* buffer, int size){
    if(size != 7){
        printf("Buffer to ack => size != 7. DIE.\n");
        exit(1);
    }
    uint8_t tmp8;
    uint16_t tmp16;
    uint32_t tmp32;
    ackpacket* p = malloc(sizeof(ackpacket));
    
    memcpy(&tmp16, buffer, 2);
    p->ack_currBlock = ntohs(tmp16);
    memcpy(&tmp8, buffer + 2, 1);
    p->ack_currDof = tmp8;
    memcpy(&tmp32, buffer + 3, 4);
    p->ack_seqNo = ntohl(tmp32);
    
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
    printf("AckPacket :\n");
    printf("\tcurrBlock = %u\n", p.ack_currBlock);
    printf("\tSeq No = %u\n", p.ack_seqNo);
    printf("\tcurrDof = %u\n", p.ack_currDof);
}
