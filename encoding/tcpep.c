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

/* buffer for reading from tun/tap interface, must be >= 1500 */
#define BUFSIZE 2000     
#define CLIENT 0
#define SERVER 1
#define PORT 55555
#define MTU 1400

int debug;
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
    fprintf(stderr, "-d: outputs debug information while running\n");
    fprintf(stderr, "-h: prints this help text\n");
    exit(1);
}

int main(int argc, char *argv[]) {
    int tun_fd, option, i;
    char if_name[IFNAMSIZ] = "tun0";
    int maxfd;
    uint16_t nread, nwrite;
    char buffer[BUFSIZE], tmp[BUFSIZE];
    struct sockaddr_in local, remote;
    char remote_ip[16] = "";     /* dotted quad IP string */
    unsigned short int port = PORT;
    int sock_fd, optval = 1;
    int cliserv = -1;        /* must be specified on cmd line */
    unsigned long int tap2net = 0, net2tap = 0;
    uint16_t sport;
    uint16_t dport;
    uint32_t dip;
    clearpacketarray* clearPacketArray;
    encodedpacketarray* encodedPacketArray;
    
    char nullMux[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    
    muxstate* muxTable = 0;
    int tableLength = 0;

    progname = argv[0];
    
    /* Check command line options */
    while((option = getopt(argc, argv, "i:sc:p:uahd")) > 0) {
        switch(option) {
            case 'd':
                debug = 1;
                break;
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
        
        udpSend(sock_fd, "HELO", 5, (struct sockaddr *)&remote);

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
        if(0 != memcmp(buffer, "HELO", 4)){
            printf("Received an error HELO message.\n");
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
        do_debug("Entering select\n");
        ret = select(maxfd + 1, &rd_set, NULL, NULL, NULL);

        if (ret < 0 && errno == EINTR){
            continue;
        }

        if (ret < 0) {
            perror("select()");
            exit(1);
        }

        if(FD_ISSET(tun_fd, &rd_set)) {
            /* data from tun: just read it and write it to the network */
            
            nread = cread(tun_fd, buffer, BUFSIZE);

            tap2net++;
            do_debug("TAP2NET %lu: Read %d bytes from the tun interface :\n", tap2net, nread);
            
            if(isTCP(buffer, nread)){
                do_debug("Received packet is TCP\n");
                extractMuxInfosFromIP(buffer, nread, &sport, &dport, &dip);
                do_debug("sport = %d, dport = %d, remote = %d\n", sport, dport, dip);
                sport = htons(sport);
                dport = htons(dport);
                dip = htonl(dip);
                
                int nMux = assignMux(sport, dport, dip, &muxTable, &tableLength);
                do_debug("Assigned to mux #%d\n", nMux);
                
                encodedPacketArray = handleInClear(bufferToClearPacket(buffer, nread), *(muxTable[nMux].encoderState));
                printf("%d coded packets has been generated.\n", encodedPacketArray->nPackets);
                for(i=0; i<encodedPacketArray->nPackets; i++){
                    encodedPacketPrint((*encodedPacketArray->packets[i]));
                    encodedPacketToBuffer((*encodedPacketArray->packets[i]), buffer, (int*)&nread);
                    memcpy(tmp, &dip, 4);
                    memcpy(tmp + 4, &sport, 2);
                    memcpy(tmp + 6, &dport, 2);
                    memcpy(tmp + 8, buffer, nread);
                    nwrite = udpSend(sock_fd, tmp, nread + 8, (struct sockaddr*)&remote);
                    do_debug("NET2TAP %lu: Written %d bytes to the tun interface out of a %d bytes coded packet #%d\n", net2tap, nwrite, nread, i);
                }
                encodedArrayFree(encodedPacketArray);
            } else {
                memset(tmp, 0x00, 8);
                memcpy(tmp + 8, buffer, nread);
                nwrite = udpSend(sock_fd, tmp, nread + 8, (struct sockaddr*)&remote);
                do_debug("TAP2NET %lu: Written %d bytes to the network\n", tap2net, nwrite);
            }
        }

        if(FD_ISSET(sock_fd, &rd_set)) {
            /* data from the network: read it, and write it to the tun/tap interface. */
            net2tap++;
            /* read packet */
            nread = cread(sock_fd, buffer, BUFSIZE);
            do_debug("NET2TAP %lu: Read %d bytes from the network\n", net2tap, nread);

            // If the data is muxed :
            if(memcmp(nullMux, buffer, 8) != 0){
                // It is muxed
                do_debug("Received packet is Muxed\n");
                extractMuxInfosFromMuxed(buffer, &sport, &dport, &dip);
                do_debug("sport = %d, dport = %d, remote = %d\n", sport, dport, dip);
                int nMux = assignMux(sport, dport, dip, &muxTable, &tableLength);
                do_debug("Assigned to mux #%d\n", nMux);
                
                encodedPacketPrint(bufferToEncodedPacket(buffer + 8, nread - 8));
                
                clearPacketArray = handleInCoded(bufferToEncodedPacket(buffer + 8, nread - 8), *(muxTable[nMux].decoderState));
                printf("%d packets has been decoded.\n", clearPacketArray->nPackets);
                for(i=0; i<clearPacketArray->nPackets; i++){
                    clearPacketToBuffer(*(clearPacketArray->packets[i]), buffer, (int*)&nread);
                    nwrite = cwrite(tun_fd, buffer, nread);
                    do_debug("NET2TAP %lu: Written %d bytes to the tun interface out of a %d bytes clear packet #%d\n", net2tap, nwrite, nread, i);
                }
                clearArrayFree(clearPacketArray);
            } else { // Just send it
                nwrite = cwrite(tun_fd, buffer + 8, nread - 8);
                do_debug("NET2TAP %lu: Written %d bytes to the tun interface\n", net2tap, nwrite);
            }
        }
    }
    
    return(0);
}
