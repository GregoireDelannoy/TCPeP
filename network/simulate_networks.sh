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
echo "Done !"

if [ "$1" == "clean" ]; then
    exit
fi

echo "Setting the new parameters..."
# Replace the egress queue by a netem
echo "netem queue on interface"
tc qdisc add dev $INTERFACE root netem

# Apply parameters
tc qdisc change dev $INTERFACE root netem delay $DELAY $DELAY_VARIATION loss $LOSS $LOSS_CORRELATION

# Display queue policies :
tc -s qdisc
