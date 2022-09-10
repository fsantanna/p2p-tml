#!/bin/sh

grep TICK out.log | wc
grep EVT out.log | wc
grep GOBAK out.log | wc
cat out.log | lua5.3 test-ana.lua
