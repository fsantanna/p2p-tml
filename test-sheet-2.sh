#!/bin/sh

for LAT in 5 25 50 100 250 500
do
    for MUL in 5 2 1
    do
        for EVT in 5 10 25 50 100 200
        do
            echo "===  $LAT  $MUL  $EVT"
            cat sheet.log | grep "all-6[0-9].log" | grep " \+$LAT \+$MUL \+$EVT \+"
        done
    done
done

#lua5.3 test-sheet-2.lua sheet-2.log
