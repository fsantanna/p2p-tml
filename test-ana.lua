-- cat out.log | lua5.3 test-ana.lua

local _PEERS_   = 21
local _LATENCY_ = 100
local _FPS_     = 50
local _TOTAL_   = _FPS_*60*5 -- 5min
local _START_   = _FPS_*60*2 -- 1min
local _COUNT_   = 5

local ok,no,tot = 1,1,0
local TICK = _START_
local GOFWDS = 0
local GOBAKS = 0
local T = {}

for l in io.lines(...) do
    --local peer,tick = string.find(l,'TICK')
    local peer,tick = string.match(l,'%[(%d+)%] TICK=(%d+)')
    local _,gofwd = string.match(l,'%[(%d+)%] GOFWD=(%d+)')
    local _,gobak = string.match(l,'%[(%d+)%] GOBAK=(%d+)')
    local n,state = string.match(l,'%[(%d+)%] TICK='.._TOTAL_..' pos=(.*)')
    if n then
        T[tonumber(n)] = state
    end
    if peer then
        peer = tonumber(peer)
        tick = tonumber(tick)
        --print(peer, tick)
        tot = tot + 1
        if tick <= _START_ then
            tot = tot - 1
            -- wait
        elseif tick > TICK then
            TICK = tick
            --print(peer, tick)
        elseif tick <= TICK+(_FPS_*_LATENCY_/1000) then
            ok = ok + 1
        else
--print(peer,tick,TICK)
            no = no + 1
            --no = no + (TICK-tick)/5
        end
    elseif gofwd then
        if TICK > _START_ then
            gofwd = tonumber(gofwd)
            GOFWDS = GOFWDS + 1 --gofwd
        end
    elseif gobak then
        if TICK > _START_ then
            gobak = tonumber(gobak)
            GOBAKS = GOBAKS + gobak
        end
    end
end

local state = T[0]
for i=1, 20 do
    assert(T[i] == state)
end

print(state)
print('TOT', tot, 'EXP', (_TOTAL_-_START_)*_PEERS_/_COUNT_)
--print('NO',no, 'OK',ok, 'TOT',no+ok)
print('NORLTS', no,     (no/(no+ok)))
print('GOFWDS', GOFWDS, (GOFWDS/tot*100))
print('GOBAKS', GOBAKS, (GOBAKS/tot*100))
