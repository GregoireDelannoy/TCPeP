#include "tun.h"


// Set the MTU of a TUN link
int set_mtu(struct ifreq *ifr_tun, int sock, unsigned int mtu)
{
    /* Set the MTU of the tap interface */
    ifr_tun->ifr_mtu = mtu; 
    if (ioctl(sock, SIOCSIFMTU, ifr_tun) < 0)  {
        perror("set_mtu : ");
        return -1;
    }

    printf("MTU was set to %d\n", mtu);
    return 0;
}

// allocates or reconnects to a tun device.
int tun_alloc(char *dev, int flags, int mtu) {
    struct ifreq ifr;
    int fd, err;
    char *clonedev = "/dev/net/tun";

    if( (fd = open(clonedev , O_RDWR)) < 0 ) {
        perror("Opening /dev/net/tun");
        return fd;
    }

    memset(&ifr, 0, sizeof(ifr));

    ifr.ifr_flags = flags;

    if (*dev) {
        strncpy(ifr.ifr_name, dev, IFNAMSIZ);
    }

    if( (err = ioctl(fd, TUNSETIFF, (void *)&ifr)) < 0 ) {
        perror("ioctl(TUNSETIFF)");
        close(fd);
        return err;
    }
    
    // Set MTU
    int sock = socket(AF_INET, SOCK_DGRAM, 0); // Don't really know what it's used for, but needed in the ioctl...
    set_mtu(&ifr, sock, mtu);
    close(sock);

    strcpy(dev, ifr.ifr_name);

    return fd;
}
