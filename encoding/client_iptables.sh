#!/bin/sh

iptables -t nat -A OUTPUT -j REDIRECT --dest "10.0.0.1" -p tcp --to-ports 1337
ip route add 10.0.0.1 via 192.168.56.1

tc qdisc replace dev eth0 root netem delay 20ms loss 5%
