#!/bin/sh

ifconfig eth0 10.0.0.1

tc qdisc replace dev vboxnet0 root netem loss 5% delay 20ms
