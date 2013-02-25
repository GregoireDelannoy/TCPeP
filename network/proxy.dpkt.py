#!/usr/bin/env python
# -*- coding: utf-8 -*-

import socket, binascii, random
import os
from fcntl import ioctl
from select import select
import struct
import dpkt

PROXY_PORT = 31337
LOSS = 0 # Loss per thousand packets ( created ; both ways!)

# Receive data via UDP from the client ; relay it to the original host via TCP

proxy_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
proxy_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, True)
proxy_socket.bind(("0.0.0.0", PROXY_PORT))

# TUN device
TUNSETIFF = 0x400454ca
TUNMODE = 0x0001
IFF_NO_PI = 0x1000

tun_file = os.open("/dev/net/tun", os.O_RDWR)
ifs = ioctl(tun_file, TUNSETIFF, struct.pack("16sH", "tun%d", TUNMODE | IFF_NO_PI))
ifname = ifs[:16].strip("\x00")
print "Allocated interface %s. Configure it and use it" % ifname

client = None

while True:
    if(random.randint(1,1000) < LOSS):
        is_lost = True
    else:
        is_lost = False
    # Main loop, we wait for packets on either proxy_socket or the tun interface
    ans = select([tun_file, proxy_socket],[],[])[0][0]
    
    if(ans == tun_file): # Received data from the tun interface
        pkt = os.read(tun_file, 1600)
        
        if client == None:
            print 'Client is not known ; unable to transfer data'
            continue
        
        # Decode packet using Scapy
        new_packet = dpkt.ip.IP(pkt)
        print 'type(pkt)', type(new_packet)
        print new_packet
        
        if(new_packet.p == 6): # We got a TCP packet ; time to play !
            # Introduce loss
            if(is_lost):
                continue
        
            #new_packet[IP].ttl = 42 # Not to get intercepted by fw
            
            ## Switch cloudapp and home
            #if(new_packet[IP].src == "168.63.173.185"):
                #new_packet[IP].src = "78.225.128.2"
            #elif(new_packet[IP].src == "78.225.128.2"):
                #new_packet[IP].src = "168.63.173.185"
            
            #new_packet[IP].chksum = None
            #new_packet[TCP].chksum = None
            ##new_packet.show2() # Will cause the checksum to be recalculated
            #proxy_socket.sendto(str(new_packet), client)
        #else: # Just send it...
        proxy_socket.sendto(pkt, client)
            
    elif(ans == proxy_socket): # Received data from the client
        # Receive packet
        pkt, peer = proxy_socket.recvfrom(1600)
        
        if client == None: # If the client is not yet known, identify it
            print 'Client will be : %s:%i' % peer
            client = peer
            print client
        elif client != peer:
            print 'Receiving data from unknown peer : %s:%i' % peer
            continue
        
        ## Decode packet using Scapy
        #new_packet = IP(pkt)
        
        #if(new_packet[IP].proto == 6): # We got a TCP packet ; time to play !
            ## Introduce loss
            #if(is_lost):
                #continue
        
            ## Modify received packet :
            #new_packet[IP].ttl = 42 # Not to get intercepted by fw
            
            ## Switch cloudapp and home
            #if(new_packet[IP].dst == "168.63.173.185"):
                #new_packet[IP].dst = "78.225.128.2"
            #elif(new_packet[IP].dst == "78.225.128.2"):
                #new_packet[IP].dst = "168.63.173.185"
            
            #new_packet[IP].chksum = None
            #new_packet[TCP].chksum = None
            ##new_packet.show2() # Will cause the checksum to be recalculated
            #os.write(tun_file, str(new_packet))
        #else:
            # Write to the tun interface
        os.write(tun_file, pkt)
    else:
        print 'Error from select. Received : ', ans

