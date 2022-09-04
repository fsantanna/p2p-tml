#!/bin/sh

grep EVT out.log | wc
grep FWD out.log | wc
cat out.log | lua5.3 test-ana.lua
