#!/bin/sh

rm files/file-*.txt

for i in $(seq 0 20); do
#for i in $(seq 0 2); do
    ./xtest $i $((5100+$i)) &
done

wait
