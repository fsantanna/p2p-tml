-- lua5.3 test2-sheet.lua

local F   = 'all2.log'
local LAT = nil
local EVT = nil
local MUL = nil
local T   = {}

local CUR = nil -- { lat=X, mul=Y, evt=Z }
local RET = {}

for l in io.lines(F) do
    local lat,mul,evt = string.match(l,'=== LAT=(%d+), MULT=(%d+), EVTS=(%d+)')
    if lat and mul and evt then
        CUR = lat..'.'..mul..'.'..evt
    end

    local peer, secs = string.match(l,'^(%d+)%s+(%d+)')
    if peer and secs then
        assert(CUR)
--print(s, bak)
        secs = tonumber(secs)
        local t = RET[CUR] or { n=0, secs=0 }
        RET[CUR] = t
        t.n    = t.n    + 1
        t.secs = t.secs + secs
    end
end

--print("-=-=-=-=-=-")
for _,lat in ipairs{'5','25','50','100','250','500'} do
    for _,val in ipairs{'secs'} do
        for _,mul in ipairs{'5','2','1'} do
            for _,evt in ipairs{'5','10','25','50','100','200'} do
                local s = lat..'.'..mul..'.'..evt
                local v = RET[s][val] / RET[s].n
                local p = string.format('%3.1f\t', v)
--print(s)
                io.write(p)
--print()
            end
        end
        print()
    end
end
