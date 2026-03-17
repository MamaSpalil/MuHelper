#pragma once
#include <windows.h>

// ============================================================
//  MuHelper v2 — Shared Packets
//  GameServer 1.00.18 / Client 1.02.11
// ============================================================

#pragma pack(push, 1)

#define MUHELPER_OPCODE_MAIN        0x97

#define MUHELPER_SUB_ENABLE         0x01
#define MUHELPER_SUB_CFG_SEND       0x02
#define MUHELPER_SUB_CFG_ACK        0x03
#define MUHELPER_SUB_CFG_REQUEST    0x04
#define MUHELPER_SUB_CFG_REPLY      0x05
#define MUHELPER_SUB_ITEM_PICKED    0x06
#define MUHELPER_SUB_STATUS         0x07
#define MUHELPER_SUB_SKILL_STATUS   0x08
#define MUHELPER_SUB_PARTY_HP       0x09
#define MUHELPER_SUB_PROFILE_SAVE   0x0A
#define MUHELPER_SUB_PROFILE_LOAD   0x0B

// Pickup flags
#define PICKUP_FLAG_NONE            0x0000
#define PICKUP_FLAG_ZEN             0x0001
#define PICKUP_FLAG_JEWELS          0x0002
#define PICKUP_FLAG_EXCELLENT       0x0004
#define PICKUP_FLAG_ANCIENT         0x0008
#define PICKUP_FLAG_SET             0x0010
#define PICKUP_FLAG_SOCKETS         0x0020
#define PICKUP_FLAG_WINGS           0x0040
#define PICKUP_FLAG_PET             0x0080
#define PICKUP_FLAG_MISC            0x0100
#define PICKUP_FLAG_ALL             0xFFFF

// Combat flags
#define COMBAT_MODE_ATTACK          0x01
#define COMBAT_MODE_SKILL           0x02
#define COMBAT_MODE_COMBO           0x04
#define COMBAT_MODE_AOE             0x08

// Potion flags
#define POTION_FLAG_HP              0x01
#define POTION_FLAG_MP              0x02
#define POTION_FLAG_SHIELD          0x04
#define POTION_FLAG_ANTIDOTE        0x08

#define BUFF_SLOT_NONE              0xFF
#define SKILL_NONE                  0xFFFF
#define MUHELPER_MAX_RADIUS         12
#define MUHELPER_DEFAULT_RADIUS     5
#define MUHELPER_MAX_PROFILES       5

// ============================================================
//  Character Class IDs (server Class byte, MU Online protocol)
//  Encoding: (class_line << 4) | evolution
//    class_line: 0=Wizard, 1=Knight, 2=Elf, 3=MG, 4=DL
//    evolution:  0=base, 1=1st, 3=2nd (MG/DL: 0=base, 2=evo)
// ============================================================
enum MuCharClass
{
    CLASS_DW  = 0,    // Dark Wizard (base)
    CLASS_SM  = 1,    // Soul Master (1st evolution)
    CLASS_GM  = 3,    // Grand Master (2nd evolution)
    CLASS_DK  = 16,   // Dark Knight (base)
    CLASS_BK  = 17,   // Blade Knight (1st evolution)
    CLASS_HK  = 19,   // High Knight (2nd evolution)
    CLASS_FE  = 32,   // Fairy Elf (base)
    CLASS_ME  = 33,   // Muse Elf (1st evolution)
    CLASS_HE  = 35,   // High Elf (2nd evolution)
    CLASS_MG  = 48,   // Magic Gladiator (base)
    CLASS_DM  = 50,   // Duel Master (MG evolution)
    CLASS_DL  = 64,   // Dark Lord (base)
    CLASS_LE  = 66,   // Lord Emperor (DL evolution)
    CLASS_COUNT= 13   // total number of classes (not max ID)
};

// ============================================================
//  Skill IDs — unique per class ability
// ============================================================
enum MuSkillId
{
    // -- Dark Knight line --
    SKILL_DK_SLASH              = 0x0001,
    SKILL_DK_UPPERCUT           = 0x0002,
    SKILL_DK_CYCLONE            = 0x0003,
    SKILL_DK_LUNGE              = 0x0004,
    SKILL_BK_SWORD_DANCE        = 0x0010,
    SKILL_BK_COMBO_ATTACK       = 0x0011,
    SKILL_BK_IMPALE             = 0x0012,
    SKILL_HK_DRAGON_SLASH       = 0x0020,
    SKILL_HK_PENETRATION        = 0x0021,
    SKILL_HK_LIGHTNING_STRIKE   = 0x0022,
    // -- Dark Wizard line --
    SKILL_DW_FIREBALL           = 0x0101,
    SKILL_DW_POWER_WAVE         = 0x0102,
    SKILL_DW_LIGHTNING           = 0x0103,
    SKILL_DW_ICE_ARROW          = 0x0104,
    SKILL_DW_TELEPORT           = 0x0105,
    SKILL_SM_CHAIN_LIGHTNING    = 0x0110,
    SKILL_SM_NOVA               = 0x0111,
    SKILL_SM_DECAY              = 0x0112,
    SKILL_GM_STORM              = 0x0120,
    SKILL_GM_INFERNO            = 0x0121,
    SKILL_GM_ICE_STORM          = 0x0122,
    // -- Fairy Elf line --
    SKILL_FE_TRIPLE_SHOT        = 0x0201,
    SKILL_FE_HEAL               = 0x0202,
    SKILL_FE_DEFENSE_UP         = 0x0203,
    SKILL_FE_ATTACK_UP          = 0x0204,
    SKILL_FE_SUMMON_MONSTER     = 0x0205,
    SKILL_ME_MULTI_SHOT         = 0x0210,
    SKILL_ME_PENETRATION_ARROW  = 0x0211,
    SKILL_ME_CURE               = 0x0212,
    SKILL_HE_BOW_LASER          = 0x0220,
    SKILL_HE_INFINITY_ARROW     = 0x0221,
    SKILL_HE_BLESS              = 0x0222,
    // -- Magic Gladiator line --
    SKILL_MG_POWER_SLASH        = 0x0301,
    SKILL_MG_FIREBALL_SLASH     = 0x0302,
    SKILL_MG_TELEPORT           = 0x0303,
    SKILL_MG_SWORD_POWER        = 0x0304,
    SKILL_DM_ICE_SLASH          = 0x0310,
    SKILL_DM_FLAME_STRIKE       = 0x0311,
    SKILL_DM_EARTHQUAKE         = 0x0312,
    // -- Dark Lord line --
    SKILL_DL_FIRESCREAM         = 0x0401,
    SKILL_DL_CHAOTIC_DISEIER    = 0x0402,
    SKILL_DL_DARKNESS           = 0x0403,
    SKILL_DL_ARMY_OF_DARKNESS   = 0x0404,
    SKILL_LE_IMPERIAL_FORT      = 0x0410,
    SKILL_LE_FIRE_BURST         = 0x0411,
    SKILL_LE_DARK_HORSE_ATTACK  = 0x0412,
};

// ============================================================
//  Per-class skill table — defines primary, secondary, AoE,
//  buff, combo, and party heal for each class
// ============================================================
struct ClassSkillInfo
{
    MuCharClass eClass;
    const char* szClassName;
    const char* szClassAbbr;
    WORD  wPrimarySkill;        // main single-target attack
    WORD  wSecondarySkill;      // secondary/alternate attack
    WORD  wAoESkill;            // area-of-effect skill
    WORD  wBuffSkill1;          // self/party buff 1
    WORD  wBuffSkill2;          // self/party buff 2
    WORD  wComboSkill;          // combo finisher (0xFFFF = none)
    WORD  wPartyHealSkill;      // party heal (0xFFFF = none)
    BYTE  bComboHitsRequired;   // hits before combo fires (0 = no combo)
    BYTE  bHasPartyHeal;        // 1 if can heal party members
    BYTE  bIsMelee;             // 1 = melee, 0 = ranged
    BYTE  bCanTeleport;         // 1 if class has teleport
    DWORD dwPrimaryCooldownMs;
    DWORD dwSecondaryCooldownMs;
    DWORD dwAoECooldownMs;
};

inline const ClassSkillInfo* GetClassSkillTable()
{
    static const ClassSkillInfo table[CLASS_COUNT] = {
        // Dark Wizard (DW) — ranged, can teleport
        { CLASS_DW, "Dark Wizard",     "DW",
          SKILL_DW_FIREBALL,    SKILL_DW_POWER_WAVE,    SKILL_DW_LIGHTNING,
          SKILL_NONE,           SKILL_NONE,             SKILL_NONE,
          SKILL_NONE,           0, 0, 0, 1,
          700, 900, 2500 },
        // Soul Master (SM) — ranged, can teleport
        { CLASS_SM, "Soul Master",     "SM",
          SKILL_SM_CHAIN_LIGHTNING, SKILL_DW_ICE_ARROW, SKILL_SM_NOVA,
          SKILL_NONE,           SKILL_NONE,             SKILL_NONE,
          SKILL_NONE,           0, 0, 0, 1,
          700, 800, 3000 },
        // Grand Master (GM) — ranged, can teleport
        { CLASS_GM, "Grand Master",    "GM",
          SKILL_GM_STORM,       SKILL_GM_INFERNO,       SKILL_GM_ICE_STORM,
          SKILL_NONE,           SKILL_NONE,             SKILL_NONE,
          SKILL_NONE,           0, 0, 0, 1,
          700, 800, 2500 },
        // Dark Knight (DK) — melee, no combo yet
        { CLASS_DK, "Dark Knight",     "DK",
          SKILL_DK_SLASH,       SKILL_DK_UPPERCUT,      SKILL_DK_CYCLONE,
          SKILL_NONE,           SKILL_NONE,             SKILL_NONE,
          SKILL_NONE,           0, 0, 1, 0,
          600, 800, 2000 },
        // Blade Knight (BK) — melee, combo at 3 hits
        { CLASS_BK, "Blade Knight",    "BK",
          SKILL_BK_SWORD_DANCE, SKILL_BK_IMPALE,        SKILL_DK_CYCLONE,
          SKILL_NONE,           SKILL_NONE,             SKILL_BK_COMBO_ATTACK,
          SKILL_NONE,           3, 0, 1, 0,
          600, 800, 2000 },
        // High Knight (HK) — melee, combo at 3 hits
        { CLASS_HK, "High Knight",     "HK",
          SKILL_HK_DRAGON_SLASH,SKILL_HK_PENETRATION,   SKILL_HK_LIGHTNING_STRIKE,
          SKILL_NONE,           SKILL_NONE,             SKILL_BK_COMBO_ATTACK,
          SKILL_NONE,           3, 0, 1, 0,
          600, 700, 1800 },
        // Fairy Elf (FE) — ranged, has party heal & buffs
        { CLASS_FE, "Fairy Elf",       "FE",
          SKILL_FE_TRIPLE_SHOT, SKILL_FE_SUMMON_MONSTER,SKILL_NONE,
          SKILL_FE_DEFENSE_UP,  SKILL_FE_ATTACK_UP,     SKILL_NONE,
          SKILL_FE_HEAL,        0, 1, 0, 0,
          600, 2000, 0 },
        // Muse Elf (ME) — ranged, has party heal & buffs
        { CLASS_ME, "Muse Elf",        "ME",
          SKILL_ME_MULTI_SHOT,  SKILL_ME_PENETRATION_ARROW, SKILL_NONE,
          SKILL_FE_DEFENSE_UP,  SKILL_FE_ATTACK_UP,     SKILL_NONE,
          SKILL_ME_CURE,        0, 1, 0, 0,
          600, 1500, 0 },
        // High Elf (HE) — ranged, has party heal & buffs
        { CLASS_HE, "High Elf",        "HE",
          SKILL_HE_BOW_LASER,   SKILL_HE_INFINITY_ARROW,SKILL_NONE,
          SKILL_FE_DEFENSE_UP,  SKILL_FE_ATTACK_UP,     SKILL_NONE,
          SKILL_HE_BLESS,       0, 1, 0, 0,
          500, 1200, 0 },
        // Magic Gladiator (MG) — hybrid melee/ranged, can teleport
        { CLASS_MG, "Magic Gladiator", "MG",
          SKILL_MG_POWER_SLASH, SKILL_MG_FIREBALL_SLASH,SKILL_MG_SWORD_POWER,
          SKILL_NONE,           SKILL_NONE,             SKILL_NONE,
          SKILL_NONE,           0, 0, 1, 1,
          600, 800, 2500 },
        // Duel Master (DM) — MG evolution, hybrid
        { CLASS_DM, "Duel Master",     "DM",
          SKILL_DM_ICE_SLASH,   SKILL_DM_FLAME_STRIKE,  SKILL_DM_EARTHQUAKE,
          SKILL_NONE,           SKILL_NONE,             SKILL_NONE,
          SKILL_NONE,           0, 0, 1, 1,
          600, 700, 2200 },
        // Dark Lord (DL) — melee, summon buffs
        { CLASS_DL, "Dark Lord",       "DL",
          SKILL_DL_FIRESCREAM,  SKILL_DL_CHAOTIC_DISEIER,SKILL_DL_ARMY_OF_DARKNESS,
          SKILL_DL_DARKNESS,    SKILL_NONE,             SKILL_NONE,
          SKILL_NONE,           0, 0, 1, 0,
          700, 900, 3000 },
        // Lord Emperor (LE) — DL evolution
        { CLASS_LE, "Lord Emperor",    "LE",
          SKILL_LE_FIRE_BURST,  SKILL_LE_DARK_HORSE_ATTACK, SKILL_DL_ARMY_OF_DARKNESS,
          SKILL_LE_IMPERIAL_FORT,SKILL_DL_DARKNESS,     SKILL_NONE,
          SKILL_NONE,           0, 0, 1, 0,
          600, 800, 2500 },
    };
    return table;
}

inline const ClassSkillInfo* GetClassSkillInfo(BYTE classId)
{
    const ClassSkillInfo* table = GetClassSkillTable();
    for (int i = 0; i < CLASS_COUNT; i++)
        if (table[i].eClass == (MuCharClass)classId) return &table[i];
    return nullptr;
}

inline const char* GetClassName(BYTE classId)
{
    const ClassSkillInfo* info = GetClassSkillInfo(classId);
    return info ? info->szClassName : "Unknown";
}

inline const char* GetClassAbbr(BYTE classId)
{
    const ClassSkillInfo* info = GetClassSkillInfo(classId);
    return info ? info->szClassAbbr : "??";
}

// ============================================================
//  Skill name lookup
// ============================================================
struct SkillNameEntry { WORD wSkillId; const char* szName; };

inline const char* GetSkillName(WORD skillId)
{
    static const SkillNameEntry names[] = {
        { SKILL_DK_SLASH,              "Slash" },
        { SKILL_DK_UPPERCUT,           "Uppercut" },
        { SKILL_DK_CYCLONE,            "Cyclone" },
        { SKILL_DK_LUNGE,              "Lunge" },
        { SKILL_BK_SWORD_DANCE,        "Sword Dance" },
        { SKILL_BK_COMBO_ATTACK,       "Combo Attack" },
        { SKILL_BK_IMPALE,             "Impale" },
        { SKILL_HK_DRAGON_SLASH,       "Dragon Slash" },
        { SKILL_HK_PENETRATION,        "Penetration" },
        { SKILL_HK_LIGHTNING_STRIKE,   "Lightning Strike" },
        { SKILL_DW_FIREBALL,           "Fireball" },
        { SKILL_DW_POWER_WAVE,         "Power Wave" },
        { SKILL_DW_LIGHTNING,          "Lightning" },
        { SKILL_DW_ICE_ARROW,          "Ice Arrow" },
        { SKILL_DW_TELEPORT,           "Teleport" },
        { SKILL_SM_CHAIN_LIGHTNING,    "Chain Lightning" },
        { SKILL_SM_NOVA,               "Nova" },
        { SKILL_SM_DECAY,              "Decay" },
        { SKILL_GM_STORM,              "Storm" },
        { SKILL_GM_INFERNO,            "Inferno" },
        { SKILL_GM_ICE_STORM,          "Ice Storm" },
        { SKILL_FE_TRIPLE_SHOT,        "Triple Shot" },
        { SKILL_FE_HEAL,               "Heal" },
        { SKILL_FE_DEFENSE_UP,         "Defense Up" },
        { SKILL_FE_ATTACK_UP,          "Attack Up" },
        { SKILL_FE_SUMMON_MONSTER,     "Summon Monster" },
        { SKILL_ME_MULTI_SHOT,         "Multi-Shot" },
        { SKILL_ME_PENETRATION_ARROW,  "Penetration Arrow" },
        { SKILL_ME_CURE,               "Cure" },
        { SKILL_HE_BOW_LASER,          "Bow Laser" },
        { SKILL_HE_INFINITY_ARROW,     "Infinity Arrow" },
        { SKILL_HE_BLESS,              "Bless" },
        { SKILL_MG_POWER_SLASH,        "Power Slash" },
        { SKILL_MG_FIREBALL_SLASH,     "Fireball Slash" },
        { SKILL_MG_TELEPORT,           "Teleport" },
        { SKILL_MG_SWORD_POWER,        "Sword Power" },
        { SKILL_DM_ICE_SLASH,          "Ice Slash" },
        { SKILL_DM_FLAME_STRIKE,       "Flame Strike" },
        { SKILL_DM_EARTHQUAKE,         "Earthquake" },
        { SKILL_DL_FIRESCREAM,         "Firescream" },
        { SKILL_DL_CHAOTIC_DISEIER,    "Chaotic Diseier" },
        { SKILL_DL_DARKNESS,           "Darkness" },
        { SKILL_DL_ARMY_OF_DARKNESS,   "Army of Darkness" },
        { SKILL_LE_IMPERIAL_FORT,      "Imperial Fort" },
        { SKILL_LE_FIRE_BURST,         "Fire Burst" },
        { SKILL_LE_DARK_HORSE_ATTACK,  "Dark Horse Attack" },
        { SKILL_NONE,                  "(none)" },
    };
    for (int i = 0; i < (int)(sizeof(names)/sizeof(names[0])); i++)
        if (names[i].wSkillId == skillId) return names[i].szName;
    return "Unknown";
}

// ============================================================
//  Config struct (96 bytes)
// ============================================================
struct MuHelperConfig
{
    BYTE  bEnabled;
    BYTE  bCombatMode;
    BYTE  bAttackSkillSlot;
    BYTE  bComboSkillSlot;
    BYTE  bAttackRadius;
    BYTE  bPickupRadius;
    WORD  wPickupFlags;
    DWORD dwCustomItemFilter;
    BYTE  bPotionFlags;
    BYTE  bHpThreshold;
    BYTE  bMpThreshold;
    BYTE  bShieldThreshold;
    BYTE  bSelfBuff;
    BYTE  bBuffSkillSlot;
    BYTE  bBuffSkillSlot2;
    BYTE  bUsePartyHeal;
    BYTE  bAutoRepair;
    BYTE  bFollowPartyLeader;
    BYTE  bStopOnLevelUp;
    BYTE  bStopOnDrop;
    BYTE  bRotationSlot1;
    BYTE  bRotationSlot2;
    BYTE  bRotationSlot3;
    BYTE  bRotationInterval;
    BYTE  bShowStatOverlay;
    BYTE  bOverlayX;
    BYTE  bOverlayY;
    BYTE  bAvoidPvP;
    BYTE  bReserved[64];
};
static_assert(sizeof(MuHelperConfig) == 96, "MuHelperConfig size");

// ============================================================
//  Profile (shared between server and client)
// ============================================================
struct HelperProfile
{
    char           szName[16];
    MuHelperConfig cfg;
    BYTE           bUsed;
};

struct PKT_MuHelper_Enable       { BYTE h,sz,op,sub; BYTE bEnable; BYTE pad[3]; };
struct PKT_MuHelper_CfgSend      { BYTE h,sz,op,sub; MuHelperConfig cfg; };
struct PKT_MuHelper_CfgAck       { BYTE h,sz,op,sub; BYTE bResult; BYTE pad[3]; };
struct PKT_MuHelper_CfgRequest   { BYTE h,sz,op,sub; };
struct PKT_MuHelper_CfgReply     { BYTE h,sz,op,sub; MuHelperConfig cfg; };

struct PKT_MuHelper_ItemPicked
{
    BYTE h,sz,op,sub;
    WORD  wItemType;
    BYTE  bItemLevel;
    BYTE  bItemOpts;    // bit0=exc bit1=anc bit2=sock
    DWORD dwZenAmount;
    char  szItemName[32];
};

struct PKT_MuHelper_Status
{
    BYTE  h,sz,op,sub;
    BYTE  bRunning;
    DWORD dwZenPickup;
    WORD  wItemPickup;
    WORD  wKillCount;
    DWORD dwExpGained;
    DWORD dwExpPerHour;
    WORD  wKillsPerHour;
    WORD  wSessionMinutes;
};

struct PKT_MuHelper_SkillStatus  { BYTE h,sz,op,sub; BYTE bSlot; WORD wCooldownMs; BYTE pad; };

struct PartyMemberHP { char szName[11]; BYTE bHpPct; BYTE bMpPct; BYTE bClass; };
struct PKT_MuHelper_PartyHP      { BYTE h,sz,op,sub; BYTE bCount; PartyMemberHP m[5]; };

struct PKT_MuHelper_ProfileSave  { BYTE h,sz,op,sub; BYTE bSlot; char szName[16]; MuHelperConfig cfg; };
struct PKT_MuHelper_ProfileLoad  { BYTE h,sz,op,sub; BYTE bSlot; };

#pragma pack(pop)
