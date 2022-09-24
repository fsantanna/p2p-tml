#!/bin/bash

# ./test2-ana.sh

TOTAL=300
WAIT=30
#LATENCY=50
#EVENTS=100

# 21 peers
#  5 hops
# 50 fps
# 10 random seconds initial delay
# no delay in time travel

for LATENCY in 5 25 50 100 250 500 1000
do
    for EVENTS in 1 5 10 25 50 100 250 500
    do
        if [ "$LATENCY" == "100" ] && [ "$EVENTS" == "1000" ]
        then
            break
        fi
        if [ "$LATENCY" == "250" ] && [ "$EVENTS" == "250" ]
        then
            break
        fi
        if [ "$LATENCY" == "500" ] && [ "$EVENTS" == "250" ]
        then
            break
        fi
        if [ "$LATENCY" == "1000" ] && [ "$EVENTS" == "100" ]
        then
            break
        fi

        echo "=== TOTAL=$TOTAL, WAIT=$WAIT, LATENCY=$LATENCY, EVENTS=$EVENTS" | tee -a all.log
        echo "" | tee -a all2.log

        gcc -g -Wall `sdl2-config --cflags` -DP2P_LATENCY=$LATENCY -DP2P_TOTAL=$TOTAL -DP2P_WAIT=$WAIT -DP2P_EVENTS=$EVENTS p2p.c test2.c -o xtest2 `sdl2-config --libs` -lpthread -lSDL2_net -lSDL2_image

        pkill -9 -f xtest2
        time ./test2.sh | tee out2.log

        lua5.3 test2-ana.lua $TOTAL $WAIT $LATENCY | tee -a all2.log
        echo "" | tee -a all2.log
    done
done
