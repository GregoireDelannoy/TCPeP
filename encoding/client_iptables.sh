#!/bin/sh

## Add/Create chain
#sudo iptables -t nat -N tests-redirect

## Flush chain, just in case
#sudo iptables -t nat -F tests-redirect

## Make chain active in OUTPUT (for reqs originating from this client) and PREROUTING (for routed reqs)
#sudo iptables -t nat -I OUTPUT 1 -j tests-redirect
#sudo iptables -t nat -I PREROUTING 1 -j tests-redirect

## Redirect TCP destinated to IP to local port
#sudo iptables -t nat -A tests-redirect -j REDIRECT --dest 78.225.128.2 -p tcp --to-ports 1337 -m ttl ! --ttl 42

## Drop TCP to destinations
##sudo iptables -t nat -A tests-redirect -j DROP --dest 78.225.128.2 -p tcp

ifconfig tun0 192.168.13.2 pointopoint 192.168.13.1

ip route flush table 200
ip route add table 200 via 192.168.13.1

iptables -t mangle -F
iptables -t mangle -A OUTPUT -p tcp -d 78.225.128.2  -m ttl ! --ttl 42 -j MARK --set-mark 3

ip rule add fwmark 3 table 200
