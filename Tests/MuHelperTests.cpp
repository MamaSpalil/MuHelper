// ============================================================
//  MuHelper v2 — Test Suite
//  Portable tests for packet structures, class definitions,
//  config validation, and skill lookup tables.
//
//  Build (Linux):
//    g++ -std=c++11 -I/path/to/stubs -I.. -o run_tests MuHelperTests.cpp
//    (requires stub windows.h providing BYTE/WORD/DWORD types)
//  Build (Windows/VS2012):
//    Add to VS solution as a console application.
// ============================================================

#include "../Shared/MuHelperPackets.h"
#include <cassert>
#include <cstdio>
#include <cstring>

static int g_passed = 0;
static int g_failed = 0;

#define TEST(name) static void test_##name()
#define RUN(name) do { \
    printf("  %-50s", #name); \
    try { test_##name(); g_passed++; printf(" PASS\n"); } \
    catch (...) { g_failed++; printf(" FAIL\n"); } \
} while(0)

#define ASSERT_EQ(a, b) do { if ((a) != (b)) { \
    printf(" [%d != %d] ", (int)(a), (int)(b)); throw 1; } } while(0)
#define ASSERT_STREQ(a, b) do { if (strcmp((a),(b)) != 0) { \
    printf(" [\"%s\" != \"%s\"] ", (a), (b)); throw 1; } } while(0)
#define ASSERT_TRUE(x) do { if (!(x)) { printf(" [false] "); throw 1; } } while(0)
#define ASSERT_NOT_NULL(x) do { if ((x) == nullptr) { printf(" [null] "); throw 1; } } while(0)

// ============================================================
//  1. PACKET STRUCTURE SIZE TESTS
// ============================================================
TEST(config_size)
{
    ASSERT_EQ(sizeof(MuHelperConfig), 96);
}

TEST(pkt_enable_size)
{
    ASSERT_EQ(sizeof(PKT_MuHelper_Enable), 8);
}

TEST(pkt_cfg_send_size)
{
    ASSERT_EQ(sizeof(PKT_MuHelper_CfgSend), 100);
}

TEST(pkt_cfg_ack_size)
{
    ASSERT_EQ(sizeof(PKT_MuHelper_CfgAck), 8);
}

TEST(pkt_cfg_request_size)
{
    ASSERT_EQ(sizeof(PKT_MuHelper_CfgRequest), 4);
}

TEST(pkt_cfg_reply_size)
{
    ASSERT_EQ(sizeof(PKT_MuHelper_CfgReply), 100);
}

TEST(pkt_item_picked_size)
{
    ASSERT_EQ(sizeof(PKT_MuHelper_ItemPicked), 44);
}

TEST(pkt_skill_status_size)
{
    ASSERT_EQ(sizeof(PKT_MuHelper_SkillStatus), 8);
}

TEST(party_member_hp_size)
{
    ASSERT_EQ(sizeof(PartyMemberHP), 14);
}

TEST(pkt_profile_save_layout)
{
    // bSlot at offset 4, szName at offset 5, cfg at offset 21
    PKT_MuHelper_ProfileSave pkt = {};
    pkt.bSlot = 3;
    ASSERT_EQ(pkt.bSlot, 3);
}

// ============================================================
//  2. CHARACTER CLASS DEFINITION TESTS
// ============================================================
TEST(class_count)
{
    ASSERT_EQ(CLASS_COUNT, 13);
}

TEST(class_enum_values)
{
    ASSERT_EQ(CLASS_DW, 0);
    ASSERT_EQ(CLASS_SM, 1);
    ASSERT_EQ(CLASS_GM, 3);
    ASSERT_EQ(CLASS_DK, 16);
    ASSERT_EQ(CLASS_BK, 17);
    ASSERT_EQ(CLASS_HK, 19);
    ASSERT_EQ(CLASS_FE, 32);
    ASSERT_EQ(CLASS_ME, 33);
    ASSERT_EQ(CLASS_HE, 35);
    ASSERT_EQ(CLASS_MG, 48);
    ASSERT_EQ(CLASS_DM, 50);
    ASSERT_EQ(CLASS_DL, 64);
    ASSERT_EQ(CLASS_LE, 66);
}

TEST(class_skill_table_not_null)
{
    const ClassSkillInfo* table = GetClassSkillTable();
    ASSERT_NOT_NULL(table);
}

TEST(class_skill_table_all_classes_present)
{
    const ClassSkillInfo* table = GetClassSkillTable();
    for (int i = 0; i < CLASS_COUNT; i++)
    {
        ASSERT_NOT_NULL(table[i].szClassName);
        ASSERT_NOT_NULL(table[i].szClassAbbr);
        ASSERT_TRUE(table[i].wPrimarySkill != SKILL_NONE);
    }
}

TEST(get_class_skill_info_valid)
{
    BYTE allClasses[] = { CLASS_DW, CLASS_SM, CLASS_GM, CLASS_DK, CLASS_BK, CLASS_HK,
                          CLASS_FE, CLASS_ME, CLASS_HE, CLASS_MG, CLASS_DM, CLASS_DL, CLASS_LE };
    for (int _i = 0; _i < (int)(sizeof(allClasses)/sizeof(allClasses[0])); _i++)
    {
        BYTE c = allClasses[_i];
        const ClassSkillInfo* ci = GetClassSkillInfo(c);
        ASSERT_NOT_NULL(ci);
        ASSERT_EQ(ci->eClass, (MuCharClass)c);
    }
}

TEST(get_class_skill_info_invalid)
{
    ASSERT_TRUE(GetClassSkillInfo(2) == nullptr);   // not a valid class ID
    ASSERT_TRUE(GetClassSkillInfo(4) == nullptr);
    ASSERT_TRUE(GetClassSkillInfo(255) == nullptr);
}

// ============================================================
//  3. CLASS-SPECIFIC SKILL TESTS
// ============================================================
TEST(dk_skills)
{
    const ClassSkillInfo* ci = GetClassSkillInfo(CLASS_DK);
    ASSERT_NOT_NULL(ci);
    ASSERT_STREQ(ci->szClassName, "Dark Knight");
    ASSERT_STREQ(ci->szClassAbbr, "DK");
    ASSERT_EQ(ci->wPrimarySkill, SKILL_DK_SLASH);
    ASSERT_EQ(ci->wSecondarySkill, SKILL_DK_UPPERCUT);
    ASSERT_EQ(ci->wAoESkill, SKILL_DK_CYCLONE);
    ASSERT_EQ(ci->wComboSkill, SKILL_NONE);
    ASSERT_EQ(ci->bComboHitsRequired, 0);
    ASSERT_EQ(ci->bIsMelee, 1);
    ASSERT_EQ(ci->bHasPartyHeal, 0);
}

TEST(bk_skills)
{
    const ClassSkillInfo* ci = GetClassSkillInfo(CLASS_BK);
    ASSERT_NOT_NULL(ci);
    ASSERT_STREQ(ci->szClassName, "Blade Knight");
    ASSERT_EQ(ci->wPrimarySkill, SKILL_BK_SWORD_DANCE);
    ASSERT_EQ(ci->wComboSkill, SKILL_BK_COMBO_ATTACK);
    ASSERT_EQ(ci->bComboHitsRequired, 3);
    ASSERT_EQ(ci->bIsMelee, 1);
}

TEST(hk_skills)
{
    const ClassSkillInfo* ci = GetClassSkillInfo(CLASS_HK);
    ASSERT_NOT_NULL(ci);
    ASSERT_STREQ(ci->szClassName, "High Knight");
    ASSERT_EQ(ci->wPrimarySkill, SKILL_HK_DRAGON_SLASH);
    ASSERT_EQ(ci->wSecondarySkill, SKILL_HK_PENETRATION);
    ASSERT_EQ(ci->wAoESkill, SKILL_HK_LIGHTNING_STRIKE);
    ASSERT_EQ(ci->wComboSkill, SKILL_BK_COMBO_ATTACK);
    ASSERT_EQ(ci->bComboHitsRequired, 3);
}

TEST(dw_skills)
{
    const ClassSkillInfo* ci = GetClassSkillInfo(CLASS_DW);
    ASSERT_NOT_NULL(ci);
    ASSERT_STREQ(ci->szClassName, "Dark Wizard");
    ASSERT_EQ(ci->wPrimarySkill, SKILL_DW_FIREBALL);
    ASSERT_EQ(ci->wSecondarySkill, SKILL_DW_POWER_WAVE);
    ASSERT_EQ(ci->wAoESkill, SKILL_DW_LIGHTNING);
    ASSERT_EQ(ci->bIsMelee, 0);
    ASSERT_EQ(ci->bCanTeleport, 1);
}

TEST(sm_skills)
{
    const ClassSkillInfo* ci = GetClassSkillInfo(CLASS_SM);
    ASSERT_NOT_NULL(ci);
    ASSERT_STREQ(ci->szClassName, "Soul Master");
    ASSERT_EQ(ci->wPrimarySkill, SKILL_SM_CHAIN_LIGHTNING);
    ASSERT_EQ(ci->wAoESkill, SKILL_SM_NOVA);
    ASSERT_EQ(ci->bCanTeleport, 1);
}

TEST(gm_skills)
{
    const ClassSkillInfo* ci = GetClassSkillInfo(CLASS_GM);
    ASSERT_NOT_NULL(ci);
    ASSERT_STREQ(ci->szClassName, "Grand Master");
    ASSERT_EQ(ci->wPrimarySkill, SKILL_GM_STORM);
    ASSERT_EQ(ci->wSecondarySkill, SKILL_GM_INFERNO);
    ASSERT_EQ(ci->wAoESkill, SKILL_GM_ICE_STORM);
}

TEST(fe_skills)
{
    const ClassSkillInfo* ci = GetClassSkillInfo(CLASS_FE);
    ASSERT_NOT_NULL(ci);
    ASSERT_STREQ(ci->szClassName, "Fairy Elf");
    ASSERT_EQ(ci->wPrimarySkill, SKILL_FE_TRIPLE_SHOT);
    ASSERT_EQ(ci->wBuffSkill1, SKILL_FE_DEFENSE_UP);
    ASSERT_EQ(ci->wBuffSkill2, SKILL_FE_ATTACK_UP);
    ASSERT_EQ(ci->wPartyHealSkill, SKILL_FE_HEAL);
    ASSERT_EQ(ci->bHasPartyHeal, 1);
    ASSERT_EQ(ci->bIsMelee, 0);
}

TEST(me_skills)
{
    const ClassSkillInfo* ci = GetClassSkillInfo(CLASS_ME);
    ASSERT_NOT_NULL(ci);
    ASSERT_STREQ(ci->szClassName, "Muse Elf");
    ASSERT_EQ(ci->wPrimarySkill, SKILL_ME_MULTI_SHOT);
    ASSERT_EQ(ci->wSecondarySkill, SKILL_ME_PENETRATION_ARROW);
    ASSERT_EQ(ci->wPartyHealSkill, SKILL_ME_CURE);
    ASSERT_EQ(ci->bHasPartyHeal, 1);
}

TEST(he_skills)
{
    const ClassSkillInfo* ci = GetClassSkillInfo(CLASS_HE);
    ASSERT_NOT_NULL(ci);
    ASSERT_STREQ(ci->szClassName, "High Elf");
    ASSERT_EQ(ci->wPrimarySkill, SKILL_HE_BOW_LASER);
    ASSERT_EQ(ci->wSecondarySkill, SKILL_HE_INFINITY_ARROW);
    ASSERT_EQ(ci->wPartyHealSkill, SKILL_HE_BLESS);
    ASSERT_EQ(ci->bHasPartyHeal, 1);
}

TEST(mg_skills)
{
    const ClassSkillInfo* ci = GetClassSkillInfo(CLASS_MG);
    ASSERT_NOT_NULL(ci);
    ASSERT_STREQ(ci->szClassName, "Magic Gladiator");
    ASSERT_EQ(ci->wPrimarySkill, SKILL_MG_POWER_SLASH);
    ASSERT_EQ(ci->wSecondarySkill, SKILL_MG_FIREBALL_SLASH);
    ASSERT_EQ(ci->bIsMelee, 1);
    ASSERT_EQ(ci->bCanTeleport, 1);
}

TEST(dm_skills)
{
    const ClassSkillInfo* ci = GetClassSkillInfo(CLASS_DM);
    ASSERT_NOT_NULL(ci);
    ASSERT_STREQ(ci->szClassName, "Duel Master");
    ASSERT_EQ(ci->wPrimarySkill, SKILL_DM_ICE_SLASH);
    ASSERT_EQ(ci->wSecondarySkill, SKILL_DM_FLAME_STRIKE);
    ASSERT_EQ(ci->wAoESkill, SKILL_DM_EARTHQUAKE);
}

TEST(dl_skills)
{
    const ClassSkillInfo* ci = GetClassSkillInfo(CLASS_DL);
    ASSERT_NOT_NULL(ci);
    ASSERT_STREQ(ci->szClassName, "Dark Lord");
    ASSERT_EQ(ci->wPrimarySkill, SKILL_DL_FIRESCREAM);
    ASSERT_EQ(ci->wSecondarySkill, SKILL_DL_CHAOTIC_DISEIER);
    ASSERT_EQ(ci->wAoESkill, SKILL_DL_ARMY_OF_DARKNESS);
    ASSERT_EQ(ci->wBuffSkill1, SKILL_DL_DARKNESS);
}

TEST(le_skills)
{
    const ClassSkillInfo* ci = GetClassSkillInfo(CLASS_LE);
    ASSERT_NOT_NULL(ci);
    ASSERT_STREQ(ci->szClassName, "Lord Emperor");
    ASSERT_EQ(ci->wPrimarySkill, SKILL_LE_FIRE_BURST);
    ASSERT_EQ(ci->wAoESkill, SKILL_DL_ARMY_OF_DARKNESS);
    ASSERT_EQ(ci->wBuffSkill1, SKILL_LE_IMPERIAL_FORT);
    ASSERT_EQ(ci->wBuffSkill2, SKILL_DL_DARKNESS);
}

// ============================================================
//  4. CLASS NAME / ABBREVIATION LOOKUP TESTS
// ============================================================
TEST(class_name_lookup)
{
    ASSERT_STREQ(GetClassName(CLASS_DW), "Dark Wizard");
    ASSERT_STREQ(GetClassName(CLASS_SM), "Soul Master");
    ASSERT_STREQ(GetClassName(CLASS_GM), "Grand Master");
    ASSERT_STREQ(GetClassName(CLASS_DK), "Dark Knight");
    ASSERT_STREQ(GetClassName(CLASS_BK), "Blade Knight");
    ASSERT_STREQ(GetClassName(CLASS_HK), "High Knight");
    ASSERT_STREQ(GetClassName(CLASS_FE), "Fairy Elf");
    ASSERT_STREQ(GetClassName(CLASS_ME), "Muse Elf");
    ASSERT_STREQ(GetClassName(CLASS_HE), "High Elf");
    ASSERT_STREQ(GetClassName(CLASS_MG), "Magic Gladiator");
    ASSERT_STREQ(GetClassName(CLASS_DM), "Duel Master");
    ASSERT_STREQ(GetClassName(CLASS_DL), "Dark Lord");
    ASSERT_STREQ(GetClassName(CLASS_LE), "Lord Emperor");
    ASSERT_STREQ(GetClassName(255), "Unknown");
}

TEST(class_abbr_lookup)
{
    ASSERT_STREQ(GetClassAbbr(CLASS_DK), "DK");
    ASSERT_STREQ(GetClassAbbr(CLASS_FE), "FE");
    ASSERT_STREQ(GetClassAbbr(CLASS_LE), "LE");
    ASSERT_STREQ(GetClassAbbr(255), "??");
}

// ============================================================
//  5. SKILL NAME LOOKUP TESTS
// ============================================================
TEST(skill_name_lookup)
{
    ASSERT_STREQ(GetSkillName(SKILL_DK_SLASH), "Slash");
    ASSERT_STREQ(GetSkillName(SKILL_BK_COMBO_ATTACK), "Combo Attack");
    ASSERT_STREQ(GetSkillName(SKILL_DW_FIREBALL), "Fireball");
    ASSERT_STREQ(GetSkillName(SKILL_FE_HEAL), "Heal");
    ASSERT_STREQ(GetSkillName(SKILL_DL_FIRESCREAM), "Firescream");
    ASSERT_STREQ(GetSkillName(SKILL_LE_FIRE_BURST), "Fire Burst");
    ASSERT_STREQ(GetSkillName(SKILL_NONE), "(none)");
    ASSERT_STREQ(GetSkillName(0x9999), "Unknown");
}

// ============================================================
//  6. COOLDOWN VALUE TESTS
// ============================================================
TEST(cooldown_values)
{
    // All classes should have valid cooldown values
    const ClassSkillInfo* table = GetClassSkillTable();
    for (int i = 0; i < CLASS_COUNT; i++)
    {
        ASSERT_TRUE(table[i].dwPrimaryCooldownMs >= 400);
        ASSERT_TRUE(table[i].dwPrimaryCooldownMs <= 5000);
        if (table[i].wSecondarySkill != SKILL_NONE)
        {
            ASSERT_TRUE(table[i].dwSecondaryCooldownMs >= 400);
            ASSERT_TRUE(table[i].dwSecondaryCooldownMs <= 5000);
        }
        if (table[i].wAoESkill != SKILL_NONE)
        {
            ASSERT_TRUE(table[i].dwAoECooldownMs >= 1000);
            ASSERT_TRUE(table[i].dwAoECooldownMs <= 5000);
        }
    }
}

// ============================================================
//  7. ELF CLASS PARTY HEAL TESTS
// ============================================================
TEST(elf_classes_have_party_heal)
{
    const ClassSkillInfo* fe = GetClassSkillInfo(CLASS_FE);
    const ClassSkillInfo* me = GetClassSkillInfo(CLASS_ME);
    const ClassSkillInfo* he = GetClassSkillInfo(CLASS_HE);

    ASSERT_TRUE(fe->bHasPartyHeal == 1);
    ASSERT_TRUE(me->bHasPartyHeal == 1);
    ASSERT_TRUE(he->bHasPartyHeal == 1);
    ASSERT_TRUE(fe->wPartyHealSkill != SKILL_NONE);
    ASSERT_TRUE(me->wPartyHealSkill != SKILL_NONE);
    ASSERT_TRUE(he->wPartyHealSkill != SKILL_NONE);
}

TEST(non_elf_classes_no_party_heal)
{
    BYTE nonElf[] = { CLASS_DK, CLASS_BK, CLASS_HK, CLASS_DW, CLASS_SM, CLASS_GM,
                      CLASS_MG, CLASS_DM, CLASS_DL, CLASS_LE };
    for (int _i = 0; _i < (int)(sizeof(nonElf)/sizeof(nonElf[0])); _i++)
    {
        const ClassSkillInfo* ci = GetClassSkillInfo(nonElf[_i]);
        ASSERT_TRUE(ci->bHasPartyHeal == 0);
    }
}

// ============================================================
//  8. COMBO SYSTEM TESTS
// ============================================================
TEST(bk_hk_have_combo)
{
    const ClassSkillInfo* bk = GetClassSkillInfo(CLASS_BK);
    const ClassSkillInfo* hk = GetClassSkillInfo(CLASS_HK);

    ASSERT_TRUE(bk->wComboSkill != SKILL_NONE);
    ASSERT_TRUE(hk->wComboSkill != SKILL_NONE);
    ASSERT_EQ(bk->bComboHitsRequired, 3);
    ASSERT_EQ(hk->bComboHitsRequired, 3);
}

TEST(non_combo_classes)
{
    BYTE noCombo[] = { CLASS_DK, CLASS_DW, CLASS_SM, CLASS_GM, CLASS_FE,
                       CLASS_ME, CLASS_HE, CLASS_MG, CLASS_DM, CLASS_DL, CLASS_LE };
    for (int _i = 0; _i < (int)(sizeof(noCombo)/sizeof(noCombo[0])); _i++)
    {
        const ClassSkillInfo* ci = GetClassSkillInfo(noCombo[_i]);
        ASSERT_TRUE(ci->wComboSkill == SKILL_NONE);
    }
}

// ============================================================
//  9. TELEPORT CAPABILITY TESTS
// ============================================================
TEST(teleport_classes)
{
    BYTE canTP[] = { CLASS_DW, CLASS_SM, CLASS_GM, CLASS_MG, CLASS_DM };
    for (int _i = 0; _i < (int)(sizeof(canTP)/sizeof(canTP[0])); _i++)
    {
        const ClassSkillInfo* ci = GetClassSkillInfo(canTP[_i]);
        ASSERT_EQ(ci->bCanTeleport, 1);
    }
}

TEST(no_teleport_classes)
{
    BYTE noTP[] = { CLASS_DK, CLASS_BK, CLASS_HK, CLASS_FE, CLASS_ME,
                    CLASS_HE, CLASS_DL, CLASS_LE };
    for (int _i = 0; _i < (int)(sizeof(noTP)/sizeof(noTP[0])); _i++)
    {
        const ClassSkillInfo* ci = GetClassSkillInfo(noTP[_i]);
        ASSERT_EQ(ci->bCanTeleport, 0);
    }
}

// ============================================================
//  10. MELEE/RANGED CLASSIFICATION TESTS
// ============================================================
TEST(melee_classes)
{
    BYTE melee[] = { CLASS_DK, CLASS_BK, CLASS_HK, CLASS_MG, CLASS_DM, CLASS_DL, CLASS_LE };
    for (int _i = 0; _i < (int)(sizeof(melee)/sizeof(melee[0])); _i++)
    {
        const ClassSkillInfo* ci = GetClassSkillInfo(melee[_i]);
        ASSERT_EQ(ci->bIsMelee, 1);
    }
}

TEST(ranged_classes)
{
    BYTE ranged[] = { CLASS_DW, CLASS_SM, CLASS_GM, CLASS_FE, CLASS_ME, CLASS_HE };
    for (int _i = 0; _i < (int)(sizeof(ranged)/sizeof(ranged[0])); _i++)
    {
        const ClassSkillInfo* ci = GetClassSkillInfo(ranged[_i]);
        ASSERT_EQ(ci->bIsMelee, 0);
    }
}

// ============================================================
//  11. CONFIG DEFAULT VALUES TEST
// ============================================================
TEST(config_defaults)
{
    MuHelperConfig cfg = {};
    ASSERT_EQ(cfg.bEnabled, 0);
    ASSERT_EQ(cfg.bCombatMode, 0);
    ASSERT_EQ(cfg.bAttackRadius, 0);
    ASSERT_EQ(sizeof(cfg.bReserved), 64);
}

// ============================================================
//  12. OPCODE / SUBCODE CONSTANTS TEST
// ============================================================
TEST(opcodes)
{
    ASSERT_EQ(MUHELPER_OPCODE_MAIN, 0x97);
    ASSERT_EQ(MUHELPER_SUB_ENABLE, 0x01);
    ASSERT_EQ(MUHELPER_SUB_CFG_SEND, 0x02);
    ASSERT_EQ(MUHELPER_SUB_CFG_ACK, 0x03);
    ASSERT_EQ(MUHELPER_SUB_CFG_REQUEST, 0x04);
    ASSERT_EQ(MUHELPER_SUB_CFG_REPLY, 0x05);
    ASSERT_EQ(MUHELPER_SUB_ITEM_PICKED, 0x06);
    ASSERT_EQ(MUHELPER_SUB_STATUS, 0x07);
    ASSERT_EQ(MUHELPER_SUB_SKILL_STATUS, 0x08);
    ASSERT_EQ(MUHELPER_SUB_PARTY_HP, 0x09);
    ASSERT_EQ(MUHELPER_SUB_PROFILE_SAVE, 0x0A);
    ASSERT_EQ(MUHELPER_SUB_PROFILE_LOAD, 0x0B);
}

// ============================================================
//  13. HELPER PROFILE STRUCT TEST
// ============================================================
TEST(helper_profile)
{
    HelperProfile p = {};
    p.bUsed = 1;
    strncpy(p.szName, "TestProfile", 15);
    ASSERT_STREQ(p.szName, "TestProfile");
    ASSERT_EQ(p.bUsed, 1);
}

// ============================================================
//  14. DL/LE BUFF TESTS (Darkness, Imperial Fort)
// ============================================================
TEST(dl_has_darkness_buff)
{
    const ClassSkillInfo* ci = GetClassSkillInfo(CLASS_DL);
    ASSERT_EQ(ci->wBuffSkill1, SKILL_DL_DARKNESS);
}

TEST(le_has_imperial_fort_and_darkness)
{
    const ClassSkillInfo* ci = GetClassSkillInfo(CLASS_LE);
    ASSERT_EQ(ci->wBuffSkill1, SKILL_LE_IMPERIAL_FORT);
    ASSERT_EQ(ci->wBuffSkill2, SKILL_DL_DARKNESS);
}

// ============================================================
//  MAIN
// ============================================================
int main()
{
    printf("========================================\n");
    printf("  MuHelper v2 — Test Suite\n");
    printf("========================================\n\n");

    // 1. Packet structure sizes
    printf("[Packet Structures]\n");
    RUN(config_size);
    RUN(pkt_enable_size);
    RUN(pkt_cfg_send_size);
    RUN(pkt_cfg_ack_size);
    RUN(pkt_cfg_request_size);
    RUN(pkt_cfg_reply_size);
    RUN(pkt_item_picked_size);
    RUN(pkt_skill_status_size);
    RUN(party_member_hp_size);
    RUN(pkt_profile_save_layout);

    // 2. Class definitions
    printf("\n[Character Classes]\n");
    RUN(class_count);
    RUN(class_enum_values);
    RUN(class_skill_table_not_null);
    RUN(class_skill_table_all_classes_present);
    RUN(get_class_skill_info_valid);
    RUN(get_class_skill_info_invalid);

    // 3. Class-specific skills
    printf("\n[Class Skills]\n");
    RUN(dk_skills);
    RUN(bk_skills);
    RUN(hk_skills);
    RUN(dw_skills);
    RUN(sm_skills);
    RUN(gm_skills);
    RUN(fe_skills);
    RUN(me_skills);
    RUN(he_skills);
    RUN(mg_skills);
    RUN(dm_skills);
    RUN(dl_skills);
    RUN(le_skills);

    // 4. Name lookups
    printf("\n[Name Lookups]\n");
    RUN(class_name_lookup);
    RUN(class_abbr_lookup);
    RUN(skill_name_lookup);

    // 5. Cooldowns
    printf("\n[Cooldown Values]\n");
    RUN(cooldown_values);

    // 6. Party heal
    printf("\n[Party Heal]\n");
    RUN(elf_classes_have_party_heal);
    RUN(non_elf_classes_no_party_heal);

    // 7. Combo system
    printf("\n[Combo System]\n");
    RUN(bk_hk_have_combo);
    RUN(non_combo_classes);

    // 8. Teleport
    printf("\n[Teleport]\n");
    RUN(teleport_classes);
    RUN(no_teleport_classes);

    // 9. Melee/Ranged
    printf("\n[Melee/Ranged]\n");
    RUN(melee_classes);
    RUN(ranged_classes);

    // 10. Config
    printf("\n[Config]\n");
    RUN(config_defaults);
    RUN(opcodes);
    RUN(helper_profile);

    // 11. DL/LE buffs
    printf("\n[DL/LE Buffs]\n");
    RUN(dl_has_darkness_buff);
    RUN(le_has_imperial_fort_and_darkness);

    printf("\n========================================\n");
    printf("  Results: %d passed, %d failed\n", g_passed, g_failed);
    printf("========================================\n");

    return g_failed > 0 ? 1 : 0;
}
