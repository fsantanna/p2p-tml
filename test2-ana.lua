-- lua5.3 test-ana-2.lua 120 50

local total,_LATENCY_ = ...

local _PEERS_ = 21
local _FPS_   = 50

local _TOTAL_ = _FPS_*total

local ok,no,tot = 0,0,0
local SECS = 0
local TICK = 0

local POS  = {}
local OFFS = {}
local TCKS = {}

local NET  = {
     [0] = {  1 },
     [1] = {  0,  2 },
     [2] = {  1,  3 },
     [3] = {  2,  4,  8 },
     [4] = {  3,  5 },
     [5] = {  4,  6 },
     [6] = {  5,  7,  9 },
     [7] = {  6,  8 },
     [8] = {  3,  7 },
     [9] = {  6, 10 },
    [10] = {  9, 11 },
    [11] = { 10, 12 },
    [12] = { 11, 13, 14, 15, 16, 17 },
    [13] = { 12, 14, 15, 16, 17 },
    [14] = { 12, 13, 15, 16, 17 },
    [15] = { 12, 13, 14, 16, 17, 18 },
    [16] = { 12, 13, 14, 15, 17 },
    [17] = { 12, 13, 14, 15, 16 },
    [18] = { 15, 19 },
    [19] = { 18, 20 },
    [20] = { 19 },
};

function is_up (i, j, cache)
    cache = cache or {}
    if cache[j] then
        return false
    end
    cache[j] = true
    if i == j then
        return true
    end
    for _,v in ipairs(NET[j]) do
        if (not OFFS[v]) or (OFFS[v].state ~= 'offline') then
            if is_up(i, v, cache) then
                return true
            end
        end
    end
    return false
end

function max_tick (i)
    local ret = 0
    for j=0, _PEERS_-1 do
--print(i,j, is_up(i,j))
        if is_up(i,j) then
            ret = math.max(ret, TCKS[j] or 0)
        end
    end
    return ret
end

for l in io.lines('out2.log') do
    local n1,tick,pos = string.match(l,'%[(%d+)%] tick (%d+) pos=(.*)')
    local n2 = string.match(l,'%[(%d+)%] OFFLINE')
    local n3 = string.match(l,'%[(%d+)%] ONLINE')
    local s  = string.match(l,'=== (%d+) ===')

    n1   = tonumber(n1)
    n2   = tonumber(n2)
    n3   = tonumber(n3)
    s    = tonumber(s)
    tick = tonumber(tick)

    if s then
        SECS = s
    elseif n1 then
        if tick > TICK then
            TICK = tick
        end
        TCKS[n1] = tick
        if tick == _TOTAL_ then
            POS[n1] = state
        end
        if OFFS[n1] and OFFS[n1].state=='online' then
print(n1, SECS, tick, max_tick(n1), tick>=max_tick(n1)-_LATENCY_*5)
            if tick >= max_tick(n1)-_LATENCY_*5 then
                OFFS[n1] = { state='done', ret=(SECS-OFFS[n1].secs) }
            end
        end
    elseif n2 then
        OFFS[n2] = { state='offline' }
    elseif n3 and OFFS[n3] and OFFS[n3].state=='offline' then
        OFFS[n3] = { state='online', secs=SECS }
    end
end

local pos = POS[0]
for i=1, _PEERS_-1 do
    assert(POS[i] == pos)
end

for i=0, _PEERS_-1 do
    assert(OFFS[i], i)
    print(i, OFFS[i].ret)
end
