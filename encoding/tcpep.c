#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <net/if.h>
#include <linux/if_tun.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h> 
#include <sys/select.h>
#include <sys/time.h>
#include <errno.h>
#include <stdarg.h>
#include <netinet/in.h> 
#include <netdb.h> 

#include "tun.h"
#include "utils.h"
#include "protocol.h"
#include "coding.h"

/* buffer for reading from tun interface, must be >= 1500 */
#define BUFSIZE 2000     
#define CLIENT 0
#define SERVER 1
#define PORT 55555
#define MTU 1500 - (28 + 8 + 5 + 6 * CODING_WINDOW)
// mtu = standard MTU - overhead ( = IP + UDP + Mux info + Coding infos) 

char *progname;



/**************************************************************************
 * cread: read routine that checks for errors and exits if an error is        *
 *                returned.                                                                                                             *
 **************************************************************************/
int cread(int fd, char *buf, int n){
    
    int nread;

    if((nread=read(fd, buf, n)) < 0){
        perror("Reading data");
        exit(1);
    }
    return nread;
}

// Send to UDP socket
int udpSend(int fd, char *buf, int n, struct sockaddr* remote){
    int ret = sendto(fd, buf, n, 0, remote, sizeof(struct sockaddr_in));
    if( n != ret){
        perror("in udpSend() : sent != than n");
        return -1;
    }
    return ret;
}

/**************************************************************************
 * cwrite: write routine that checks for errors and exits if an error is    *
 *                 returned.                                                                                                            *
 **************************************************************************/
int cwrite(int fd, char *buf, int n){
    
    int nwrite;

    if((nwrite=write(fd, buf, n)) < 0){
        perror("Writing data");
        exit(1);
    }
    return nwrite;
}

/**************************************************************************
 * usage: prints usage and exits.                                                                                 *
 **************************************************************************/
void usage(void) {
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "%s -i <ifacename> [-s|-c <serverIP>] [-p <port>] [-u|-a] [-d]\n", progname);
    fprintf(stderr, "%s -h\n", progname);
    fprintf(stderr, "\n");
    fprintf(stderr, "-i <ifacename>: Name of interface to use (mandatory)\n");
    fprintf(stderr, "-s|-c <serverIP>: run in server mode (-s), or specify server address (-c <serverIP>) (mandatory)\n");
    fprintf(stderr, "-p <port>: port to listen on (if run in server mode) or to connect to (in client mode), default 55555\n");
    //fprintf(stderr, "-d: outputs debug information while running\n");
    fprintf(stderr, "-h: prints this help text\n");
    exit(1);
}

int main(int argc, char *argv[]) {
    int tun_fd, option, i, nMux;
    char if_name[IFNAMSIZ] = "tun0";
    int maxfd;
    uint16_t nread, nwrite;
    char buffer[BUFSIZE], tmp[BUFSIZE];
    struct sockaddr_in local, remote;
    char remote_ip[16] = "";     /* dotted quad IP string */
    unsigned short int port = PORT;
    int sock_fd, optval = 1;
    int cliserv = -1;        /* must be specified on cmd line */
    unsigned long int tun2net = 0, net2tun = 0;
    uint16_t sport;
    uint16_t dport;
    uint32_t sip, dip;
    clearpacketarray* clearPacketArray;
    clearpacket* tmpClearPacket;
    encodedpacketarray* encodedPacketArray;
    encodedpacket* tmpEncodedPacket;
    int ipLength;
    
    char nullMux[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    
    muxstate* muxTable = 0;
    int tableLength = 0;

    progname = argv[0];
    
    /* Check command line options */
    while((option = getopt(argc, argv, "i:sc:p:uah")) > 0) {
        switch(option) {
            case 'h':
                usage();
                break;
            case 'i':
                strncpy(if_name,optarg, IFNAMSIZ-1);
                break;
            case 's':
                cliserv = SERVER;
                break;
            case 'c':
                cliserv = CLIENT;
                strncpy(remote_ip,optarg,15);
                break;
            case 'p':
                port = atoi(optarg);
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

    if(*if_name == '\0') {
        my_err("Must specify interface name!\n");
        usage();
    } else if(cliserv < 0) {
        my_err("Must specify client or server mode!\n");
        usage();
    } else if((cliserv == CLIENT)&&(*remote_ip == '\0')) {
        my_err("Must specify server address!\n");
        usage();
    }

    /* initialize tun interface */
    if ( (tun_fd = tun_alloc(if_name, IFF_TUN | IFF_NO_PI, MTU)) < 0 ) {
        my_err("Error connecting to tun interface %s!\n", if_name);
        exit(1);
    }

    do_debug("Successfully connected to interface %s\n", if_name);

    if ( (sock_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket()");
        exit(1);
    }

    if(cliserv == CLIENT) {
        /* Client, try to connect to server */

        /* assign the destination address */
        memset(&remote, 0, sizeof(remote));
        remote.sin_family = AF_INET;
        remote.sin_addr.s_addr = inet_addr(remote_ip);
        remote.sin_port = htons(port);

        memset(&local, 0, sizeof(local));
        local.sin_family = AF_INET;
        local.sin_addr.s_addr = htonl(INADDR_ANY);
        local.sin_port = htons(0); // Bind on any port
        if (bind(sock_fd, (struct sockaddr*) &local, sizeof(local)) < 0) {
            perror("bind()");
            exit(1);
        }
        
        udpSend(sock_fd, HELO_MSG, 5, (struct sockaddr *)&remote);

        do_debug("CLIENT: Connected to server %s\n", inet_ntoa(remote.sin_addr));
        
    } else {
        /* Server, wait for connections */
        /* avoid EADDRINUSE error on bind() */
        if(setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&optval, sizeof(optval)) < 0) {
            perror("setsockopt()");
            exit(1);
        }
        
        memset(&local, 0, sizeof(local));
        local.sin_family = AF_INET;
        local.sin_addr.s_addr = htonl(INADDR_ANY);
        local.sin_port = htons(1337);
        if (bind(sock_fd, (struct sockaddr*) &local, sizeof(local)) < 0) {
            perror("bind()");
            exit(1);
        }
        

        
        /* wait for connection request */
        memset(&remote, 0, sizeof(remote));
        int remotelen = sizeof(remote);
        remote.sin_family = AF_INET;
        do_debug("Starting recvfrom...\n");
        recvfrom(sock_fd, buffer, BUFSIZE, 0, (struct sockaddr*)&remote, (socklen_t*)&remotelen);
        if(0 != memcmp(buffer, HELO_MSG, 4)){
            do_debug("Received an error HELO message.\n");
            exit(-1);
        }

        do_debug("SERVER: Client connected from %s:%d\n", inet_ntoa(remote.sin_addr), ntohs(remote.sin_port));
    }
    
    /* use select() to handle two descriptors at once */
    maxfd = (tun_fd > sock_fd)?tun_fd:sock_fd;

    while(1) {
        int ret;
        fd_set rd_set;

        FD_ZERO(&rd_set);
        FD_SET(tun_fd, &rd_set); FD_SET(sock_fd, &rd_set);

        //ret = select(maxfd + 1, &rd_set, NULL, NULL, NULL);
        do_debug("\n\n~~~~~~~~~~\nEntering select\n");
        ret = select(maxfd + 1, &rd_set, NULL, NULL, NULL);

        if (ret < 0 && errno == EINTR){ // If the program was woken up due to an interruption, continue
            continue;
        } else if (ret < 0) { // If it was another error, die.
            perror("select()");
            exit(1);
        }

        if(FD_ISSET(tun_fd, &rd_set)) {
            /* data from tun: just read it and write it to the network */
            nread = cread(tun_fd, buffer, BUFSIZE);

            tun2net++;
            do_debug("TUN2NET %lu: Read %d bytes from the tun interface :\n", tun2net, nread);
            
            if(isTCP(buffer, nread)){
                ipLength = ipHeaderLength(buffer);
                do_debug("TUN2NET %lu: Received packet is TCP. IP Hdr length = %d\n", tun2net, ipLength);
                
                if(cliserv == CLIENT){
                    extractMuxInfosFromIP(buffer, nread, &sport, &dport, &sip, &dip);
                } else { // Note : by convention, we invert data on the proxy side ; so that 1 connection = effectively 1 mux
                    extractMuxInfosFromIP(buffer, nread, &dport, &sport, &dip, &sip);
                }
                do_debug("TUN2NET %lu: sport = %u, dport = %u, remote = %lu\n", tun2net, sport, dport, dip);
                
                nMux = assignMux(sport, dport, dip, &muxTable, &tableLength);
                do_debug("TUN2NET %lu:Assigned to mux #%d\n", tun2net, nMux);
                
                if(tcpDataLength(buffer + ipLength, nread - ipLength) > 0){
                    do_debug("TUN2NET %lu: Packet has data in it; encode.\n", tun2net);
                    do_debug("TUN2NET %lu: Creating a clear packet out of the buffer :\n", tun2net);
                    tmpClearPacket = bufferToClearPacket(buffer, nread, ipLength);
                    
                    encodedPacketArray = handleInClear(*tmpClearPacket, *(muxTable[nMux].encoderState));
                    do_debug("TUN2NET %lu: %d coded packets has been generated.\n", tun2net, encodedPacketArray->nPackets);
                    for(i=0; i<encodedPacketArray->nPackets; i++){
                        encodedPacketToBuffer(*(encodedPacketArray->packets[i]), buffer, (int*)&nread);
                        dip = htonl(dip);
                        memcpy(tmp, &dip, 4);
                        sport = htons(sport);
                        memcpy(tmp + 4, &sport, 2);
                        dport = htons(dport);
                        memcpy(tmp + 6, &dport, 2);
                        memcpy(tmp + 8, buffer, nread);
                        nwrite = udpSend(sock_fd, tmp, nread + 8, (struct sockaddr*)&remote);
                        do_debug("TUN2NET %lu: Written %d bytes to the tun interface out of a %d bytes coded packet #%d\n", tun2net, nwrite, nread, i);
                    }
                    encodedArrayFree(encodedPacketArray);
                } else {
                    do_debug("TUN2NET %lu: TCP packet has no data in it; send it unMuxed\n", tun2net);
                    memset(tmp, 0x00, 8);
                    memcpy(tmp + 8, buffer, nread);
                    nwrite = udpSend(sock_fd, tmp, nread + 8, (struct sockaddr*)&remote);
                    do_debug("TUN2NET %lu: Written %d bytes to the network\n", tun2net, nwrite);
                }
            } else {
                do_debug("TUN2NET %lu: Packet is not TCP, send it unMuxed\n", tun2net);
                memset(tmp, 0x00, 8);
                memcpy(tmp + 8, buffer, nread);
                nwrite = udpSend(sock_fd, tmp, nread + 8, (struct sockaddr*)&remote);
                do_debug("TUN2NET %lu: Written %d bytes to the network\n", tun2net, nwrite);
            }
        }

        if(FD_ISSET(sock_fd, &rd_set)) {
            /* data from the network: read it, and write it to the tun interface. */
            net2tun++;
            /* read packet */
            nread = cread(sock_fd, buffer, BUFSIZE);
            do_debug("NET2TUN %lu: Read %d bytes from the network\n", net2tun, nread);

            // If the data is muxed :
            if(memcmp(nullMux, buffer, 8) != 0){
                // It is muxed
                do_debug("NET2TUN %lu: Received packet is Muxed\n", net2tun);
                extractMuxInfosFromMuxed(buffer, &sport, &dport, &dip);
                do_debug("NET2TUN %lu: sport = %u, dport = %u, remote = %lu\n", net2tun, sport, dport, dip);
                int nMux = assignMux(sport, dport, dip, &muxTable, &tableLength);
                do_debug("NET2TUN %lu: Assigned to mux #%d\n", net2tun, nMux);
                
                tmpEncodedPacket = bufferToEncodedPacket(buffer + 8, nread - 8);
                
                clearPacketArray = handleInCoded(*tmpEncodedPacket, *(muxTable[nMux].decoderState));
                do_debug("NET2TUN %lu: %d packets has been decoded.\n", net2tun, clearPacketArray->nPackets);
                for(i=0; i<clearPacketArray->nPackets; i++){
                    do_debug("NET2TUN %lu: Before current packet : Last byte acked = %u ; last byte sent = %u\n", net2tun, muxTable[nMux].encoderState->lastByteAcked, muxTable[nMux].decoderState->lastByteSent);
                    
                    do_debug("NET2TUN %lu: packet #%d\n", net2tun, i);
                    clearPacketPrint(*(clearPacketArray->packets[i]));
                    
                    // Assuming no reordering...
                    clearPacketToBuffer(*(clearPacketArray->packets[i]), buffer, (int*)&nread);
                    
                    // Inspect generated packet
                    ipLength = ipHeaderLength(buffer);
                    muxTable[nMux].encoderState->lastByteAcked = max(muxTable[nMux].encoderState->lastByteAcked, getAckNumber(buffer + ipLength));  // Actualize the last byte acked
                    muxTable[nMux].decoderState->lastByteSent = max(muxTable[nMux].decoderState->lastByteSent, getSeqNumber(buffer+ipLength) + tcpDataLength(buffer + ipLength, nread - ipLength) - 1); // Actualize the last byte sent
                    
                    nwrite = cwrite(tun_fd, buffer, nread);
                    do_debug("NET2TUN %lu: Written %d bytes to the tun interface out of a %d bytes clear packet #%d\n", net2tun, nwrite, nread, i);
                    do_debug("NET2TUN %lu: Last byte acked = %u ; last byte sent = %u\n", net2tun, muxTable[nMux].encoderState->lastByteAcked, muxTable[nMux].decoderState->lastByteSent);
                }
                clearArrayFree(clearPacketArray);
            } else { // Not muxed ; just send it after inspection
                // Inspect generated packet
                if(isTCP(buffer + 8, nread - 8)){
                    do_debug("NET2TUN %lu: unMuxed packet is TCP.\n", net2tun);
                    if(cliserv == CLIENT){
                        extractMuxInfosFromIP(buffer + 8, nread - 8, &dport, &sport, &dip, &sip);
                    } else { // Note : by convention, we invert data on the proxy side ; so that 1 connection = effectively 1 mux
                        extractMuxInfosFromIP(buffer + 8, nread - 8, &sport, &dport, &sip, &dip);
                    }
                    do_debug("NET2TUN %lu: sport = %u, dport = %u, remote = %lu\n", net2tun, sport, dport, dip);
                    
                    nMux = assignMux(sport, dport, dip, &muxTable, &tableLength);
                    do_debug("NET2TUN %lu: Assigned to mux #%d\n", net2tun, nMux);
                    
                    do_debug("NET2TUN %lu: Before actualizing : Last byte acked = %u ; last byte sent = %u\n", net2tun, muxTable[nMux].encoderState->lastByteAcked, muxTable[nMux].decoderState->lastByteSent);
                    
                    ipLength = ipHeaderLength(buffer + 8);
                    muxTable[nMux].encoderState->lastByteAcked = max(muxTable[nMux].encoderState->lastByteAcked, getAckNumber(buffer + 8 + ipLength));  // Actualize the last byte acked
                    muxTable[nMux].decoderState->lastByteSent = max(muxTable[nMux].decoderState->lastByteSent, getSeqNumber(buffer + 8 + ipLength) + tcpDataLength(buffer + 8 + ipLength, nread - 8 - ipLength) - 1); // Actualize the last byte sent
                    do_debug("NET2TUN %lu: After actualizing : Last byte acked = %u ; last byte sent = %u\n", net2tun, muxTable[nMux].encoderState->lastByteAcked, muxTable[nMux].decoderState->lastByteSent);
                } else {
                    do_debug("NET2TUN %lu: Is not TCP\n", net2tun);
                }
                
                // Send it to the TUN interface
                nwrite = cwrite(tun_fd, buffer + 8, nread - 8);
                do_debug("NET2TUN %lu: Written %d bytes to the tun interface\n", net2tun, nwrite);
            }
        }
    }
    
    return(0);
}
