#!/bin/sh

for f in all-*.log; do
    echo $f
    lua5.3 test-sheet.lua $f
done
