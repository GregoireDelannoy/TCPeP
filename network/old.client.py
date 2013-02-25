#!/usr/bin/env python
# -*- coding: utf-8 -*-

# Wait for connections on a TCP socket, pass them along on top of UDP to the server

import socket, binascii, struct, ctypes
from scapy.all import *
import os
from fcntl import ioctl
from select import select
import struct
import subprocess


CLIENT_PORT = 1337
PROXY_PORT = 31337
PROXY_ADDR = "127.0.0.1"

# Create UDP socket for communicating with the proxy
proxy_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

## Scapy way
#def incoming_handler(packet):
    #print 'Packet received in handler'
    
    #packet.show()
    
    #print 'handling end'

#a=sniff(filter="dst 78.225.128.2", prn=lambda x: incoming_handler(x))

## Raw socket way
#expression = [
## (000) ldh      [12]
#( 0x28, 0, 0, 0x0000000c ),
## (001) jeq      #0x86dd          jt 2	jf 6
#( 0x15, 0, 4, 0x000086dd ),
## (002) ldb      [20]
#( 0x30, 0, 0, 0x00000014 ),
## (003) jeq      #0x6             jt 4	jf 15
#( 0x15, 0, 11, 0x00000006 ),
## (004) ldh      [56]
#( 0x28, 0, 0, 0x00000038 ),
## (005) jeq      #0x539           jt 14	jf 15
#( 0x15, 8, 9, 0x00000539 ),
## (006) jeq      #0x800           jt 7	jf 15
#( 0x15, 0, 8, 0x00000800 ),
## (007) ldb      [23]
#( 0x30, 0, 0, 0x00000017 ),
## (008) jeq      #0x6             jt 9	jf 15
#( 0x15, 0, 6, 0x00000006 ),
## (009) ldh      [20]
#( 0x28, 0, 0, 0x00000014 ),
## (010) jset     #0x1fff          jt 15	jf 11
#( 0x45, 4, 0, 0x00001fff ),
## (011) ldxb     4*([14]&0xf)
#( 0xb1, 0, 0, 0x0000000e ),
## (012) ldh      [x + 16]
#( 0x48, 0, 0, 0x00000010 ),
## (013) jeq      #0x539           jt 14	jf 15
#( 0x15, 0, 1, 0x00000539 ),
## (014) ret      #65535
#( 0x6, 0, 0, 0x0000ffff ),
## (015) ret      #0
#( 0x6, 0, 0, 0x00000000 )]

#blob = ctypes.create_string_buffer(''.join(struct.pack("HBBI", *e) for e in expression))
#address = ctypes.addressof(blob)
#to_pass = struct.pack('HL', len(expression), address)


## As defined in asm/socket.h  
#SO_ATTACH_FILTER = 26  
  
## Create listening socket with filters  
#s = socket.socket(socket.AF_PACKET, socket.SOCK_RAW, 0x0800)  
#s.setsockopt(socket.SOL_SOCKET, SO_ATTACH_FILTER, to_pass)
#s.bind(('lo', 0x0800))  


#def original_dst(sock):
    #try:
        #SO_ORIGINAL_DST = 80
        #SOCKADDR_MIN = 16
        #sockaddr_in = sock.getsockopt(socket.SOL_IP,
                                      #SO_ORIGINAL_DST, SOCKADDR_MIN)
        #(proto, port, a,b,c,d) = struct.unpack('!HHBBBB', sockaddr_in[:8])
        #assert(socket.htons(proto) == socket.AF_INET)
        #ip = '%d.%d.%d.%d' % (a,b,c,d)
        #return (ip,port)
    #except socket.error, e:
        #if e.args[0] == errno.ENOPROTOOPT:
            #return sock.getsockname()
        #raise

#while True:  
    #data, addr = s.recvfrom(65565)  
    #print 'got data from', addr, ':', binascii.hexlify(data)
    #print 'Original dst : ', original_dst(s)
    
    ## Interpret it in Scapy
    #pkt = Ether(data)
    #pkt.show()
    
    
# TUN device
TUNSETIFF = 0x400454ca
TUNMODE = 0x0001

tunFile = os.open("/dev/net/tun", os.O_RDWR)
ifs = ioctl(tunFile, TUNSETIFF, struct.pack("16sH", "tun%d", TUNMODE))
ifname = ifs[:16].strip("\x00")
print "Allocated interface %s. Configure it and use it" % ifname
subprocess.call("ifconfig %s 192.168.13.1" % ifname,shell=True)
# Reading
def read():
    print 'start select()'
    r = select([tunFile], [], [])[0][0]
    print 'select() released'
    if r == tunFile:
        print 'try to read'
        return os.read(tunFile, 1600)
    return None

# Writing
def write(buf):
    os.write(tunFile, buf)
    
while True:
    pkt = read()
    if pkt != None:
        pkt = pkt[4:] # Note : kernel add 4 bytes to tun-ed packets ; we remove them
        
        # Print packet
        print 'Scapy decoding : '
        IP(pkt).show() 
        
        # Pass packet to the proxy via UDP
        proxy_socket.sendto(pkt, (PROXY_ADDR, PROXY_PORT))

