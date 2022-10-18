#!/bin/sh

# time ./test2-ana.sh 2>&1 | tee out2.log

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
                -DP2P_TOTAL=120 -DP2P_WAIT=30 -DP2P_EVENTS=$EVTS \
                -DP2P_LATENCY=$LAT -DP2P_AVG_HOPS=5 -DP2P_DELTA_MULT=$MULT \
                p2p.c test.c -o xtest2 \
                `sdl2-config --libs` -lpthread -lSDL2_net -lSDL2_image

            pkill -9 -f xtest2
            time ./test2.sh 2>&1 > out2.log
            lua5.3 test2-ana.lua 120 $LAT
        done
    done
done
tc qdisc del dev lo root

now=`date "+%s"`
echo "FINISH [$now]"

