/*
 * 猎人宠物模型缩放脚本 —— mod-playervip 模块
 *
 * 功能：
 *   当猎人宠物（HUNTER_PET）加入世界时，根据其生物 entry 在下方硬编码字典中查找
 *   对应的缩放倍率，并应用到宠物模型上，实现对特定生物召唤/驯服的宠物进行大小缩放。
 *
 * 说明：
 *   - 缩放字典直接硬编码在代码中（g_hunterPetScaleMap），key 为生物 entry，value 为缩放倍率。
 *   - 缩放倍率 1.0 表示原始大小，大于 1.0 放大，小于 1.0 缩小。
 *   - 通过 PetScript::OnPetAddToWorld 钩子实现，仅在宠物真正加入世界时应用一次。
 */

#include "Pet.h"
#include "PetScript.h"
#include "ScriptMgr.h"

#include <unordered_map>

namespace
{
// 硬编码的猎人宠物缩放字典：生物 entry -> 缩放倍率
// 需要新增时，直接在下方追加一行：{ entry, 缩放倍率 },
std::unordered_map<uint32, float> const g_hunterPetScaleMap =
{
    { 94000, 0.8f }, // 可驯服生物召唤卷轴（物品 69000）召唤出的生物，放大 1.5 倍（示例）
    // { 12345, 0.6f }, // 缩小到 0.6 倍（示例）
    // { 23456, 2.0f }, // 放大到 2.0 倍（示例）
};

// 猎人宠物模型缩放脚本
class HunterPetScaleScript : public PetScript
{
public:
    HunterPetScaleScript() : PetScript("HunterPetScaleScript") { }

    void OnPetAddToWorld(Pet* pet) override
    {
        // 只处理猎人宠物
        if (!pet || pet->getPetType() != HUNTER_PET)
            return;

        // 在硬编码字典中查找该宠物对应的缩放倍率
        auto const itr = g_hunterPetScaleMap.find(pet->GetEntry());
        if (itr == g_hunterPetScaleMap.end())
            return;

        pet->SetObjectScale(itr->second);
    }
};
}

void AddSC_hunter_pet_scale()
{
    new HunterPetScaleScript();
}
