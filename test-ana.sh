#!/bin/bash

# sudo ./test-ana.sh 2>&1 | tee all.log

TOTAL=300
WAIT=30
#LATENCY=50
#EVENTS=100

# 21 peers
#  5 hops
# 50 fps
# 10 random seconds initial delay
# no delay in time travel

tc qdisc del dev lo root

for LATENCY in 5 25 50 100 250 500 1000
do
    tc qdisc add dev lo root netem delay ${LATENCY}ms $(($LATENCY/5))ms distribution normal
    for EVENTS in 1 5 10 25 50 100 250 500
    do

        echo "=== TOTAL=$TOTAL, WAIT=$WAIT, LATENCY=$LATENCY, EVENTS=$EVENTS"
        echo ""

        gcc -g -Wall `sdl2-config --cflags` -DP2P_LATENCY=$LATENCY -DP2P_TOTAL=$TOTAL -DP2P_WAIT=$WAIT -DP2P_EVENTS=$EVENTS p2p.c test.c -o xtest `sdl2-config --libs` -lpthread -lSDL2_net -lSDL2_image

        pkill -9 -f xtest
        time ./test.sh 2>&1 > out.log

        lua5.3 test-ana.lua $TOTAL $WAIT $LATENCY
        echo ""
    done
    tc qdisc del dev lo root
done
