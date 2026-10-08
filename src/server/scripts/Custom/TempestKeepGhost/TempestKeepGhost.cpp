/*
 * 风暴要塞（Tempest Keep）死亡释放：保持幽灵状态 + 骑飞行坐骑
 *
 * 目标：在风暴要塞系列副本中死亡并释放灵魂时，不再被服务器“直接复活”，
 *       而是保持幽灵状态并被传送到墓地；由于墓地在外面的虚空风暴，
 *       幽灵会像诺森德户外区域一样自动骑上迅捷幽灵狮鹫（夜精灵为迅捷飞行小精灵）飞回副本。
 *
 * 实现拆成两部分：
 *  1.（本文件）去掉风暴要塞各区域的 AREA_FLAG_NEED_FLY。
 *     核心 Player::RepopAtGraveyard() 对带该标志的区域，会在释放瞬间直接把玩家复活
 *     （原意是浮空副本里的幽灵无法走回尸体），去掉后即可正常保持幽灵状态。
 *  2.（SQL：data/sql/custom/db_world/2026_10_08_00_spell_area.sql）
 *     给虚空风暴(3523)添加 spell_area 行：幽灵光环 8326 + autocast，
 *     使处于幽灵状态的玩家在该区域自动获得飞行坐骑。
 */

#include "DBCStores.h"
#include "Log.h"
#include "ScriptMgr.h"

namespace
{
    // 判断某个区域是否属于风暴要塞系列副本。
    // 同时按“地图”和“区域ID”匹配：AreaTable 里这些副本区域的 mapid 就是各自的副本地图ID，
    // 但不同客户端版本的字段可能有差异，两个条件取并集更稳妥。
    bool IsTempestKeepArea(AreaTableEntry const* areaEntry)
    {
        if (areaEntry->mapid == MAP_TEMPEST_KEEP                // 550 风暴要塞（The Eye，团队副本）
            || areaEntry->mapid == MAP_TEMPEST_KEEP_THE_ARCATRAZ // 552 禁魔监狱
            || areaEntry->mapid == MAP_TEMPEST_KEEP_THE_BOTANICA // 553 生态船
            || areaEntry->mapid == MAP_TEMPEST_KEEP_THE_MECHANAR) // 554 能源舰
            return true;

        switch (areaEntry->ID)
        {
            case 3845: // 风暴要塞（The Eye）
            case 3847: // 生态船
            case 3848: // 禁魔监狱
            case 3849: // 能源舰
                return true;
            default:
                return false;
        }
    }
}

class tempest_keep_ghost_worldscript : public WorldScript
{
public:
    tempest_keep_ghost_worldscript() : WorldScript("tempest_keep_ghost_worldscript") { }

    void OnStartup() override
    {
        uint32 modifiedCount = 0;

        for (uint32 i = 0; i < sAreaTableStore.GetNumRows(); ++i)
        {
            AreaTableEntry* areaEntry = const_cast<AreaTableEntry*>(sAreaTableStore.LookupEntry(i));
            if (!areaEntry || !IsTempestKeepArea(areaEntry))
                continue;

            // 取消“释放后直接在墓地复活”，改为保持幽灵状态，以便骑马飞回副本
            areaEntry->flags &= ~AREA_FLAG_NEED_FLY;
            ++modifiedCount;
        }

        LOG_INFO("scripts", "Tempest Keep: 已对 {} 个区域关闭 NEED_FLY（死亡后保持幽灵、可骑飞行坐骑）。", modifiedCount);
    }
};

void AddSC_tempest_keep_ghost()
{
    new tempest_keep_ghost_worldscript();
}
