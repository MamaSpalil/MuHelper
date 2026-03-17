#pragma once
// ============================================================
//  MuHelper — Network Data Structures
//  Ported from MuMain Season 5.2 (WSclient.h)
//  Compatible with official MU Online MuHelper protocol
// ============================================================
//
//  MUHELPER_NET_DATA matches PRECEIVE_MUHELPER_DATA from the
//  reference MuMain source (sven-n/MuMain).  The binary layout
//  is identical so both client and server can exchange configs.
// ============================================================

#include <windows.h>

#pragma pack(push, 1)

// ── MuHelper configuration blob sent over the network ────────
// Total expected size: 245 bytes
// (1+1+1+2 +5*2 +3*2 +2*1 +3*1 +1 +36 +12*15)
// Matches PRECEIVE_MUHELPER_DATA from MuMain Season 5.2
typedef struct _MUHELPER_NET_DATA
{
    BYTE DataStartMarker;                    // Index: 4 (after pkt header)
                                             // Value: 0x00 (reserved, must be zero)

    // Bit-packed pickup flags                  Index: 5
    BYTE _unused5_lo   : 3;
    BYTE JewelOrGem    : 1;
    BYTE SetItem       : 1;
    BYTE ExcellentItem : 1;
    BYTE Zen           : 1;
    BYTE AddExtraItem  : 1;

    // Range settings                           Index: 6
    BYTE HuntingRange  : 4;
    BYTE ObtainRange   : 4;

    WORD DistanceMin;                        // Index: 7-8

    // Attack skills                            Index: 9-18
    WORD BasicSkill1;                        // primary skill
    WORD ActivationSkill1;                   // 2nd skill
    WORD DelayMinSkill1;                     // interval for 2nd
    WORD ActivationSkill2;                   // 3rd skill
    WORD DelayMinSkill2;                     // interval for 3rd

    // Buff skills                              Index: 19-26
    WORD CastingBuffMin;
    WORD BuffSkill0NumberID;
    WORD BuffSkill1NumberID;
    WORD BuffSkill2NumberID;

    // HP thresholds (4-bit each)               Index: 27
    BYTE HPStatusAutoPotion : 4;
    BYTE HPStatusAutoHeal   : 4;

    // HP status continued                      Index: 28
    BYTE HPStatusOfPartyMembers : 4;
    BYTE HPStatusDrainLife      : 4;

    // Feature flags (bit-packed)               Index: 29
    BYTE AutoPotion            : 1;
    BYTE AutoHeal              : 1;
    BYTE DrainLife             : 1;
    BYTE LongDistanceAttack    : 1;
    BYTE OriginalPosition      : 1;
    BYTE Combo                 : 1;
    BYTE Party                 : 1;
    BYTE PreferenceOfPartyHeal : 1;

    // More flags                               Index: 30
    BYTE BuffDurationforAllPartyMembers : 1;
    BYTE UseDarkSpirits     : 1;
    BYTE BuffDuration       : 1;
    BYTE Skill1Delay        : 1;
    BYTE Skill1Con          : 1;
    BYTE Skill1PreCon       : 1;
    BYTE Skill1SubCon       : 2;

    // Skill 2 conditions + misc                Index: 31
    BYTE Skill2Delay        : 1;
    BYTE Skill2Con          : 1;
    BYTE Skill2PreCon       : 1;
    BYTE Skill2SubCon       : 2;
    BYTE RepairItem         : 1;
    BYTE PickAllNearItems   : 1;
    BYTE PickSelectedItems  : 1;

    BYTE PetAttack;                          // Index: 32

    BYTE _UnusedPadding[36];                 // Index: 33-68

    char ExtraItems[12][15];                 // Index: 69-248

} MUHELPER_NET_DATA;

// ── MuHelper status notification ─────────────────────────────
typedef struct _MUHELPER_NET_STATUS
{
    DWORD ConsumeMoney;     // zen spent on helper
    DWORD Money;            // current zen balance
    DWORD Pause;            // 0=running, 1=paused
} MUHELPER_NET_STATUS;

#pragma pack(pop)
