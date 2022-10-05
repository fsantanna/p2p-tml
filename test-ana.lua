-- lua5.3 test-ana.lua 300 30 50

local total,wait,_LATENCY_ = ...

local _PEERS_ = 21
local _FPS_   = 50

local _TOTAL_ = _FPS_*total
local _WAIT_  = _FPS_*wait

local ok,no,tot = 0,0,0
local TICK = _WAIT_

local NOS = {}
local POS = {}
local EVTS = 0
local TCKS = {}
local FWDS1, FWDS2 = 0, 0
local BAKS1, BAKS2 = 0, 0

for l in io.lines('out.log') do
    local n1,tick,pos = string.match(l,'%[(%d+)%] tick (%d+) pos=(.*)')
    local n2,evts = string.match(l,'%[(%d+)%] evts (%d+)')
    local n3,tcks = string.match(l,'%[(%d+)%] tcks (%d+)')
    local n4,fwds1,fwds2 = string.match(l,'%[(%d+)%] fwds (%d+) (%d+)')
    local n5,baks1,baks2 = string.match(l,'%[(%d+)%] baks (%d+) (%d+)')

    if n1 then
        tick = tonumber(tick)
        if tick == _TOTAL_ then
            POS[tonumber(n1)] = pos
        end
        tot = tot + 1
        if tick <= _WAIT_ then
            tot = tot - 1
            -- wait
        elseif tick > TICK then
            TICK = tick
            --print(peer, tick)
        elseif tick < TICK-50 then
        --elseif tick < TICK-(_LATENCY_*_FPS_/1000) then
            --print('NO', n1, tick, TICK)
            no = no + 1
            local i = tick // 1000
            NOS[i] = (NOS[i] or 0) + 1
            --no = no + (TICK-tick)/5
        else
            ok = ok + 1
        end
    elseif n2 then
        EVTS = EVTS + tonumber(evts)
    elseif n3 then
        TCKS[tonumber(n3)] = tonumber(tcks)
    elseif n4 then
        FWDS1 = FWDS1 + tonumber(fwds1)
        FWDS2 = FWDS2 + tonumber(fwds2)
    elseif n5 then
        BAKS1 = BAKS1 + tonumber(baks1)
        BAKS2 = BAKS2 + tonumber(baks2)
    end
end

local pos = POS[0]
assert(pos)
for i=1, _PEERS_-1 do
    --assert(POS[i] == pos)
    if POS[i] ~= pos then
        print('ERR', i, pos, POS[i])
    end
end

local tcks = TCKS[0] * _PEERS_
for i=0, _PEERS_-1 do
    assert(TCKS[i] == _TOTAL_-_WAIT_+1)
end

for i=0, 15 do
    print('NOS['..i..'] = '..(NOS[i] or 0))
end
print()

print(string.format('TCKS    %d', tcks))
print(string.format('EVTS    %d', EVTS))
print(string.format('NO      %02.3f', (no/(no+ok))))
print(string.format('FWDS    %02.3f    %02.3f', (FWDS2/tcks), (FWDS1/tcks)))
print(string.format('BAKS    %02.3f    %02.3f', (BAKS2/tcks), (BAKS1/tcks)))
