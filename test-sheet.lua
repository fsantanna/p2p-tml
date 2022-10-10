-- lua5.3 test-sheet.lua all-01.log

local F   = ...
local LAT = nil
local EVT = nil
local T = {}

function P ()
    --print(LAT, EVT, T.evts, T.fwds, T.baks)
    print(string.format("%4d  %3d  %4d  %03.03f  %03.03f  %03.03f  %s", LAT, EVT, T.evts, T.fwds, T.baks1, T.baks2, F))
end

for l in io.lines(F) do
    local lat,evt = string.match(l,'=== TOTAL=300, WAIT=30, LATENCY=(%d+), EVENTS=(%d+)')
    if lat and evt then
        if LAT and EVT then
            P()
        end
        LAT = lat
        EVT = evt
    end

    local fwds = string.match(l,'FWDS.*(%d+%.%d+)$')
    if fwds then
        T.fwds = fwds
    end

    local baks1,baks2 = string.match(l,'BAKS.*(%d+%.%d+).*(%d+%.%d+)$')
    if baks1 and baks2 then
        T.baks1 = baks1
        T.baks2 = baks2
    else
        local baks1 = string.match(l,'BAKS.*(%d+%.%d+)$')
        if baks1 then
            T.baks1 = baks1
            T.baks2 = 9.99
        end
    end

    local evts = string.match(l,'EVTS%s+(%d+)')
    if evts then
        T.evts = evts
    end
end

P()
