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
        if (p->IsInCombat() || p->IsInFlight() || p->GetMap()->IsBattlegroundOrArena())
        {
            ChatHandler(p->GetSession()).PSendSysMessage("你现在还不能使用它!");
            return false;
        }

        p->SetAtLoginFlag(AT_LOGIN_RENAME);
        p->DestroyItemCount(i->GetEntry(), 1, true);
        ChatHandler(p->GetSession()).PSendSysMessage("成功: 请返回角色登录界面以修改角色名称.");
        return true;
    }
};

void AddSC_TransmogItemScript()
{
    new TransmogItemScript();
}
