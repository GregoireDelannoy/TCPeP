#!/usr/bin/env python
# -*- coding: utf-8 -*-

# Wait for incoming packets on a TUN interface, pass them along on top of UDP to the server

import socket
from scapy.all import *
import os
from fcntl import ioctl
from select import select
import struct
import subprocess


CLIENT_PORT = 1337
PROXY = ("192.168.56.1", 31337)

# Create UDP socket for communicating with the proxy
proxy_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
proxy_socket.connect(PROXY)


# TUN device
TUNSETIFF = 0x400454ca 
TUNMODE = 0x0001 # Mode = TUN  = do not pass ethernet but only IP
IFF_NO_PI = 0x1000 # Pass pure IP frames (no additional 4 bytes


tun_file = os.open("/dev/net/tun", os.O_RDWR)
ifs = ioctl(tun_file, TUNSETIFF, struct.pack("16sH", "tun%d", TUNMODE | IFF_NO_PI))
ifname = ifs[:16].strip("\x00")
print "Allocated interface %s. Configure it and use it" % ifname

while True:
    # Main loop, we wait for packets on either proxy_socket or the tun interface
    ans = select([tun_file, proxy_socket],[],[])[0][0]
    
    if(ans == tun_file):
        #print 'Received data from the tun interface'
        pkt = os.read(tun_file, 1600)
        
        # Print packet
        #print 'Scapy decoding : '
        #IP(pkt).show() 
        
        # Send it to the server
        proxy_socket.send(pkt)
    elif(ans == proxy_socket):
        #print 'Received data from the proxy'
        
        # Receive packet
        pkt= proxy_socket.recv(1600)
        
        ## Print packet
        #print 'Scapy decoding : '
        #IP(pkt).show() 
        
        os.write(tun_file, pkt)
    else:
        print 'Error from select. Received : ', ans

