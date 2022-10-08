#!/bin/sh

# time ./test2.sh 2>&1 | tee out2.log

gcc -g -Wall `sdl2-config --cflags` -DP2P_LATENCY=50 -DP2P_TOTAL=120 -DP2P_WAIT=30 -DP2P_EVENTS=50 p2p.c test.c -o xtest `sdl2-config --libs` -lpthread -lSDL2_net -lSDL2_image

rm files/file-*.txt

now=`date "+%s"`
echo "STARTED [$now]"

tc qdisc add dev lo root netem delay 50ms 10ms distribution normal

for i in $(seq 0 20); do
#for i in $(seq 0 2); do
    ./x.sh $i &
done

for i in $(seq 1 130); do
    sleep 1
    echo "=== $i ==="
done

wait
tc qdisc del dev lo root

now=`date "+%s"`
echo "FINISH [$now]"

