#pragma once
#include <windows.h>

// ============================================================
//  MuHelper v2 — Shared Packets
//  GameServer 1.00.19 / Client 1.02.19
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
#define MUHELPER_MAX_RADIUS         12
#define MUHELPER_DEFAULT_RADIUS     5
#define MUHELPER_MAX_PROFILES       5

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
    BYTE  bReserved[68];
};
static_assert(sizeof(MuHelperConfig) == 96, "MuHelperConfig size");

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
