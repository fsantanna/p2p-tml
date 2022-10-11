#!/bin/sh

for LAT in '   5' '  25' '  50' ' 100' ' 250' ' 500' '1000'
do
    for EVT in '  1' '  5' ' 10' ' 25' ' 50' '100' '250' '500'
    do
        for MUL in 1 2 5
        do
            echo "=== $LAT  $EVT  $MUL"
            cat sheet.log | grep "^$LAT  $EVT  $MUL"
        done
    done
done
