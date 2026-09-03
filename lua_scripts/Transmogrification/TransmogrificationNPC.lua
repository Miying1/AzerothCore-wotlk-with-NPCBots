local AIO = AIO or require("AIO")

-- 设置为实际幻化 NPC 的 entry，0 表示不注册 NPC 脚本。
local TRANSMOGRIFICATION_NPC_ENTRY = 190010

local AIO_NAMESPACE = "TransmogrificationServer"
local AIO_HANDLER = "TransmogrificationFrame"
local GOSSIP_ACTION_OPEN = 1
local GOSSIP_ACTION_CLOSE = 2

local function OpenTransmogrification(player)
    player:GossipComplete()
    AIO.Handle(player, AIO_NAMESPACE, AIO_HANDLER)
end

local function OnTransmogrificationNpcHello(event, player, creature)
    player:GossipClearMenu()
    player:GossipMenuAddItem(0, "打开幻化界面", 0, GOSSIP_ACTION_OPEN)
    player:GossipSendMenu(1, creature)
end

local function OnTransmogrificationNpcSelect(event, player, creature, sender, action)
    if action == GOSSIP_ACTION_OPEN then
        OpenTransmogrification(player)
    else
        player:GossipComplete()
    end
end

if TRANSMOGRIFICATION_NPC_ENTRY > 0 then
    RegisterCreatureGossipEvent(TRANSMOGRIFICATION_NPC_ENTRY, 1, OnTransmogrificationNpcHello)
    RegisterCreatureGossipEvent(TRANSMOGRIFICATION_NPC_ENTRY, 2, OnTransmogrificationNpcSelect)
    print("[ALE] 幻化 NPC 脚本已加载，NPC entry: " .. TRANSMOGRIFICATION_NPC_ENTRY)
end
