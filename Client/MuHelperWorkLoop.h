#pragma once
// ============================================================
//  MuHelper — Work Loop  (ported from MuMain Season 5.2)
//  Adapted for Season 3 Ep 1 (main.exe 1.02.19)
//  VS2010-compatible (no constexpr, no enum class, no C++14)
// ============================================================
//
//  This module implements the client-side automation logic:
//    - Attack (skill selection, combo, target management)
//    - Buff   (self / party, interval-based recast)
//    - Heal   (self / party, potion consumption, drain life)
//    - Pickup (item filtering, walk-to-item, zen/jewels/etc.)
//    - Move   (regroup to original position)
//    - Repair (auto-repair when durability low)
//    - Pet    (Dark Spirit / Raven activation)
//
//  Ported from https://github.com/sven-n/MuMain.git
//  (src/source/MUHelper/MuHelper.cpp)
// ============================================================

#include <windows.h>
#include "../Shared/MuHelperPackets.h"
#include "../Shared/MuHelperNetData.h"

// ── Skill activation conditions (from MuMain MuHelperData.h) ─
// VS2010: plain integer constants instead of enum class
namespace SkillCond
{
    static const DWORD ALWAYS               = 0x00000000;
    static const DWORD ON_TIMER             = 0x00000001;
    static const DWORD ON_CONDITION         = 0x00000002;
    static const DWORD ON_MOBS_NEARBY       = 0x00000004;
    static const DWORD ON_MOBS_ATTACKING    = 0x00000008;
    static const DWORD ON_MORE_THAN_TWO     = 0x00000010;
    static const DWORD ON_MORE_THAN_THREE   = 0x00000020;
    static const DWORD ON_MORE_THAN_FOUR    = 0x00000040;
    static const DWORD ON_MORE_THAN_FIVE    = 0x00000080;
}

// ── Pet attack modes ─────────────────────────────────────────
namespace PetMode
{
    static const BYTE CEASE    = 0x00;
    static const BYTE AUTO     = 0x01;
    static const BYTE TOGETHER = 0x02;
}

// ── Work-loop configuration (adapted from MuMain ConfigData) ─
// VS2010 compatible — no NSDMI, no =default, no std::array
struct MuHelperWorkConfig
{
    int  iHuntingRange;
    int  iObtainingRange;
    bool bLongRangeCounterAttack;
    bool bReturnToOriginalPosition;
    int  iMaxSecondsAway;

    // 3 attack skills + conditions + intervals
    DWORD aiSkill[3];
    DWORD aiSkillCondition[3];
    DWORD aiSkillInterval[3];
    bool  bUseCombo;

    // 3 buff skills
    DWORD aiBuff[3];
    bool  bBuffDuration;
    bool  bBuffDurationParty;
    int   iBuffCastInterval;

    // Healing
    bool bAutoHeal;
    int  iHealThreshold;
    bool bSupportParty;
    bool bAutoHealParty;
    int  iHealPartyThreshold;

    // Potions
    bool bUseHealPotion;
    int  iPotionThreshold;

    // Drain life / Dark Raven
    bool bUseDrainLife;
    bool bUseDarkRaven;
    int  iDarkRavenMode;

    // Repair
    bool bRepairItem;

    // Item pickup
    bool bPickAllItems;
    bool bPickSelectItems;
    bool bPickJewel;
    bool bPickZen;
    bool bPickAncient;
    bool bPickExcellent;
    bool bPickExtraItems;
};

// ── Initialize config to defaults ────────────────────────────
inline void MuHelperWorkConfig_Init(MuHelperWorkConfig* cfg)
{
    memset(cfg, 0, sizeof(MuHelperWorkConfig));
    cfg->iHuntingRange   = 4;
    cfg->iObtainingRange = 4;
    cfg->iMaxSecondsAway = 30;
    cfg->iHealThreshold  = 50;
    cfg->iPotionThreshold= 40;
    cfg->iHealPartyThreshold = 60;
}

// ── Serialization: MuHelperWorkConfig ↔ MUHELPER_NET_DATA ────
void MuHelperWorkConfig_ToNet(const MuHelperWorkConfig& cfg,
                              MUHELPER_NET_DATA& net);
void MuHelperWorkConfig_FromNet(const MUHELPER_NET_DATA& net,
                                MuHelperWorkConfig& cfg);

// ============================================================
//  CMuHelperWorkLoop — client-side automation engine
//  Driven by a Windows timer (~200 ms / 5 ticks per second)
// ============================================================
class CMuHelperWorkLoop
{
public:
    static CMuHelperWorkLoop& Instance();

    void Init();

    // Timer callback (set by SetTimer in dllmain)
    static void CALLBACK TimerProc(HWND hwnd, UINT uMsg,
                                   UINT_PTR idEvent, DWORD dwTime);

    // Control
    void Start();
    void Stop();
    void Toggle();
    bool IsActive() const { return m_bActive; }

    // Config
    void         SetConfig(const MuHelperWorkConfig& cfg) { m_config = cfg; }
    const MuHelperWorkConfig& GetConfig() const { return m_config; }

    // Target / item management (called from game hooks)
    void AddTarget(int iTargetId, bool bIsAttacking);
    void DeleteTarget(int iTargetId);
    void DeleteAllTargets();
    void AddItem(int iItemId, int x, int y);
    void DeleteItem(int iItemId);

    // Cost tracking (zen spent on repairs/potions)
    void AddCost(int c) { m_iTotalCost += c; }
    int  GetTotalCost() const { return m_iTotalCost; }

private:
    CMuHelperWorkLoop();

    // Main work sequence
    void WorkLoop();
    void Work();

    // Sub-steps (return 1 = OK, 0 = busy/retry)
    int ActivatePet();
    int Buff();
    int RecoverHealth();
    int Heal();
    int DrainLife();
    int ConsumePotion();
    int Attack();
    int RepairEquipments();
    int Regroup();
    int ObtainItem();

    // Attack helpers
    DWORD SelectAttackSkill();
    int   SimulateAttack(DWORD skill);
    int   SimulateComboAttack();

    // Target helpers
    int  GetNearestTarget();
    int  GetFarthestAttackingTarget();
    void CleanupTargets();

    // Item helpers
    int  SelectItemToObtain();
    bool ShouldObtainItem(int iItemId);

    // Distance helpers
    int ComputeDistanceByRange(int range);
    int ComputeDistanceBetween(int x1, int y1, int x2, int y2);

    // State
    MuHelperWorkConfig m_config;
    bool    m_bActive;
    int     m_iCurrentTarget;
    int     m_iCurrentItem;
    DWORD   m_dwCurrentSkill;
    int     m_iComboState;
    int     m_iCurrentBuffIndex;
    int     m_iCurrentBuffPartyIndex;
    int     m_iCurrentHealPartyIndex;
    int     m_iHuntingDistance;
    int     m_iObtainingDistance;
    int     m_iLoopCounter;
    int     m_iSecondsElapsed;
    int     m_iSecondsAway;
    int     m_iTotalCost;
    bool    m_bTimerActivatedBuffOngoing;
    bool    m_bPetActivated;
    int     m_posOrigX;
    int     m_posOrigY;

    // Simple target/item sets (VS2010 compatible — no std::set)
    static const int MAX_TRACKED = 64;
    int  m_targets[MAX_TRACKED];
    bool m_targetsAttacking[MAX_TRACKED];
    int  m_nTargets;

    int  m_items[MAX_TRACKED];
    int  m_itemX[MAX_TRACKED];
    int  m_itemY[MAX_TRACKED];
    int  m_nItems;

    CRITICAL_SECTION m_csTargets;
    CRITICAL_SECTION m_csItems;
};
