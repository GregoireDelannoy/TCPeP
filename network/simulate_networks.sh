#!/bin/bash

# ~~ Parameters ~~
INTERFACE="eth0"

LOSS="1%"
LOSS_CORRELATION="25%"

DELAY="150ms"
DELAY_VARIATION="10ms"

RATE="512kbit"


# ~~ Code ~~
echo "Cleaning the system..."
# Remove existing qdisc policies and IFB
tc qdisc del dev $INTERFACE root netem
tc qdisc del dev $INTERFACE ingress
tc qdisc del dev ifb0 root netem
ip link set dev ifb0 down
rmmod ifb
echo "Done !"

if [ "$1" == "clean" ]; then
    exit
fi

echo "Setting the new parameters..."
modprobe ifb
# Replace the egress queue by a netem
tc qdisc add dev $INTERFACE root handle 1:0 netem
# Set-up the IDB
ip link set dev ifb0 up
tc qdisc add dev $INTERFACE ingress
tc filter add dev eth0 parent ffff: protocol ip u32 match u32 0 0 flowid 1:1 action mirred egress redirect dev ifb0
tc qdisc add dev ifb0 root handle 1:0 netem

# Apply parameters
tc qdisc change dev $INTERFACE root netem delay $DELAY $DELAY_VARIATION loss $LOSS $LOSS_CORRELATION
tc qdisc change dev ifb0 root netem delay $DELAY $DELAY_VARIATION loss $LOSS $LOSS_CORRELATION

tc qdisc add dev $INTERFACE parent 1:1 handle 10: tbf rate $RATE buffer 1600 limit 3000
tc qdisc add dev ifb0 parent 1:1 handle 10: tbf rate $RATE buffer 1600 limit 3000

# Display queue policies :
tc -s qdisc
