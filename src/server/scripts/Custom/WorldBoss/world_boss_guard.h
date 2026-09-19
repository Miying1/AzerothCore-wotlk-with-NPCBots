/*
 * 世界BOSS统一战斗机制基类（归属锁定 + 脱战距离 + 技能伤害缩放）
 *
 * 所有自定义世界BOSS本体（奥、伊利丹）继承 WorldBossGuardAI，召唤物继承 WorldBossSummonAI，
 * 从而共享同一套战斗机制，避免为每个BOSS重复实现。
 *
 * 参考裂隙BOSS（HeroicDungeonRift::BossAIBase）的缩放实现：在 AI 的 DamageDealt 回调里，
 * 按“施法者身份”判定是否需要缩放，再依据倍率表按法术 ID 统一放大，从而天然隔离原版内容，
 * 无需依赖全局 entry 范围守卫。
 */

#ifndef CUSTOM_WORLD_BOSS_GUARD_H
#define CUSTOM_WORLD_BOSS_GUARD_H

#include "Position.h"
#include "ScriptedCreature.h"
#include "world_boss_common.h"

class SpellInfo;

// 世界BOSS（83级 10 人奥杜尔强度）法术伤害倍率条目。
// direct   = 直接伤害倍率（SPELL_DIRECT_DAMAGE）
// periodic = DOT 周期伤害每跳倍率（DOT）
// 两者独立，值为 0 表示该侧不缩放。
struct WorldBossSpellScale
{
    float direct;
    float periodic;
};

// 查询法术缩放条目；未配置返回 nullptr。
WorldBossSpellScale const* GetWorldBossSpellScale(uint32 spellId);

// 世界BOSS本体统一基类：归属锁定、脱战距离、技能伤害缩放。
class WorldBossGuardAI : public WorldBossAI
{
public:
    explicit WorldBossGuardAI(Creature* creature);

    // 进入战斗：通用逻辑 + 记录位置并锁定开怪队伍。
    void JustEngagedWith(Unit* who) override;

    // 重置/死亡：通用逻辑 + 清除锁定状态。
    void Reset() override;
    void JustDied(Unit* killer) override;

    // 脱战：先还原被 CheckLeash() 临时改写的核心回家基准，再交给基类执行（保证“走回家”目标点正确）。
    void EnterEvadeMode(EvadeReason why) override;

    // 技能伤害缩放：按倍率表对直伤与 DOT 统一放大。
    void DamageDealt(Unit* victim, uint32& damage, DamageEffectType damageType, SpellSchoolMask schoolMask) override;

    // 记录最近施放的法术，供 DamageDealt 识别伤害来源。
    void OnSpellCast(SpellInfo const* spell) override;
    void OnSpellStart(SpellInfo const* spell) override;

protected:
    // 脱战距离检测：超过 WORLD_BOSS_LEASH_RANGE 则脱战。
    // 返回 true 表示已触发脱战，子类 UpdateAI 应立即返回。
    bool CheckLeash();

    // 记录进入战斗位置并锁定开怪队伍。
    void LockToGroup(Unit* who);

    // 清除锁定状态。
    void Unlock();

    // 还原战斗中临时改写的核心回家基准（见 CheckLeash）。
    void RestoreCoreHome();

private:
    Position _homePosition;        // 进入战斗时的位置（脱战距离基准）
    ObjectGuid _ownerGuid;         // 开怪玩家
    ObjectGuid _groupGuid;         // 开怪玩家所在队伍/团队（单独玩家时为空）
    bool _locked = false;          // 是否已锁定（战斗中）
    uint32 _lastCastSpellId = 0;   // 最近施放的法术
    Position _savedHomePosition;   // 进入战斗时的核心回家基准（战斗中会被临时改写，脱战/退场前还原）
    bool _hasSavedHome = false;    // 是否已保存上述基准
};

// 世界BOSS召唤物基类：共享技能伤害缩放（按倍率表统一放大）。
class WorldBossSummonAI : public ScriptedAI
{
public:
    explicit WorldBossSummonAI(Creature* creature);

    void DamageDealt(Unit* victim, uint32& damage, DamageEffectType damageType, SpellSchoolMask schoolMask) override;
    void OnSpellCast(SpellInfo const* spell) override;
    void OnSpellStart(SpellInfo const* spell) override;

protected:
    // 预置最近施放法术，供瞬发直伤（OnSpellStart 对瞬发不触发，OnSpellCast 在伤害结算之后）
    // 的首次伤害识别来源，避免首跳因 _lastCastSpellId 尚未更新而漏缩放。
    void SetLastCastSpellId(uint32 spellId) { _lastCastSpellId = spellId; }

private:
    uint32 _lastCastSpellId = 0; // 最近施放的法术
};

#endif // CUSTOM_WORLD_BOSS_GUARD_H
