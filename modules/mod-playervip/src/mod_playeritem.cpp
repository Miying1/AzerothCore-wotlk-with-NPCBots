 

#include "Configuration/Config.h"
#include "ScriptMgr.h"
#include "Player.h"
#include "Chat.h"

 
 
class ReNameItem : public ItemScript
{
public:
    ReNameItem() : ItemScript("ReNameItem") { }

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
class ChangeFactionItem : public ItemScript
{
public:
    ChangeFactionItem() : ItemScript("ChangeFactionItem") { }

    bool OnUse(Player* p, Item* i, const SpellCastTargets&) override
    {
        if (p->IsInCombat() || p->IsInFlight() || p->GetMap()->IsBattlegroundOrArena())
        {
            ChatHandler(p->GetSession()).PSendSysMessage("你现在还不能使用它!");
            return false;
        }
        p->SetAtLoginFlag(AT_LOGIN_CHANGE_FACTION); 
        p->DestroyItemCount(i->GetEntry(), 1, true);
        ChatHandler(p->GetSession()).PSendSysMessage("成功: 请返回角色登录界面选择阵营."); 
        return true;
    }
};
class MaxLevelItem : public ItemScript
{
public:
    MaxLevelItem() : ItemScript("MaxLevelItem") { }

    bool OnUse(Player* p, Item* i, const SpellCastTargets&) override
    {
        if (p->IsInCombat() || p->IsInFlight() || p->GetMap()->IsBattlegroundOrArena())
        {
            ChatHandler(p->GetSession()).PSendSysMessage("你现在还不能使用它!");
            return false;
        }
        uint16 max_level = 80;
        uint16 cur_level = p->GetLevel();
        if (cur_level >= max_level)
        {
            ChatHandler(p->GetSession()).PSendSysMessage("你已经满级,无法使用该物品!");
            return false;
        }
        if(!p->HasActiveSpell(33388))
            p->learnSpell(33388);
        if (!p->HasActiveSpell(33391))
            p->learnSpell(33391);
        if (!p->HasActiveSpell(34090))
            p->learnSpell(34090);
        if (!p->HasActiveSpell(54197))
            p->learnSpell(54197);
        if(cur_level<40)
            p->AddItem(49282,1);
        if (cur_level < 60)
            p->AddItem(35225, 1); 
        p->GiveLevel(max_level);
        p->SetUInt32Value(PLAYER_XP, 0);
        p->DestroyItemCount(i->GetEntry(), 1, true);
        ChatHandler(p->GetSession()).PSendSysMessage("成功: 恭喜满级！.");
        return true;
    }
};

 
 
void AddPlayerItemScripts()
{ 
    new ReNameItem();
    new ChangeFactionItem();
    new MaxLevelItem();
}
