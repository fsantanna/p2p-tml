-- lua5.3 test-sheet-2.lua sheet-2.log

local F   = ...
local LAT = nil
local EVT = nil
local MUL = nil
local T   = {}

local CUR = nil -- { lat=X, mul=Y, evt=Z }
local RET = {}

for l in io.lines(F) do
    local lat,mul,evt = string.match(l,'===%s+(%d+)%s+(%d+)%s+(%d+)')
    if lat and mul and evt then
        CUR = { lat=lat, mul=mul, evt=evt, str=lat..'.'..mul..'.'..evt }
    end

    local lat,mul,evt,n,fwd,bak,f = string.match(l,'%s+(%d+)%s+(%d+)%s+(%d+)%s+(%d+)%s+(%d+%.%d+)%s+(%d+%.%d+)%s+all%-(%d+)%.log')
    if lat then
        assert(CUR)
        assert(tonumber(f) >= 60)
        assert(CUR.lat==lat and CUR.mul==mul and CUR.evt==evt)
        local s = lat..'.'..mul..'.'..evt
        fwd = tonumber(fwd)
        bak = tonumber(bak)
        local t = RET[s] or { n=0, fwd=0, bak=0 }
        RET[s] = t
        t.n   = t.n   + 1
        t.fwd = t.fwd + fwd
        t.bak = t.bak + bak
    end
end

for _,lat in ipairs{'5','25','50','100','250','500'} do
    for _,val in ipairs{'bak'} do
        for _,mul in ipairs{'5','2','1'} do
            for _,evt in ipairs{'5','10','25','50','100','200'} do
                local s = lat..'.'..mul..'.'..evt
                local v = RET[s][val] / RET[s].n
                local p = string.format('%3.1f\t', v)
                io.write(p)
            end
        end
        print()
    end
end
