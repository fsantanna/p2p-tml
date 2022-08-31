#!/bin/sh

for i in $(seq 0 49); do
    ./xstrip $i $((5000+$i)) &
done

wait
