#include "Chat.h"
#include "Config.h"
#include "DisableMgr.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SpellInfo.h"

class TransmogItemScript : public ItemScript
{
public:
    TransmogItemScript() : ItemScript("TransmogItemScript") { }

    bool OnUse(Player* p, Item* i, const SpellCastTargets&) override
    { 
        if (p->CastSpell(p->ToUnit(), 100004, false, i) == SPELL_CAST_OK) {
            if (urand(1, 2) == 1)
                p->SetDisplayId(11657);
            else
            {
                p->SetDisplayId(4527);
            }
        }
        ChatHandler(p->GetSession()).PSendSysMessage("成功.");
        return true;
    }
};

void AddSC_TransmogItemScript()
{
    new TransmogItemScript();
}
