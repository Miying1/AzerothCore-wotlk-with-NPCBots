/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "SpellAuraEffects.h"
#include "SpellScript.h"
#include "SpellScriptLoader.h"

#include <unordered_map>

// 卡拉赞地图 ID（532）。缩放仅对卡拉赞内的施法者生效，避免污染复用同一法术的其它内容：
// 例如五人英雄裂隙·死亡矿井·曲奇（boss_rift_cookie）复用 29901 酸性之牙，其伤害由
// 裂隙自己的 CastRiftTunedSpell 体系写入基线，本脚本不得再叠加卡拉赞倍率。
constexpr uint32 KARAZHAN_MAP_ID = 532;

// 卡拉赞 80 级 10 人升级：Boss 与召唤物法术伤害倍率表。
// direct   = 直接伤害倍率（SpellScript::OnHit）
// periodic = DOT 周期伤害每跳倍率（AuraScript::OnEffectApply）
// 两者独立，可只填其一；值为 0 表示该侧不缩放。
struct KarazhanSpellScale
{
    float direct;
    float periodic;
};

static std::unordered_map<uint32, KarazhanSpellScale> const KarazhanSpellScaling =
{
    // ---- 直接法术伤害（direct） ----
    { 30383, { 3.0f, 0.0f } }, // 馆长·憎恨之箭 4434-5999 -> 13300-18000
    { 29973, { 3.5f, 0.0f } }, // 埃兰·奥术爆炸 8999-11000 -> 31500-38500
    { 29978, { 4.5f, 0.0f } }, // 埃兰·群体炎爆 6800 -> 30600
    { 29953, { 2.8f, 0.0f } }, // 埃兰·火球术 3909-5290 -> 10950-14810
    { 29954, { 2.9f, 0.0f } }, // 埃兰·寒冰箭 3499-4500 -> 10150-13050
    { 29949, { 8.5f, 0.0f } }, // 埃兰·花环爆炸(穿圈惩罚) 3237-3762 -> 27500-32000
    { 29956, { 3.2f, 0.0f } }, // 埃兰·奥术飞弹(每发, 子法术) 1260-1540 -> 4000-4900
    { 30055, { 3.6f, 0.0f } }, // 泰雷斯蒂安·暗影箭 3187-4312 -> 11470-15520
    { 30852, { 4.7f, 0.0f } }, // 玛克扎尔·暗影新星(AOE) 3000 -> 14100
    { 32337, { 3.7f, 0.0f } }, // 老巫婆·闪电链 2774-3225 -> 10260-11930
    { 30282, { 3.5f, 0.0f } }, // 夜之魇·火球齐射 2549-3450 -> 8920-12080
    { 29904, { 3.4f, 0.0f } }, // 沙迪基斯·音爆 2187-2812 -> 7440-9560
    { 31012, { 2.9f, 0.0f } }, // 桃乐丝·水箭(高频) 2024-2475 -> 5870-7180
    { 32445, { 5.5f, 0.0f } }, // 贞洁圣女·神圣之怒(80码AOE) 1949-2050 -> 10720-11280
    { 30815, { 4.5f, 0.0f } }, // 罗密欧·后跳突刺 1899-2100 -> 8550-9450
    { 30128, { 4.0f, 0.0f } }, // 夜之魇·烟爆 1849-2150 -> 7400-8600
    { 37057, { 4.0f, 0.0f } }, // 夜之魇·烟爆(空战T变体) 1849-2150 -> 7400-8600
    { 29711, { 3.0f, 0.0f } }, // 午夜·击倒(控制+小伤) 1500 -> 4500
    { 29425, { 3.0f, 0.0f } }, // 莫罗斯·凿击(控制+小伤) 949-1050 -> 2850-3150
    { 30050, { 8.0f, 0.0f } }, // 小鬼·火球 180-209 -> 1440-1670
    { 38524, { 3.0f, 0.0f } }, // 虚空幽龙·虚空吐息(子法术) 4163-4837 -> 12490-14510
    { 30860, { 4.0f, 0.0f } }, // 地狱火·灼烧(每跳, 子法术) 875-1125 -> 3500-4500

    // ---- DOT 周期伤害（periodic，每跳） ----
    { 30129, { 0.0f, 2.8f } }, // 夜之魇·焦土 2187-2812/3s -> 6120-7870/3s
    { 29964, { 0.0f, 3.0f } }, // 埃兰·龙息术 2000/2s -> 6000/2s
    { 29522, { 3.4f, 3.1f } }, // 贞洁圣女·神圣之火(直伤+DOT) 直3238-3762 + DOT1750/2s
    { 30210, { 3.9f, 3.5f } }, // 夜之魇·灼热吐息(直伤+DOT) 直3699-4300 + DOT1687-1912/3s
    { 30854, { 0.0f, 3.0f } }, // 玛克扎尔·暗言术:痛 1500/3s -> 4500/3s
    { 37066, { 0.0f, 3.0f } }, // 莫罗斯·绞喉 1075/3s -> 3220/3s
    { 30890, { 4.0f, 3.3f } }, // 朱丽叶·盲目激情(直伤+DOT) 直1500 + DOT750/1s
    { 29906, { 0.0f, 4.0f } }, // 罗卡德·蹂躏(DOT段) 509-690/2s -> 2040-2760/2s
    { 31041, { 3.2f, 4.0f } }, // 狮吼·撕裂(直伤+DOT) 直1599-2400 + DOT500/3s
    { 29901, { 0.0f, 4.0f } }, // 希亚基斯·酸性之牙(DOT段) 500/2s -> 2000/2s

    // ---- 特殊精英怪技能（A 档高危 + 特殊机制怪；倍率低于 Boss，保持精英 < Boss 梯度） ----
    { 29666, { 2.0f, 0.0f } }, // 骷髅引座员·打击+减速 2880-3520 -> 5760-7040
    { 29677, { 2.0f, 0.0f } }, // 幻影舞台工 2975-4025 -> 5950-8050
    { 29673, { 2.0f, 0.0f } }, // 幻影舞台工 3500-4500 -> 7000-9000
    { 29712, { 2.0f, 0.0f } }, // 鬼魅游魂 3700-4300 -> 7400-8600
    { 29717, { 2.2f, 0.0f } }, // 被困灵魂 2625-3375 -> 5775-7425
    { 29765, { 2.2f, 0.0f } }, // 奥术看守 2625-3375 -> 5775-7425
    { 29885, { 3.0f, 0.0f } }, // 奥术异常体(特殊机制怪) 2125-2875 -> 6375-8625
    { 37161, { 2.5f, 0.0f } }, // 虚无窃法者 2250-2750 -> 5625-6875
    { 29939, { 3.0f, 0.0f } }, // 血肉兽/巨型血肉兽 850-1150 -> 2550-3450
    { 29670, { 0.0f, 3.0f } }, // 骷髅引座员·DOT 602-698/2s -> 1806-2094/2s
    { 29935, { 0.0f, 3.0f } }, // 血肉兽/巨型血肉兽·DOT 1150/3s -> 3450/3s

    // ---- 精英怪 B/C 档伤害技能（复核补齐，统一 2.0×，保持精英 < Boss 梯度） ----
    { 18812, { 2.0f, 0.0f } }, // 幽灵马夫 60-80 -> 120-160
    { 29293, { 2.0f, 2.0f } }, // 冷雾寡妇 直1500-2500+DOT238-262/5s -> 3000-5000+476-524
    { 29298, { 2.0f, 0.0f } }, // 暗影蝠 1350-1650 -> 2700-3300
    { 29300, { 2.0f, 0.0f } }, // 巨型暗影蝠 2520-3080 -> 5040-6160
    { 29317, { 2.0f, 0.0f } }, // 暗影掠夺者 2880-3520 -> 5760-7040
    { 29477, { 2.0f, 0.0f } }, // 浪荡女主人 2250-2750 -> 4500-5500
    { 29487, { 2.0f, 0.0f } }, // 黑夜女主人 2250-2750 -> 4500-5500
    { 29491, { 0.0f, 2.0f } }, // 黑夜女主人 DOT 3000/10s -> 6000/10s
    { 29492, { 2.0f, 0.0f } }, // 幻影宾客/暗影掠夺者 1350-1650 -> 2700-3300
    { 29575, { 2.0f, 0.0f } }, // 幽灵哨兵 2700-3300 -> 5400-6600
    { 29576, { 2.0f, 0.0f } }, // 幽灵哨兵 3150-3850 -> 6300-7700
    { 29578, { 0.0f, 2.0f } }, // 幽灵侍从 DOT 600/3s -> 1200/3s
    { 29586, { 2.0f, 0.0f } }, // 幽灵侍从/幻影侍者 675-825 -> 1350-1650
    { 29609, { 2.0f, 0.0f } }, // 幽灵慈善家 3150-3850 -> 6300-7700
    { 29618, { 2.0f, 0.0f } }, // 幽灵学徒 139-161 -> 278-322
    { 29675, { 2.0f, 2.0f } }, // 幽灵面包师 直2250-2750+DOT500/3s -> 4500-5500+1000/3s
    { 29676, { 2.0f, 0.0f } }, // 幽灵面包师 2250-2750 -> 4500-5500
    { 29680, { 2.0f, 0.0f } }, // 幽灵演员 2775-3225 -> 5550-6450
    { 29684, { 2.0f, 0.0f } }, // 幻影卫兵 1850-2150 -> 3700-4300
    { 29919, { 2.0f, 0.0f } }, // 法力扭曲 5012-5538 -> 10024-11076
    { 29922, { 2.0f, 2.0f } }, // 巫师之影 直1530-2070+DOT200/1s -> 3060-4140+400/1s
    { 29925, { 2.0f, 2.0f } }, // 法术之影 直1530-2070+DOT200/1s -> 3060-4140+400/1s
    { 29928, { 2.0f, 2.0f } }, // 暗影掠夺者 直2000+DOT200/3s -> 4000+400/3s
    { 29930, { 0.0f, 2.0f } }, // 幻影侍者/暗影掠夺者 DOT 500/3s -> 1000/3s
    { 30180, { 2.0f, 0.0f } }, // 荷蒙克鲁斯 694-806 -> 1388-1612
    { 30358, { 2.0f, 0.0f } }, // 黑夜女主人 1800-2200 -> 3600-4400
    { 37078, { 2.0f, 0.0f } }, // 魔法恐魔 1620-1980 -> 3240-3960

    // ---- 埃兰·暴风雪（Boss 召唤物 17161，地面 AOE DOT） ----
    { 29951, { 0.0f, 3.0f } }, // 暴风雪 DOT 1313-1687/2s -> 3939-5061/2s

    // ---- 莫罗斯宾客（6选4，精英级召唤物，统一 2.0×） ----
    { 29563, { 2.0f, 2.0f } }, // 宾客·神圣之火 直838-1064+DOT950/2s -> 1676-2128+1900/2s
    { 24212, { 0.0f, 2.0f } }, // 宾客·暗言术痛 DOT 705-796/3s -> 1410-1592/3s
    { 29570, { 0.0f, 2.0f } }, // 宾客·精神鞭笞 DOT 875/1s -> 1750/1s
    { 29386, { 2.0f, 0.0f } }, // 宾客·命令审判 直 1425-1575 -> 2850-3150
};

// 直接伤害缩放：命中点重写总伤害。
class spell_karazhan_direct_scale : public SpellScript
{
    PrepareSpellScript(spell_karazhan_direct_scale);

    void HandleDamage()
    {
        if (!GetCaster() || GetCaster()->GetMapId() != KARAZHAN_MAP_ID)
            return;

        auto const it = KarazhanSpellScaling.find(GetSpellInfo()->Id);
        if (it == KarazhanSpellScaling.end() || it->second.direct <= 0.0f)
            return;

        SetHitDamage(int32(GetHitDamage() * it->second.direct));
    }

    void Register() override
    {
        OnHit += SpellHitFn(spell_karazhan_direct_scale::HandleDamage);
    }
};

// DOT 周期伤害缩放：首次真实应用时按倍率重设每跳伤害，避免逐跳叠加。
class spell_karazhan_periodic_scale : public AuraScript
{
    PrepareAuraScript(spell_karazhan_periodic_scale);

    void HandleApply(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/)
    {
        if (!GetCaster() || GetCaster()->GetMapId() != KARAZHAN_MAP_ID)
            return;

        if (_scaled[aurEff->GetEffIndex()])
            return;

        auto const it = KarazhanSpellScaling.find(GetSpellInfo()->Id);
        if (it == KarazhanSpellScaling.end() || it->second.periodic <= 0.0f)
            return;

        const_cast<AuraEffect*>(aurEff)->SetAmount(int32(aurEff->GetAmount() * it->second.periodic));
        _scaled[aurEff->GetEffIndex()] = true;
    }

    void Register() override
    {
        OnEffectApply += AuraEffectApplyFn(spell_karazhan_periodic_scale::HandleApply, EFFECT_ALL,
            SPELL_AURA_PERIODIC_DAMAGE, AURA_EFFECT_HANDLE_REAL);
    }

private:
    bool _scaled[3] = { false, false, false };
};

void AddSC_karazhan_spell_scaling()
{
    // 一个 loader 同时承载 SpellScript（直接伤害）与 AuraScript（DOT）。
    // 脚本名默认取第一个类名：spell_karazhan_direct_scale。
    RegisterSpellAndAuraScriptPair(spell_karazhan_direct_scale, spell_karazhan_periodic_scale);
}
