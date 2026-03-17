// ============================================================
//  MuHelper v2 — Client Logic Implementation
// ============================================================
#include "MuHelperClient.h"
#include "HookEngine.h"
#include <cstdio>
#include <cstdarg>
#include <cstring>

CMuHelperClient& CMuHelperClient::Instance()
{
    static CMuHelperClient inst;
    return inst;
}

void CMuHelperClient::Init()
{
    memset(&m_config, 0, sizeof(m_config));
    m_config.bAttackRadius    = MUHELPER_DEFAULT_RADIUS;
    m_config.bPickupRadius    = MUHELPER_DEFAULT_RADIUS;
    m_config.wPickupFlags     = PICKUP_FLAG_ZEN|PICKUP_FLAG_JEWELS|PICKUP_FLAG_EXCELLENT;
    m_config.bPotionFlags     = POTION_FLAG_HP|POTION_FLAG_MP;
    m_config.bHpThreshold     = 30;
    m_config.bMpThreshold     = 20;
    m_config.bShieldThreshold = 30;
    m_config.bAttackSkillSlot = 0xFF;
    m_config.bComboSkillSlot  = 0xFF;
    m_config.bBuffSkillSlot   = BUFF_SLOT_NONE;
    m_config.bBuffSkillSlot2  = BUFF_SLOT_NONE;
    m_config.bCombatMode      = COMBAT_MODE_ATTACK;
    m_config.bRotationSlot1   = 0xFF;
    m_config.bRotationSlot2   = 0xFF;
    m_config.bRotationSlot3   = 0xFF;
    m_config.bRotationInterval= 3;
    m_config.bShowStatOverlay  = 1;

    // Init profiles
    for (auto& p : m_profiles) { p.bUsed = false; memset(p.szName,0,16); }

    RequestConfig();
    AddLog(LogColor::Cyan, "MuHelper v2 ready. Press F10 to open.");
}

// ── Packet router ─────────────────────────────────────────────
void CMuHelperClient::OnPacketReceived(BYTE* lpMsg, int nLen)
{
    if (nLen < 4) return;
    switch (lpMsg[3])
    {
    case MUHELPER_SUB_CFG_ACK:    HandleCfgAck    (lpMsg, nLen); break;
    case MUHELPER_SUB_CFG_REPLY:  HandleCfgReply  (lpMsg, nLen); break;
    case MUHELPER_SUB_ITEM_PICKED:HandleItemPicked(lpMsg, nLen); break;
    case MUHELPER_SUB_STATUS:     HandleStatus    (lpMsg, nLen); break;
    case MUHELPER_SUB_SKILL_STATUS:HandleSkillCD  (lpMsg, nLen); break;
    case MUHELPER_SUB_PARTY_HP:   HandlePartyHP   (lpMsg, nLen); break;
    }
}

// ── Handlers ──────────────────────────────────────────────────
void CMuHelperClient::HandleCfgAck(BYTE* lpMsg, int nLen)
{
    if (nLen < (int)sizeof(PKT_MuHelper_CfgAck)) return;
    auto* p = (PKT_MuHelper_CfgAck*)lpMsg;
    if (p->bResult == 0)
        AddLog(LogColor::Green, "Config saved.");
    else
        AddLog(LogColor::Red, "Config save failed.");
}

void CMuHelperClient::HandleCfgReply(BYTE* lpMsg, int nLen)
{
    if (nLen < (int)sizeof(PKT_MuHelper_CfgReply)) return;
    m_config = ((PKT_MuHelper_CfgReply*)lpMsg)->cfg;
    AddLog(LogColor::Cyan, "Config loaded.");
    if (m_cbCfgReply) m_cbCfgReply(m_config);
}

void CMuHelperClient::HandleItemPicked(BYTE* lpMsg, int nLen)
{
    if (nLen < (int)sizeof(PKT_MuHelper_ItemPicked)) return;
    auto* p = (PKT_MuHelper_ItemPicked*)lpMsg;

    if (p->dwZenAmount > 0)
        AddLog(LogColor::Gold, "Zen: +%u", p->dwZenAmount);
    else
    {
        const char* tag = "";
        if (p->bItemOpts & 0x01) tag = " [EXC]";
        else if (p->bItemOpts & 0x02) tag = " [ANC]";
        else if (p->bItemOpts & 0x04) tag = " [SOC]";

        LogColor c = (p->bItemOpts) ? LogColor::Gold : LogColor::White;
        AddLog(c, "+%s +%d%s", p->szItemName, p->bItemLevel, tag);
    }
    if (m_cbItemPick) m_cbItemPick(*p);
}

void CMuHelperClient::HandleStatus(BYTE* lpMsg, int nLen)
{
    if (nLen < (int)sizeof(PKT_MuHelper_Status)) return;
    auto* p = (PKT_MuHelper_Status*)lpMsg;
    m_bRunning           = (p->bRunning != 0);
    m_stats.dwZenPickup   = p->dwZenPickup;
    m_stats.wItemPickup   = p->wItemPickup;
    m_stats.wKillCount    = p->wKillCount;
    m_stats.dwExpGained   = p->dwExpGained;
    m_stats.dwExpPerHour  = p->dwExpPerHour;
    m_stats.wKillsPerHour = p->wKillsPerHour;
    m_stats.wSessionMinutes=p->wSessionMinutes;
    if (m_cbStatus) m_cbStatus(*p);
}

void CMuHelperClient::HandleSkillCD(BYTE* lpMsg, int nLen)
{
    if (nLen < (int)sizeof(PKT_MuHelper_SkillStatus)) return;
    auto* p = (PKT_MuHelper_SkillStatus*)lpMsg;
    if (p->bSlot < 8)
    {
        m_skills[p->bSlot].bSlot       = p->bSlot;
        m_skills[p->bSlot].wRemMs      = p->wCooldownMs;
        m_skills[p->bSlot].dwLastUpdate= GetTickCount();
    }
}

void CMuHelperClient::HandlePartyHP(BYTE* lpMsg, int nLen)
{
    if (nLen < (int)sizeof(PKT_MuHelper_PartyHP)) return;
    m_partyHP = *(PKT_MuHelper_PartyHP*)lpMsg;
    if (m_cbPartyHP) m_cbPartyHP(m_partyHP);
}

// ── Commands ──────────────────────────────────────────────────
void CMuHelperClient::SendEnable(bool bEnable)
{
    PKT_MuHelper_Enable pkt = {};
    pkt.h = 0xC1; pkt.sz = sizeof(pkt);
    pkt.op = MUHELPER_OPCODE_MAIN; pkt.sub = MUHELPER_SUB_ENABLE;
    pkt.bEnable = bEnable ? 1 : 0;
    RawSend((BYTE*)&pkt, sizeof(pkt));
    AddLog(bEnable ? LogColor::Green : LogColor::Red,
           "Helper %s.", bEnable ? "STARTED" : "STOPPED");
}

void CMuHelperClient::SendConfig(const MuHelperConfig& cfg)
{
    m_config = cfg;
    PKT_MuHelper_CfgSend pkt = {};
    pkt.h = 0xC1; pkt.sz = sizeof(pkt);
    pkt.op = MUHELPER_OPCODE_MAIN; pkt.sub = MUHELPER_SUB_CFG_SEND;
    pkt.cfg = cfg;
    RawSend((BYTE*)&pkt, sizeof(pkt));
}

void CMuHelperClient::RequestConfig()
{
    PKT_MuHelper_CfgRequest pkt = {};
    pkt.h = 0xC1; pkt.sz = sizeof(pkt);
    pkt.op = MUHELPER_OPCODE_MAIN; pkt.sub = MUHELPER_SUB_CFG_REQUEST;
    RawSend((BYTE*)&pkt, sizeof(pkt));
}

void CMuHelperClient::SaveProfile(BYTE slot, const char* name, const MuHelperConfig& cfg)
{
    if (slot >= MUHELPER_MAX_PROFILES) return;
    PKT_MuHelper_ProfileSave pkt = {};
    pkt.h = 0xC1; pkt.sz = sizeof(pkt);
    pkt.op = MUHELPER_OPCODE_MAIN; pkt.sub = MUHELPER_SUB_PROFILE_SAVE;
    pkt.bSlot = slot;
    strncpy_s(pkt.szName, name, 15);
    pkt.cfg = cfg;
    RawSend((BYTE*)&pkt, sizeof(pkt));

    // Cache locally
    m_profiles[slot].bUsed = true;
    strncpy_s(m_profiles[slot].szName, name, 15);
    m_profiles[slot].cfg = cfg;
    AddLog(LogColor::Cyan, "Profile %d '%s' saved.", slot+1, name);
}

void CMuHelperClient::LoadProfile(BYTE slot)
{
    if (slot >= MUHELPER_MAX_PROFILES) return;
    PKT_MuHelper_ProfileLoad pkt = {};
    pkt.h = 0xC1; pkt.sz = sizeof(pkt);
    pkt.op = MUHELPER_OPCODE_MAIN; pkt.sub = MUHELPER_SUB_PROFILE_LOAD;
    pkt.bSlot = slot;
    RawSend((BYTE*)&pkt, sizeof(pkt));
    AddLog(LogColor::Cyan, "Loading profile %d...", slot+1);
}

void CMuHelperClient::RawSend(BYTE* lpMsg, int nLen)
{
    if (HookEngine::OrigDataSend)
    {
        void* pNet = GetNetObject();
        if (pNet) HookEngine::OrigDataSend(pNet, lpMsg, nLen);
    }
}

void CMuHelperClient::AddLog(LogColor c, const char* fmt, ...)
{
    char buf[256];
    va_list ap; va_start(ap, fmt); vsnprintf(buf, 255, fmt, ap); va_end(ap);
    if (m_log.size() >= MAX_LOG) m_log.pop_front();
    m_log.push_back({ buf, GetTickCount(), c });
}
