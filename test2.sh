#!/bin/sh

for i in $(seq 0 20); do
#for i in $(seq 0 2); do
    ./xtest2 $i $((5200+$i)) &
done

wait
