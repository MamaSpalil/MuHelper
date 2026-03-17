#pragma once
// ============================================================
//  MuHelper v2 — Server Manager
//  GameServer 1.00.18  |  GgSrvDll.dll verified integration
// ============================================================
#include "../Shared/MuHelperPackets.h"
#include "GgSrvDll_Interface.h"
#include <unordered_map>
#include <array>
#include <deque>
#include <string>

// ── Tuning ───────────────────────────────────────────────────
static const int  MH_TICK_MS           = 500;
static const int  MH_ATTACK_DELAY_MS   = 600;
static const int  MH_PICKUP_DELAY_MS   = 400;
static const int  MH_BUFF_INTERVAL_MS  = 30000;
static const int  MH_REPAIR_INTERVAL_MS= 60000;
static const int  MH_PARTY_SEND_MS     = 2000;
static const int  MH_POTION_COOLDOWN_MS= 1500;
static const int  MH_MAX_RADIUS        = 12;
static const int  MH_DEFAULT_RADIUS    = 5;
static const int  MH_EXP_SAMPLE_SEC    = 60;   // rate averaging window
static const int  MH_PARTY_HEAL_HP_PCT = 60;   // heal party members below this %

// RAII wrapper for CRITICAL_SECTION (replaces std::lock_guard)
class CSGuard
{
    CRITICAL_SECTION& m_cs;
    CSGuard(const CSGuard&);
    CSGuard& operator=(const CSGuard&);
public:
    CSGuard(CRITICAL_SECTION& cs) : m_cs(cs) { EnterCriticalSection(&m_cs); }
    ~CSGuard() { LeaveCriticalSection(&m_cs); }
};

// ── Per-session state ────────────────────────────────────────
struct HelperSkillCD
{
    BYTE  bSlot;
    DWORD dwStartMs;
    DWORD dwDurationMs;
};

struct MuHelperSession
{
    MuHelperConfig   cfg;
    bool             bActive;
    int              nTarget;

    // Character class (resolved on char load)
    BYTE  bClass;

    // Skill rotation state
    BYTE bRotationStep;
    BYTE bAttackCount;

    // Combo state (for BK/BM combo system)
    BYTE bComboHitCount;

    // Timers (GetTickCount values)
    DWORD tLastAttack, tLastPickup, tLastBuff, tLastRepair, tLastPotion, tLastParty;
    DWORD tLastPartyHeal;

    // Stats
    DWORD dwZenPickup;
    WORD  wItemPickup;
    WORD  wKillCount;
    DWORD dwExpGained;
    DWORD dwStartTimeSec;

    // Rate tracking (circular buffers)
    std::deque<std::pair<DWORD,WORD> >  killSamples;  // {tick,count}
    std::deque<std::pair<DWORD,DWORD> > expSamples;

    // Skill cooldowns
    std::array<HelperSkillCD,8> skillCDs;

    // GG auth session (one per connected player)
    CCSAuth2* pGGAuth;
    bool      bGGAuthReady;

    // Profile cache
    std::array<HelperProfile,5> profiles;

    MuHelperSession()
        : bActive(false), nTarget(-1), bClass(0)
        , bRotationStep(0), bAttackCount(0), bComboHitCount(0)
        , tLastAttack(0), tLastPickup(0), tLastBuff(0), tLastRepair(0)
        , tLastPotion(0), tLastParty(0), tLastPartyHeal(0)
        , dwZenPickup(0), wItemPickup(0), wKillCount(0)
        , dwExpGained(0), dwStartTimeSec(0)
        , pGGAuth(NULL), bGGAuthReady(false)
    {
        memset(&cfg, 0, sizeof(cfg));
    }
};

// ============================================================
class CMuHelperManager
{
public:
    static CMuHelperManager& Instance();

    // Lifecycle
    void OnServerStart();
    void OnServerTick();
    void OnServerShutdown();

    // Packet handlers (called from GameServer RecvProtocol dispatch)
    void OnPktEnable       (int idx, const PKT_MuHelper_Enable*);
    void OnPktCfgSend      (int idx, const PKT_MuHelper_CfgSend*);
    void OnPktCfgRequest   (int idx);
    void OnPktProfileSave  (int idx, const PKT_MuHelper_ProfileSave*);
    void OnPktProfileLoad  (int idx, const PKT_MuHelper_ProfileLoad*);

    // GameServer event hooks
    void OnCharLoad        (int idx, DWORD dwCharDbId);
    void OnCharDisconnect  (int idx);
    void OnMonsterKilled   (int killerIdx, int monsterIdx, DWORD dwExp);
    void OnItemDropped     (int idx, int itemIdx);     // for stop-on-drop
    void OnLevelUp         (int idx);                  // for stop-on-level

    // GG Auth callbacks (called by GgSrvDll after challenge/answer)
    static void CALLBACK OnGGAuthComplete(int nObjIdx, CCSAuth2* pAuth, int nResult);

private:
    CMuHelperManager();

    // Per-player update
    void TickPlayer        (int idx);
    void DoAutoAttack      (int idx, MuHelperSession&);
    void DoAutoPickup      (int idx, MuHelperSession&);
    void DoAutoPotion      (int idx, MuHelperSession&);
    void DoAutoBuff        (int idx, MuHelperSession&);
    void DoAutoRepair      (int idx, MuHelperSession&);
    void DoPartyHPBroadcast(int idx, MuHelperSession&);
    void DoPartyHeal       (int idx, MuHelperSession&);
    void DoComboAttack     (int idx, MuHelperSession&, const ClassSkillInfo*);
    void UpdateSkillCDs    (int idx, MuHelperSession&);

    // Class-aware skill selection
    WORD ResolveAttackSkill  (int idx, MuHelperSession&, const ClassSkillInfo*);
    WORD ResolveAoESkill     (const ClassSkillInfo*);
    void ApplyClassBuffs     (int idx, MuHelperSession&, const ClassSkillInfo*);

    // Target / item selection
    int  FindBestTarget    (int idx, const MuHelperSession&);
    int  CountMobsInRange  (int idx, int radius);
    bool ShouldPickup      (int itemIdx, const MuHelperSession&);
    bool IsValidMob        (int idx, int mobIdx, int radius);

    // Rate calculation
    DWORD CalcKillsPerHour (MuHelperSession&);
    DWORD CalcExpPerHour   (MuHelperSession&);
    WORD  CalcSessionMinutes(MuHelperSession&);

    // Network
    void SendCfgAck         (int idx, bool ok);
    void SendCfgReply       (int idx, const MuHelperConfig&);
    void SendItemPicked     (int idx, int itemIdx);
    void SendStatus         (int idx, const MuHelperSession&);
    void SendSkillStatus    (int idx, BYTE slot, WORD cdMs);
    void SendPartyHP        (int idx, const MuHelperSession&);

    // DB
    void LoadCfgFromDB      (int idx, DWORD charId);
    void SaveCfgToDB        (int idx);
    void LoadProfilesFromDB (int idx, DWORD charId);
    void SaveProfileToDB    (int idx, BYTE slot);

    // GG auth per-player setup
    void InitGGAuthForPlayer  (int idx);
    void CloseGGAuthForPlayer (int idx);

    // Data
    std::unordered_map<int,MuHelperSession> m_sessions;
    std::unordered_map<int,DWORD>           m_charIds;
    CRITICAL_SECTION                        m_cs;

    DWORD                                   m_dwLastTick;
    bool                                    m_bGGInitOk;
};

#define g_MuHelper CMuHelperManager::Instance()
