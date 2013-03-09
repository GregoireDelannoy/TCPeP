#include "protocol.h"


// Given a raw IP packet, decide wether or not it is a TCP packet
int isTCP(char* buffer, int size){
    if(size > 40){ // If packet size < 40 bytes, it cannot contain both IP and TCP headers.
        // Assuming that we got an IP packet.
        if((buffer[0] & 0xF0) == 0x40){
            if(buffer[9] == 0x06){ // Buffer[9] = protocol field ; 6 = TCP
                return true;
            }
        } else {
            printf("Got an IPv6 packet... HELP !\n");
        }
    }
    
    return false;
}

// Get ports and dst address from a TCP packet
muxinfos extractMuxInfos(char* buffer, int size){
    muxinfos ret;
    
    int ipHdrLen = buffer[0] & 0x0F;
    printf("IP header length = %d\n", ipHdrLen);
    
    memcpy(&(ret.remote_ip), buffer + 15 , 4);
    
    memcpy(&(ret.sport), buffer + 4*ipHdrLen, 2);
    ret.sport = ntohs(ret.sport);
    memcpy(&(ret.dport), buffer + (4*ipHdrLen + 2), 2);
    ret.dport = ntohs(ret.dport);
    
    return ret;
}
