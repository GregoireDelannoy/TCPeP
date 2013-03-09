#!/bin/sh

ifconfig tun0 192.168.13.1 pointopoint 192.168.13.2
iptables -t nat -F
iptables -t nat -A POSTROUTING -o wlan0 -j MASQUERADE
echo 1 > /proc/sys/net/ipv4/ip_forward
