#pragma once
// ============================================================
//  MuHelper v2 — Server Manager
//  GameServer 1.00.19  |  GgSrvDll.dll verified integration
// ============================================================
#include "../Shared/MuHelperPackets.h"
#include "GgSrvDll_Interface.h"
#include <unordered_map>
#include <array>
#include <deque>
#include <mutex>
#include <chrono>
#include <string>

// ── Tuning ───────────────────────────────────────────────────
constexpr int  MH_TICK_MS           = 500;
constexpr int  MH_ATTACK_DELAY_MS   = 600;
constexpr int  MH_PICKUP_DELAY_MS   = 400;
constexpr int  MH_BUFF_INTERVAL_MS  = 30000;
constexpr int  MH_REPAIR_INTERVAL_MS= 60000;
constexpr int  MH_PARTY_SEND_MS     = 2000;
constexpr int  MH_POTION_COOLDOWN_MS= 1500;
constexpr int  MH_MAX_RADIUS        = 12;
constexpr int  MH_DEFAULT_RADIUS    = 5;
constexpr int  MH_EXP_SAMPLE_SEC    = 60;   // rate averaging window

// ── Per-session state ────────────────────────────────────────
struct HelperSkillCD
{
    BYTE  bSlot;
    DWORD dwStartMs;
    DWORD dwDurationMs;
};

struct MuHelperSession
{
    MuHelperConfig   cfg       = {};
    bool             bActive   = false;
    int              nTarget   = -1;

    // Skill rotation state
    BYTE bRotationStep = 0;
    BYTE bAttackCount  = 0;

    // Timers
    using tp = std::chrono::steady_clock::time_point;
    tp tLastAttack, tLastPickup, tLastBuff, tLastRepair, tLastPotion, tLastParty;

    // Stats
    DWORD dwZenPickup       = 0;
    WORD  wItemPickup       = 0;
    WORD  wKillCount        = 0;
    DWORD dwExpGained       = 0;
    DWORD dwStartTimeSec    = 0;

    // Rate tracking (circular buffers)
    std::deque<std::pair<DWORD,WORD>>  killSamples;  // {tick,count}
    std::deque<std::pair<DWORD,DWORD>> expSamples;

    // Skill cooldowns
    std::array<HelperSkillCD,8> skillCDs = {};

    // GG auth session (one per connected player)
    CCSAuth2* pGGAuth = nullptr;
    bool      bGGAuthReady = false;

    // Profile cache
    std::array<HelperProfile,5> profiles = {};
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
    CMuHelperManager() = default;

    // Per-player update
    void TickPlayer        (int idx);
    void DoAutoAttack      (int idx, MuHelperSession&);
    void DoAutoPickup      (int idx, MuHelperSession&);
    void DoAutoPotion      (int idx, MuHelperSession&);
    void DoAutoBuff        (int idx, MuHelperSession&);
    void DoAutoRepair      (int idx, MuHelperSession&);
    void DoPartyHPBroadcast(int idx, MuHelperSession&);
    void UpdateSkillCDs    (int idx, MuHelperSession&);

    // Target / item selection
    int  FindBestTarget    (int idx, const MuHelperSession&);
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
    std::recursive_mutex                     m_mutex;

    std::chrono::steady_clock::time_point   m_tLastTick;
    bool                                    m_bGGInitOk = false;
};

#define g_MuHelper CMuHelperManager::Instance()
