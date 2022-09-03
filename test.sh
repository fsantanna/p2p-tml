#!/bin/sh

for i in $(seq 0 49); do
#for i in $(seq 0 2); do
    ./xtest $i $((5000+$i)) &
done

wait
