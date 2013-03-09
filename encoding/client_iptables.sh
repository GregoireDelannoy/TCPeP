#!/bin/sh

ifconfig tun0 192.168.13.2 pointopoint 192.168.13.1

ip route add default via 192.168.13.1
