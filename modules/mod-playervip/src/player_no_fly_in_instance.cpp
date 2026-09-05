#include "DBCStores.h"
#include "Map.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "UnitScript.h"

// 施放拦截：标准飞行坐骑（带 SPELL_ATTR4_ONLY_FLYING_AREAS）在副本内施放会被拦下。
// 注意部分自定义/GM 飞行方式不携带该属性，不会走到此钩子，需依靠下方周期检测兜底。
class PlayerNoFlyInInstance : public PlayerScript
{
public:
    PlayerNoFlyInInstance() : PlayerScript("PlayerNoFlyInInstance") { }

    bool OnPlayerCanFlyInZone(Player* /*player*/, uint32 mapId, uint32 /*zoneId*/, SpellInfo const* /*bySpell*/) override
    {
        MapEntry const* mapEntry = sMapStore.LookupEntry(mapId);
        if (mapEntry && mapEntry->IsDungeon())
        {
            return false;
        }

        return true;
    }
};

// 周期检测：玩家一旦在副本内处于飞行状态（含原地召唤飞行坐骑、从副本外飞入等
// 不走施放校验的情况），立即取消飞行坐骑并禁用飞行，使其落地。
class PlayerNoFlyInInstanceUpdater : public UnitScript
{
public:
    PlayerNoFlyInInstanceUpdater() : UnitScript("PlayerNoFlyInInstanceUpdater") { }

    void OnUnitUpdate(Unit* unit, uint32 /*diff*/) override
    {
        if (!unit || !unit->IsPlayer())
        {
            return;
        }

        Player* player = unit->ToPlayer();
        if (player->isGameMaster())
        {
            return;
        }
        Map* map = player->GetMap();
        if (map && map->IsDungeon() && player->IsFlying())
        {
            player->Dismount();
            player->SetCanFly(false);
        }
    }
};

void AddPlayerNoFlyInInstanceScript()
{
    new PlayerNoFlyInInstance();
    new PlayerNoFlyInInstanceUpdater();
}
