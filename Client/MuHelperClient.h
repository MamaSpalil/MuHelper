#pragma once
#include "../Shared/MuHelperPackets.h"
#include <functional>
#include <vector>
#include <string>
#include <array>
#include <deque>

// ============================================================
//  MuHelper v2 — Client Logic
// ============================================================

// Log entry with colour tag
enum class LogColor { White, Gold, Green, Red, Cyan, Gray };
struct HelperLogEntry
{
    std::string sText;
    DWORD       dwTimestamp;
    LogColor    color;
};

// Skill cooldown bar data
struct SkillCooldown { BYTE bSlot; WORD wMaxMs; WORD wRemMs; DWORD dwLastUpdate; };

// Statistics snapshot
struct SessionStats
{
    DWORD dwZenPickup;
    WORD  wItemPickup;
    WORD  wKillCount;
    DWORD dwExpGained;
    DWORD dwExpPerHour;
    WORD  wKillsPerHour;
    WORD  wSessionMinutes;
};

// Callbacks
using CbStatus    = std::function<void(const PKT_MuHelper_Status&)>;
using CbItemPick  = std::function<void(const PKT_MuHelper_ItemPicked&)>;
using CbCfgReply  = std::function<void(const MuHelperConfig&)>;
using CbPartyHP   = std::function<void(const PKT_MuHelper_PartyHP&)>;

// ============================================================
class CMuHelperClient
{
public:
    static CMuHelperClient& Instance();

    void Init();

    // Packet in from server
    void OnPacketReceived(BYTE* lpMsg, int nLen);

    // UI commands
    void SendEnable   (bool bEnable);
    void SendConfig   (const MuHelperConfig& cfg);
    void RequestConfig();
    void SaveProfile  (BYTE slot, const char* name, const MuHelperConfig& cfg);
    void LoadProfile  (BYTE slot);

    // Callbacks
    void SetOnStatus  (CbStatus   cb) { m_cbStatus   = cb; }
    void SetOnItemPick(CbItemPick cb) { m_cbItemPick = cb; }
    void SetOnCfgReply(CbCfgReply cb) { m_cbCfgReply = cb; }
    void SetOnPartyHP (CbPartyHP  cb) { m_cbPartyHP  = cb; }

    // State accessors
    bool                     IsRunning()    const { return m_bRunning; }
    const SessionStats&      GetStats()     const { return m_stats; }
    const MuHelperConfig&    GetConfig()    const { return m_config; }
    const PKT_MuHelper_PartyHP& GetPartyHP() const { return m_partyHP; }
    const SkillCooldown&     GetSkillCD(int i) const { return m_skills[i]; }
    const std::deque<HelperLogEntry>& GetLog() const { return m_log; }
    std::array<HelperProfile,5>& GetProfiles() { return m_profiles; }

    // Direct send (used by server integration to inject packets)
    void RawSend(BYTE* lpMsg, int nLen);

private:
    CMuHelperClient() = default;
    void HandleCfgAck    (BYTE*, int);
    void HandleCfgReply  (BYTE*, int);
    void HandleItemPicked(BYTE*, int);
    void HandleStatus    (BYTE*, int);
    void HandleSkillCD   (BYTE*, int);
    void HandlePartyHP   (BYTE*, int);
    void AddLog(LogColor c, const char* fmt, ...);

    bool                   m_bRunning = false;
    MuHelperConfig         m_config   = {};
    SessionStats           m_stats    = {};
    PKT_MuHelper_PartyHP   m_partyHP  = {};
    SkillCooldown          m_skills[8]= {};

    std::array<HelperProfile,5>   m_profiles = {};
    std::deque<HelperLogEntry>    m_log;
    static constexpr int          MAX_LOG = 80;

    CbStatus   m_cbStatus;
    CbItemPick m_cbItemPick;
    CbCfgReply m_cbCfgReply;
    CbPartyHP  m_cbPartyHP;
};
