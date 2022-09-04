-- cat out.log | lua5.3 test-ana.lua

local ok,no,tot = 1,1,0
local TICK = 4000   -- 1 minuto
local BAKS = 0

for l in io.lines(...) do
    --local peer,tick = string.find(l,'TICK')
    local peer,tick = string.match(l,'%[(%d+)%] TICK=(%d+)')
    local _,bak = string.match(l,'%[(%d+)%] BAK=(%d+)')
    if peer then
        peer = tonumber(peer)
        tick = tonumber(tick)
        --print(peer, tick)
        tot = tot + 1
        if tick <= 4000 then
            tot = tot - 1
            -- wait
        elseif tick > TICK then
            TICK = tick
            --print(peer, tick)
        elseif tick == TICK then
            ok = ok + 1
        else
            assert(tick < TICK)
print(peer,tick,TICK)
            --no = no + 1
            no = no + (TICK-tick)/5
        end
    elseif bak then
        bak = tonumber(bak)
        BAKS = BAKS + bak
    end
end

print('TOT', tot)
print('NO',no, 'OK',ok, 'TOT',no+ok)
print('BAKS', BAKS)
