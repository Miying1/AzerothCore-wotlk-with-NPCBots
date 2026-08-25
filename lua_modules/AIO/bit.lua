-- AIO 服务端兼容层：为未内置 bit/bit32 的 Lua 5.2 环境提供 CRC32 所需的位运算。
-- 仅实现 AIO Dep_crc32lua 使用的 bxor、bnot、band 和 rshift。
local MOD = 4294967296
local HALF = 2147483648
local function normalize(value)
    value = value % MOD
    if value >= HALF then
        return value - MOD
    end
    return value
end

local function bitwiseBinary(left, right, operation)
    left = left % MOD
    right = right % MOD
    local result = 0
    local bit = 1
    for _ = 1, 32 do
        local leftBit = left % 2
        local rightBit = right % 2
        if operation(leftBit, rightBit) then
            result = result + bit
        end
        left = math.floor(left / 2)
        right = math.floor(right / 2)
        bit = bit * 2
    end
    return normalize(result)
end

local function bxor(left, right)
    return bitwiseBinary(left, right, function(leftBit, rightBit)
        return leftBit ~= rightBit
    end)
end

local function band(left, right)
    return bitwiseBinary(left, right, function(leftBit, rightBit)
        return leftBit == 1 and rightBit == 1
    end)
end

local function bnot(value)
    return normalize(MOD - 1 - (value % MOD))
end

local function rshift(value, count)
    value = value % MOD
    return math.floor(value / (2 ^ count))
end

return {
    bxor = bxor,
    band = band,
    bnot = bnot,
    rshift = rshift
}
