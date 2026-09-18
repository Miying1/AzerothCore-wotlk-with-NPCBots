/*
 * 猎人宠物模型缩放脚本 —— mod-playervip 模块
 *
 * 功能：
 *   当猎人宠物（HUNTER_PET）加入世界、首次驯服成功、升级或被复活时，根据其生物 entry
 *   在下方硬编码字典中查找对应的缩放倍率，并应用到宠物模型上。
 *
 * 说明：
 *   - 缩放字典直接硬编码在代码中（g_hunterPetScaleMap），key 为生物 entry，value 为缩放倍率。
 *   - 缩放倍率 1.0 表示原始大小，大于 1.0 放大，小于 1.0 缩小。
 *   - 核心会在多个时机把宠物缩放（OBJECT_FIELD_SCALE_X）重置为原生值，只挂钩一个时机
 *     会出现“缩放被绕过”的情况，因此这里同时使用了三个时机：
 *
 *     1) 宠物加入世界（登录、从兽栏召唤、重新召唤宠物）
 *        -> PetScript::OnPetAddToWorld
 *
 *     2) 初始化属性（首次驯服成功、升级、从数据库读取宠物）
 *        -> PetScript::OnInitStatsForLevel
 *        首次驯服时 Spell::EffectTameCreature 是先 AddToMap 再 SetMinion，AddToMap 时宠物
 *        还没有主人（UNIT_FIELD_SUMMONEDBY 为空），Pet::AddToWorld 里的 OnPetAddToWorld
 *        钩子不会触发；而 Guardian::InitStatsForLevel 又会把缩放重置为原生值，
 *        所以必须靠这个钩子补上（该钩子在重置之后触发）。
 *
 *     3) 显示模型被重置（死亡后复活时 Spell::EffectResurrectPet 会调用 SetDisplayId）
 *        -> UnitScript::OnDisplayIdChange + 下一帧重新应用（原因见下方注释）
 */
#include "EventProcessor.h"
#include "Pet.h"
#include "PetScript.h"
#include "ScriptMgr.h"
#include "UnitScript.h"

#include <unordered_map>

namespace
{
// 硬编码的猎人宠物缩放字典：生物 entry -> 缩放倍率
// 需要新增时，直接在下方追加一行：{ entry, 缩放倍率 },
std::unordered_map<uint32, float> const g_hunterPetScaleMap =
{
    { 94000, 0.8f }, // 可驯服生物召唤卷轴（物品 69000）召唤出的生物
    { 94001, 0.8f },
    { 94002, 0.32f },
    { 94003, 0.32f },
    { 94004, 0.32f },
    { 94005, 0.32f },
    { 94006, 0.32f },
    { 94007, 0.8f },
    { 94008, 0.7f },
    { 94009, 0.7f },
    { 94010, 0.13f },
};

// 查询该单位是否为字典中的猎人宠物；命中时返回 true 并写入 scale
bool TryGetHunterPetScale(Unit* unit, float& scale)
{
    if (!unit || !unit->IsHunterPet())
        return false;

    auto const itr = g_hunterPetScaleMap.find(unit->GetEntry());
    if (itr == g_hunterPetScaleMap.end())
        return false;

    scale = itr->second;
    return true;
}

// 立即把字典中的缩放倍率应用到宠物身上；不在字典中的单位直接跳过
void ApplyHunterPetScale(Unit* unit)
{
    float scale = 0.0f;
    if (TryGetHunterPetScale(unit, scale))
        unit->SetObjectScale(scale);
}

// 猎人宠物缩放：加入世界 / 初始化属性
class HunterPetScaleScript : public PetScript
{
public:
    HunterPetScaleScript() : PetScript("HunterPetScaleScript") { }

    // 宠物加入世界时应用（登录、从兽栏召唤、召唤宠物）
    void OnPetAddToWorld(Pet* pet) override
    {
        ApplyHunterPetScale(pet);
    }

    // 初始化属性时应用（首次驯服成功、升级、从数据库读取宠物）
    // 该钩子在 Guardian::InitStatsForLevel 末尾触发，位置在缩放被重置为原生值之后
    void OnInitStatsForLevel(Guardian* guardian, uint8 /*petlevel*/) override
    {
        ApplyHunterPetScale(guardian);
    }
};

// 猎人宠物缩放：复活等操作会重置显示模型，进而把缩放重置为 1.0，这里补一次缩放
class HunterPetScaleDisplayScript : public UnitScript
{
public:
    HunterPetScaleDisplayScript() : UnitScript("HunterPetScaleDisplayScript") { }

    // 宠物复活时 EffectResurrectPet 会调用 Pet::SetDisplayId(原生模型)，
    // 而 Creature::SetDisplayId 在触发本回调之后还会执行一次 SetObjectScale(displayScale)，
    // 所以在这里立即设置缩放会被覆盖，必须推迟到下一帧再设置（用宠物自己的事件队列，
    // 宠物被销毁时事件会随之销毁，不会出现悬空指针）。
    void OnDisplayIdChange(Unit* unit, uint32 /*displayId*/) override
    {
        float scale = 0.0f;
        if (!TryGetHunterPetScale(unit, scale))
            return;

        unit->m_Events.AddEventAtOffset([unit, scale]()
        {
            unit->SetObjectScale(scale);
        }, 1ms);
    }
};
}

void AddSC_hunter_pet_scale()
{
    new HunterPetScaleScript();
    new HunterPetScaleDisplayScript();
}
