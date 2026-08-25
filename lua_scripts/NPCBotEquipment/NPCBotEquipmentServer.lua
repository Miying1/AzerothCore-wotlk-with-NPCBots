local AIO = AIO or require("AIO")

local NAMESPACE = "NPCBotEquipment"
local handlers = AIO.AddHandlers(NAMESPACE, {})

local function IsInteger(value, minimum, maximum)
    return type(value) == "number" and value == math.floor(value) and value >= minimum and value <= maximum
end

local function IsGuidLow(value)
    return type(value) == "string" and string.match(value, "^[1-9][0-9]*$") ~= nil and string.len(value) <= 20
end

local function IsRevision(value, allowEmpty)
    if type(value) ~= "string" then
        return false
    end

    if value == "" then
        return allowEmpty == true
    end

    return string.len(value) <= 32 and string.match(value, "^[0-9a-f]+$") ~= nil
end

local function IsRequestId(value)
    return IsInteger(value, 0, 2147483647)
end

local function SendInvalidRequest(player, handlerName, requestId, botEntry, botGuidLow, botSlot)
    AIO.Handle(player, NAMESPACE, handlerName, {
        requestId = requestId,
        ok = false,
        code = "INVALID_REQUEST",
        message = "请求参数无效",
        botEntry = botEntry,
        botGuidLow = botGuidLow,
        botSlot = botSlot,
        refreshRequired = false
    })
end

local function CopyResponseContext(result, request)
    result.requestId = request.requestId
    result.botEntry = request.botEntry
    result.botGuidLow = request.botGuidLow
    result.botSlot = request.botSlot
    result.changedSlot = request.botSlot
    return result
end

function handlers.RequestSnapshot(player, request)
    if type(request) ~= "table" or not IsRequestId(request.requestId) or
        not IsInteger(request.botEntry, 1, 4294967295) or not IsGuidLow(request.botGuidLow) then
        SendInvalidRequest(player, "SnapshotResult", type(request) == "table" and request.requestId or 0)
        return
    end

    local result = player:GetNPCBotEquipmentSnapshot(request.botEntry, request.botGuidLow)
    CopyResponseContext(result, request)
    AIO.Handle(player, NAMESPACE, "SnapshotResult", result)
end

function handlers.RequestCandidates(player, request)
    if type(request) ~= "table" or not IsRequestId(request.requestId) or
        not IsInteger(request.botEntry, 1, 4294967295) or not IsGuidLow(request.botGuidLow) or
        not IsInteger(request.botSlot, 0, 17) or not IsRevision(request.equipmentRevision or "", true) then
        SendInvalidRequest(
            player,
            "CandidatesResult",
            type(request) == "table" and request.requestId or 0,
            type(request) == "table" and request.botEntry or nil,
            type(request) == "table" and request.botGuidLow or nil,
            type(request) == "table" and request.botSlot or nil)
        return
    end

    local result = player:GetNPCBotEquipCandidates(request.botEntry, request.botGuidLow, request.botSlot)
    CopyResponseContext(result, request)
    AIO.Handle(player, NAMESPACE, "CandidatesResult", result)
end

function handlers.EquipCandidate(player, request)
    if type(request) ~= "table" or not IsRequestId(request.requestId) or
        not IsInteger(request.botEntry, 1, 4294967295) or not IsGuidLow(request.botGuidLow) or
        not IsInteger(request.botSlot, 0, 17) or not IsGuidLow(request.itemGuid) or
        not IsInteger(request.expectedItemEntry, 1, 4294967295) or
        not IsRevision(request.expectedEquipmentRevision, false) then
        SendInvalidRequest(
            player,
            "EquipResult",
            type(request) == "table" and request.requestId or 0,
            type(request) == "table" and request.botEntry or nil,
            type(request) == "table" and request.botGuidLow or nil,
            type(request) == "table" and request.botSlot or nil)
        return
    end

    local result = player:EquipNPCBotItemFromInventory(
        request.botEntry,
        request.botGuidLow,
        request.botSlot,
        request.itemGuid,
        request.expectedItemEntry,
        request.expectedEquipmentRevision,
        false)

    CopyResponseContext(result, request)
    AIO.Handle(player, NAMESPACE, "EquipResult", result)
end

function handlers.Unequip(player, request)
    if type(request) ~= "table" or not IsRequestId(request.requestId) or
        not IsInteger(request.botEntry, 1, 4294967295) or not IsGuidLow(request.botGuidLow) or
        not IsInteger(request.botSlot, 0, 17) or not IsRevision(request.expectedEquipmentRevision, false) then
        SendInvalidRequest(
            player,
            "UnequipResult",
            type(request) == "table" and request.requestId or 0,
            type(request) == "table" and request.botEntry or nil,
            type(request) == "table" and request.botGuidLow or nil,
            type(request) == "table" and request.botSlot or nil)
        return
    end

    local result = player:UnequipNPCBotItem(
        request.botEntry,
        request.botGuidLow,
        request.botSlot,
        request.expectedEquipmentRevision,
        false)

    CopyResponseContext(result, request)
    AIO.Handle(player, NAMESPACE, "UnequipResult", result)
end
