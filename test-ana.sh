#!/bin/bash

# sudo ./test-ana.sh 2>&1 | tee all.log

TOTAL=300
WAIT=30
#LAT=50
#EVENTS=100

# 21 peers
#  5 hops
# 50 fps
# 10 random seconds initial delay
# no delay in time travel

tc qdisc del dev lo root

for LAT in 5 25 50 100 250 500
do
    tc qdisc add dev lo root netem delay ${LAT}ms $(($LAT/5))ms distribution normal
    for MULT in 5 2 1
    do
        for EVTS in 5 10 25 50 100 200
        do
            echo "=== LAT=$LAT, MULT=$MULT, EVTS=$EVTS"
            echo ""

            gcc -g -Wall `sdl2-config --cflags` \
                -DP2P_TOTAL=$TOTAL -DP2P_WAIT=$WAIT -DP2P_LATENCY=$LAT \
                -DP2P_EVENTS=$EVTS -DP2P_AVG_HOPS=5 -DP2P_DELTA_MULT=$MULT \
                p2p.c test.c -o xtest \
                `sdl2-config --libs` -lpthread -lSDL2_net -lSDL2_image

            pkill -9 -f xtest
            time ./test.sh 2>&1 > out.log

            lua5.3 test-ana.lua $TOTAL $WAIT $LAT
            echo ""
        done
    done
    tc qdisc del dev lo root
done
