#!/usr/bin/env python
# -*- coding: utf-8 -*-

import socket, binascii
from scapy.all import *

PROXY_PORT = 31337

# Receive data via UDP from the client ; relay it to the original host via TCP

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

proxy_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
proxy_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, True)
proxy_socket.bind(("0.0.0.0", PROXY_PORT))

while True:
    r = select([tunFile], [], [])[0][0]
    data, addr = proxy_socket.recvfrom(1600)  
    print 'got data from', addr
    print 'Scapy decoding : '
    IP(data).show()

