#include "DBCStores.h"
#include "Player.h"
#include "ScriptMgr.h"

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

void AddPlayerNoFlyInInstanceScript()
{
    new PlayerNoFlyInInstance();
}
