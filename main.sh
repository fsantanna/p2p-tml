#!/bin/sh

./xmain 0 5010 &
./xmain 1 5011 &
./xmain 2 5012 &
./xmain 3 5013 &
./xmain 4 5014 &

wait
