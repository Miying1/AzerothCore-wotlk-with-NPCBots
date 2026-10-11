void AddRandomEnchantsScripts();
void AddReEnchantsNpcScripts();
void AddEnchantTransferScripts();
void AddTrinketEssenceScripts();
// 四分组随机附魔（法系/物理/防御/治疗），处理自定义装备段 entry >= 99000
void AddSC_item_random_enchant();
void Addmod_random_enchantsScripts()
{
    AddRandomEnchantsScripts();
    AddReEnchantsNpcScripts();
    AddEnchantTransferScripts();
    AddTrinketEssenceScripts();
    AddSC_item_random_enchant();
}
