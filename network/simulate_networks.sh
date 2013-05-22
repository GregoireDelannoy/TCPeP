#!/bin/bash

# ~~ Parameters ~~
INTERFACE="wlan0"

LOSS="10%"
LOSS_CORRELATION="25%"

DELAY="1ms"
DELAY_VARIATION="1ms"


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
echo "netem queue on interface"
tc qdisc add dev $INTERFACE root handle 1:0 netem
# Set-up the IDB
echo "ifb0 up"
ip link set dev ifb0 up
echo "add ingress to interface"
tc qdisc add dev $INTERFACE ingress
tc filter add dev $INTERFACE parent ffff: protocol ip u32 match u32 0 0 flowid 1:1 action mirred egress redirect dev ifb0
tc qdisc add dev ifb0 root handle 1:0 netem

# Apply parameters
tc qdisc change dev $INTERFACE root netem delay $DELAY $DELAY_VARIATION loss $LOSS $LOSS_CORRELATION
tc qdisc change dev ifb0 root netem delay $DELAY $DELAY_VARIATION loss $LOSS $LOSS_CORRELATION

# Display queue policies :
tc -s qdisc
