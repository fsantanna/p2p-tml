#!/bin/sh

for i in $(seq 0 20); do
#for i in $(seq 0 2); do
    ./xmove2 $i $((5000+$i)) &
done

wait
