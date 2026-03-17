// ============================================================
//  MuHelper v2 — Server Implementation
//  Integrates with GgSrvDll.dll (verified binary analysis)
//  GameServer 1.00.18
//  Database: MSSQL 2008 R2 (ODBC via GameServer DatabaseManager)
// ============================================================
#include "MuHelperManager.h"

// ── GameServer 1.00.18 internal headers (adjust paths) ───────
#include "../../GameServer/Object.h"
#include "../../GameServer/GameProtocol.h"
#include "../../GameServer/ItemManager.h"
#include "../../GameServer/DatabaseManager.h"
#include "../../GameServer/LogManager.h"
#include "../../GameServer/SkillManager.h"
#include "../../GameServer/PartyManager.h"

// GameServer globals
extern OBJECTSTRUCT gObj[];
extern int          gObjMaxIndex;
extern ITEMSTRUCT   gItem[];
extern int          gItemCount;

// GgSrvDll API instance
GgSrvDll_API g_GgSrv;

// ── Timer helpers ─────────────────────────────────────────────
static bool Elapsed(DWORD tLast, int ms)
{
    return (GetTickCount() - tLast) >= (DWORD)ms;
}
static DWORD GetTickSec() { return GetTickCount() / 1000; }

// ============================================================
CMuHelperManager& CMuHelperManager::Instance()
{
    static CMuHelperManager inst;
    return inst;
}

CMuHelperManager::CMuHelperManager() : m_dwLastTick(0), m_bGGInitOk(false)
{
    InitializeCriticalSection(&m_cs);
}

// ============================================================
//  SERVER START / SHUTDOWN
// ============================================================
void CMuHelperManager::OnServerStart()
{
    // Load GgSrvDll — it should already be loaded by GameServer
    if (g_GgSrv.Load("GgSrvDll.dll"))
    {
        // Register our MuHelper opcode so GG does NOT intercept it
        // (We are NOT registering 0x97 with AddAuthProtocol intentionally —
        //  0x97 is our custom opcode and bypasses GG auth entirely)
        m_bGGInitOk = true;
        LogAdd("[MuHelper] GgSrvDll.dll loaded OK. GG auth available.");
    }
    else
    {
        LogAdd("[MuHelper] WARNING: GgSrvDll.dll not loaded. GG auth disabled.");
    }
    LogAdd("[MuHelper] v2 server manager started.");
}

void CMuHelperManager::OnServerShutdown()
{
    EnterCriticalSection(&m_cs);
    for (std::unordered_map<int,MuHelperSession>::iterator it = m_sessions.begin();
         it != m_sessions.end(); ++it)
        CloseGGAuthForPlayer(it->first);
    m_sessions.clear();
    LeaveCriticalSection(&m_cs);
    DeleteCriticalSection(&m_cs);
}

// ============================================================
//  SERVER TICK
// ============================================================
void CMuHelperManager::OnServerTick()
{
    if (!Elapsed(m_dwLastTick, MH_TICK_MS)) return;
    m_dwLastTick = GetTickCount();

    CSGuard lk(m_cs);
    for (std::unordered_map<int,MuHelperSession>::iterator it = m_sessions.begin();
         it != m_sessions.end(); ++it)
        if (it->second.bActive) TickPlayer(it->first);
}

void CMuHelperManager::TickPlayer(int idx)
{
    OBJECTSTRUCT& obj = gObj[idx];
    if (obj.Connected != PLAYER_PLAYING) return;
    if (obj.Life <= 0) return;

    MuHelperSession& s = m_sessions[idx];

    UpdateSkillCDs       (idx, s);
    DoAutoPotion         (idx, s);
    DoAutoBuff           (idx, s);
    DoAutoRepair         (idx, s);
    DoAutoAttack         (idx, s);
    DoAutoPickup         (idx, s);
    DoPartyHeal          (idx, s);

    // Party HP broadcast every 2s
    if (Elapsed(s.tLastParty, MH_PARTY_SEND_MS))
    {
        DoPartyHPBroadcast(idx, s);
        s.tLastParty = GetTickCount();
    }

    // Status packet every 5s (10 ticks)
    static int sStatusCtr = 0;
    if (++sStatusCtr % 10 == 0) SendStatus(idx, s);
}

// ============================================================
//  SKILL COOLDOWN TRACKING
// ============================================================
void CMuHelperManager::UpdateSkillCDs(int idx, MuHelperSession& s)
{
    DWORD now = GetTickCount();
    for (size_t i = 0; i < s.skillCDs.size(); ++i)
    {
        HelperSkillCD& cd = s.skillCDs[i];
        if (cd.dwDurationMs == 0) continue;
        DWORD elapsed = now - cd.dwStartMs;
        if (elapsed < cd.dwDurationMs)
        {
            WORD rem = (WORD)(cd.dwDurationMs - elapsed);
            SendSkillStatus(idx, cd.bSlot, rem);
        }
        else if (cd.dwDurationMs > 0)
        {
            cd.dwDurationMs = 0;
            SendSkillStatus(idx, cd.bSlot, 0);
        }
    }
}

static void SetSkillCD(MuHelperSession& s, BYTE slot, DWORD durationMs)
{
    if (slot >= 8) return;
    s.skillCDs[slot].bSlot      = slot;
    s.skillCDs[slot].dwStartMs  = GetTickCount();
    s.skillCDs[slot].dwDurationMs = durationMs;
}

// ============================================================
//  CLASS-AWARE SKILL RESOLUTION
// ============================================================
WORD CMuHelperManager::ResolveAttackSkill(int idx, MuHelperSession& s, const ClassSkillInfo* ci)
{
    if (!ci) return SKILL_NONE;

    // If AoE mode and multiple mobs in range, prefer AoE skill
    if ((s.cfg.bCombatMode & COMBAT_MODE_AOE) && ci->wAoESkill != SKILL_NONE)
    {
        if (CountMobsInRange(idx, s.cfg.bAttackRadius) >= 3)
            return ci->wAoESkill;
    }

    // Alternate between primary and secondary
    if (s.bAttackCount % 3 == 2 && ci->wSecondarySkill != SKILL_NONE)
        return ci->wSecondarySkill;

    return ci->wPrimarySkill;
}

WORD CMuHelperManager::ResolveAoESkill(const ClassSkillInfo* ci)
{
    if (!ci) return SKILL_NONE;
    return ci->wAoESkill;
}

int CMuHelperManager::CountMobsInRange(int idx, int radius)
{
    OBJECTSTRUCT& self = gObj[idx];
    int count = 0;
    float r2 = (float)(radius * radius);
    for (int i = 0; i < gObjMaxIndex; i++)
    {
        if (i == idx) continue;
        OBJECTSTRUCT& o = gObj[i];
        if (o.Type != OBJTYPE_MONSTER || o.Life <= 0) continue;
        if (o.MapNumber != self.MapNumber) continue;
        float dx = (float)(self.X - o.X);
        float dy = (float)(self.Y - o.Y);
        if (dx*dx + dy*dy <= r2) count++;
    }
    return count;
}

// ============================================================
//  COMBO ATTACK (BK/HK specific)
// ============================================================
void CMuHelperManager::DoComboAttack(int idx, MuHelperSession& s, const ClassSkillInfo* ci)
{
    if (!ci || ci->wComboSkill == SKILL_NONE) return;
    if (ci->bComboHitsRequired == 0) return;

    s.bComboHitCount++;
    if (s.bComboHitCount >= ci->bComboHitsRequired)
    {
        // Fire combo finisher
        if (s.nTarget >= 0)
        {
            GCSkillAttackSend(idx, s.nTarget, ci->wComboSkill & 0xFF);
            SetSkillCD(s, ci->wComboSkill & 0x07, 2000);
        }
        s.bComboHitCount = 0;
    }
}

// ============================================================
//  AUTO ATTACK + SKILL ROTATION (class-aware)
// ============================================================
void CMuHelperManager::DoAutoAttack(int idx, MuHelperSession& s)
{
    if (!(s.cfg.bCombatMode & COMBAT_MODE_ATTACK)) return;
    if (!Elapsed(s.tLastAttack, MH_ATTACK_DELAY_MS)) return;

    // Validate / find target
    if (s.nTarget >= 0)
    {
        if (!IsValidMob(idx, s.nTarget, s.cfg.bAttackRadius))
            s.nTarget = -1;
    }
    if (s.nTarget < 0) s.nTarget = FindBestTarget(idx, s);
    if (s.nTarget < 0) return;

    // Resolve class info for this player
    const ClassSkillInfo* ci = GetClassSkillInfo(s.bClass);

    // Determine which skill to use this tick
    BYTE skillSlot = 0xFF;

    // Skill rotation: every N attacks rotate through extra slots
    bool useRotation = (s.cfg.bRotationSlot1 != 0xFF ||
                        s.cfg.bRotationSlot2 != 0xFF ||
                        s.cfg.bRotationSlot3 != 0xFF);

    if (useRotation && s.cfg.bRotationInterval > 0 &&
        s.bAttackCount >= s.cfg.bRotationInterval)
    {
        BYTE rotSlots[3] = { s.cfg.bRotationSlot1,
                             s.cfg.bRotationSlot2,
                             s.cfg.bRotationSlot3 };
        // Advance rotation step
        while (s.bRotationStep < 3 && rotSlots[s.bRotationStep] == 0xFF)
            s.bRotationStep = (s.bRotationStep + 1) % 3;

        BYTE rs = rotSlots[s.bRotationStep];
        if (rs != 0xFF)
        {
            skillSlot = rs;
            s.bRotationStep = (s.bRotationStep + 1) % 3;
            s.bAttackCount = 0;
        }
    }
    else if (s.cfg.bCombatMode & COMBAT_MODE_SKILL)
    {
        // Class-aware skill selection: use class info to determine
        // whether we should use the AoE or primary skill slot
        if (ci)
        {
            WORD resolvedSkill = ResolveAttackSkill(idx, s, ci);
            if (resolvedSkill != SKILL_NONE)
            {
                skillSlot = s.cfg.bAttackSkillSlot;
                // If resolved to AoE and combo slot is configured, use that
                if (resolvedSkill == ci->wAoESkill &&
                    s.cfg.bComboSkillSlot != 0xFF)
                {
                    skillSlot = s.cfg.bComboSkillSlot;
                }
            }
        }
        else
        {
            skillSlot = s.cfg.bAttackSkillSlot;
        }
    }

    s.bAttackCount++;

    // Dispatch
    if (skillSlot == 0xFF)
        GCAttackSend(idx, s.nTarget);
    else
    {
        GCSkillAttackSend(idx, s.nTarget, skillSlot);
        // Use class-specific cooldown based on which skill was selected
        DWORD cdMs = 1500;
        if (ci)
        {
            if (skillSlot == s.cfg.bComboSkillSlot)
                cdMs = ci->dwAoECooldownMs;
            else
                cdMs = ci->dwPrimaryCooldownMs;
        }
        SetSkillCD(s, skillSlot, cdMs);
    }

    // Combo system (BK/HK)
    if ((s.cfg.bCombatMode & COMBAT_MODE_COMBO) && ci)
        DoComboAttack(idx, s, ci);

    s.tLastAttack = GetTickCount();
}

// ============================================================
//  AUTO PICKUP (with item quality filtering)
// ============================================================
void CMuHelperManager::DoAutoPickup(int idx, MuHelperSession& s)
{
    if (s.cfg.wPickupFlags == PICKUP_FLAG_NONE) return;
    if (!Elapsed(s.tLastPickup, MH_PICKUP_DELAY_MS)) return;

    OBJECTSTRUCT& self = gObj[idx];
    for (int i = 0; i < gItemCount; i++)
    {
        ITEMSTRUCT& item = gItem[i];
        if (!item.bOnGround) continue;
        if (item.MapNumber != self.MapNumber) continue;

        float dx = (float)(self.X - item.X);
        float dy = (float)(self.Y - item.Y);
        if (dx*dx + dy*dy > (float)(s.cfg.bPickupRadius * s.cfg.bPickupRadius)) continue;

        if (!ShouldPickup(i, s)) continue;

        if (GCPickItemSend(idx, i) == TRUE)
        {
            SendItemPicked(idx, i);
            if (item.Type == ITEMTYPE_ZEN) s.dwZenPickup += item.wAmount;
            else                           s.wItemPickup++;
            s.tLastPickup = GetTickCount();
            break;
        }
    }
}

bool CMuHelperManager::ShouldPickup(int itemIdx, const MuHelperSession& s)
{
    const ITEMSTRUCT& item = gItem[itemIdx];
    const WORD fl = s.cfg.wPickupFlags;

    if (fl == PICKUP_FLAG_ALL)   return true;
    if (!fl)                     return false;

    if (item.Type == ITEMTYPE_ZEN)  return (fl & PICKUP_FLAG_ZEN) != 0;
    if (item.bIsJewel)              return (fl & PICKUP_FLAG_JEWELS) != 0;
    if (item.bIsExcellent)          return (fl & PICKUP_FLAG_EXCELLENT) != 0;
    if (item.bIsAncient)            return (fl & PICKUP_FLAG_ANCIENT) != 0;
    if (item.bIsSetItem)            return (fl & PICKUP_FLAG_SET) != 0;
    if (item.bIsSocket)             return (fl & PICKUP_FLAG_SOCKETS) != 0;
    if (item.bIsWing)               return (fl & PICKUP_FLAG_WINGS) != 0;
    if (item.bIsPet)                return (fl & PICKUP_FLAG_PET) != 0;

    // Custom bitfield
    if (s.cfg.dwCustomItemFilter & (1u << (item.Type & 0x1F))) return true;

    return (fl & PICKUP_FLAG_MISC) != 0;
}

// ============================================================
//  AUTO POTION
// ============================================================
void CMuHelperManager::DoAutoPotion(int idx, MuHelperSession& s)
{
    if (!s.cfg.bPotionFlags) return;
    if (!Elapsed(s.tLastPotion, MH_POTION_COOLDOWN_MS)) return;

    OBJECTSTRUCT& obj = gObj[idx];

    auto UsePot = [&](int type) -> bool {
        if (GCUsePotionSend(idx, type) == TRUE)
        {
            s.tLastPotion = GetTickCount();
            return true;
        }
        return false;
    };

    if ((s.cfg.bPotionFlags & POTION_FLAG_HP) && obj.MaxLife > 0)
    {
        if ((obj.Life * 100 / obj.MaxLife) < s.cfg.bHpThreshold)
            if (UsePot(POTION_HP)) return;
    }
    if ((s.cfg.bPotionFlags & POTION_FLAG_MP) && obj.MaxMana > 0)
    {
        if ((obj.Mana * 100 / obj.MaxMana) < s.cfg.bMpThreshold)
            if (UsePot(POTION_MP)) return;
    }
    if ((s.cfg.bPotionFlags & POTION_FLAG_SHIELD) && obj.MaxShield > 0)
    {
        if ((obj.Shield * 100 / obj.MaxShield) < s.cfg.bShieldThreshold)
            if (UsePot(POTION_SHIELD)) return;
    }
    if (s.cfg.bPotionFlags & POTION_FLAG_ANTIDOTE)
    {
        if (obj.bPoisoned) UsePot(POTION_ANTIDOTE);
    }
}

// ============================================================
//  CLASS-AWARE BUFF APPLICATION
// ============================================================
void CMuHelperManager::ApplyClassBuffs(int idx, MuHelperSession& s, const ClassSkillInfo* ci)
{
    if (!ci) return;

    // Apply class-specific buff skills if available
    if (ci->wBuffSkill1 != SKILL_NONE)
    {
        GCSkillSelfSend(idx, ci->wBuffSkill1 & 0xFF);
        SetSkillCD(s, 6, 30000);  // buff slot 1 on CD slot 6
    }
    if (ci->wBuffSkill2 != SKILL_NONE)
    {
        GCSkillSelfSend(idx, ci->wBuffSkill2 & 0xFF);
        SetSkillCD(s, 7, 30000);  // buff slot 2 on CD slot 7
    }
}

// ============================================================
//  AUTO BUFF (class-aware, two custom slots + class buffs)
// ============================================================
void CMuHelperManager::DoAutoBuff(int idx, MuHelperSession& s)
{
    if (!s.cfg.bSelfBuff) return;
    if (!Elapsed(s.tLastBuff, MH_BUFF_INTERVAL_MS)) return;

    // User-configured buff slots (manual override)
    if (s.cfg.bBuffSkillSlot != BUFF_SLOT_NONE)
    {
        GCSkillSelfSend(idx, s.cfg.bBuffSkillSlot);
        SetSkillCD(s, s.cfg.bBuffSkillSlot, 30000);
    }
    if (s.cfg.bBuffSkillSlot2 != BUFF_SLOT_NONE)
    {
        GCSkillSelfSend(idx, s.cfg.bBuffSkillSlot2);
        SetSkillCD(s, s.cfg.bBuffSkillSlot2, 30000);
    }

    // Class-specific buffs (e.g. Elf Defense Up / Attack Up, DL Darkness)
    const ClassSkillInfo* ci = GetClassSkillInfo(s.bClass);
    ApplyClassBuffs(idx, s, ci);

    s.tLastBuff = GetTickCount();
}

// ============================================================
//  PARTY HEAL (Elf classes only: FE, ME, HE)
// ============================================================
void CMuHelperManager::DoPartyHeal(int idx, MuHelperSession& s)
{
    if (!s.cfg.bUsePartyHeal) return;

    // Only elf classes can heal party members
    const ClassSkillInfo* ci = GetClassSkillInfo(s.bClass);
    if (!ci || !ci->bHasPartyHeal || ci->wPartyHealSkill == SKILL_NONE) return;
    if (!Elapsed(s.tLastPartyHeal, MH_POTION_COOLDOWN_MS)) return;

    OBJECTSTRUCT& self = gObj[idx];
    if (self.PartyNumber < 0) return;

    // Find party member with lowest HP% below threshold
    int healTarget = -1;
    BYTE lowestHpPct = MH_PARTY_HEAL_HP_PCT;

    for (int i = 0; i < gObjMaxIndex; i++)
    {
        if (i == idx) continue;
        OBJECTSTRUCT& m = gObj[i];
        if (m.Connected != PLAYER_PLAYING) continue;
        if (m.PartyNumber != self.PartyNumber) continue;
        if (m.Life <= 0 || m.MaxLife <= 0) continue;

        BYTE hpPct = (BYTE)(m.Life * 100 / m.MaxLife);
        if (hpPct < lowestHpPct)
        {
            lowestHpPct = hpPct;
            healTarget = i;
        }
    }

    if (healTarget >= 0)
    {
        GCSkillAttackSend(idx, healTarget, ci->wPartyHealSkill & 0xFF);
        s.tLastPartyHeal = GetTickCount();
    }
}

// ============================================================
//  AUTO REPAIR
// ============================================================
void CMuHelperManager::DoAutoRepair(int idx, MuHelperSession& s)
{
    if (!s.cfg.bAutoRepair) return;
    if (!Elapsed(s.tLastRepair, MH_REPAIR_INTERVAL_MS)) return;

    OBJECTSTRUCT& obj = gObj[idx];
    for (int slot = 0; slot < MAX_EQUIPMENT_SLOT; slot++)
    {
        auto& eq = obj.Equipment[slot];
        if (eq.ItemType == 0xFF) continue;
        if (eq.DurabilityMax == 0) continue;
        if ((eq.Durability * 100 / eq.DurabilityMax) < 10)
        {
            GCRepairItemSend(idx, slot);
            s.tLastRepair = GetTickCount();
            break;
        }
    }
}

// ============================================================
//  PARTY HP BROADCAST
// ============================================================
void CMuHelperManager::DoPartyHPBroadcast(int idx, MuHelperSession& s)
{
    OBJECTSTRUCT& self = gObj[idx];
    if (self.PartyNumber < 0) return;

    PKT_MuHelper_PartyHP pkt = {};
    pkt.h = 0xC1; pkt.op = MUHELPER_OPCODE_MAIN; pkt.sub = MUHELPER_SUB_PARTY_HP;

    BYTE count = 0;
    for (int i = 0; i < gObjMaxIndex && count < 5; i++)
    {
        OBJECTSTRUCT& m = gObj[i];
        if (m.Connected != PLAYER_PLAYING) continue;
        if (m.PartyNumber != self.PartyNumber) continue;
        if (i == idx) continue;

        auto& pm = pkt.m[count++];
        strncpy_s(pm.szName, m.Name, 10);
        pm.bHpPct = (BYTE)(m.MaxLife > 0 ? m.Life * 100 / m.MaxLife : 0);
        pm.bMpPct = (BYTE)(m.MaxMana > 0 ? m.Mana * 100 / m.MaxMana : 0);
        pm.bClass = (BYTE)m.Class;
    }
    pkt.bCount = count;
    pkt.sz     = (BYTE)sizeof(pkt);
    if (count > 0) DataSend(idx, (BYTE*)&pkt, sizeof(pkt));
}

// ============================================================
//  TARGET SELECTION
// ============================================================
int CMuHelperManager::FindBestTarget(int idx, const MuHelperSession& s)
{
    OBJECTSTRUCT& self = gObj[idx];
    float  bestDist = 9999.f;
    int    bestIdx  = -1;

    for (int i = 0; i < gObjMaxIndex; i++)
    {
        if (i == idx) continue;
        OBJECTSTRUCT& o = gObj[i];
        if (o.Type != OBJTYPE_MONSTER)   continue;
        if (o.Life <= 0)                 continue;
        if (o.MapNumber != self.MapNumber) continue;
        if (s.cfg.bAvoidPvP && o.Type == OBJTYPE_PC) continue;

        float dx = (float)(self.X - o.X);
        float dy = (float)(self.Y - o.Y);
        float d  = dx*dx + dy*dy;
        if (d > (float)(s.cfg.bAttackRadius*s.cfg.bAttackRadius)) continue;
        if (d < bestDist) { bestDist=d; bestIdx=i; }
    }
    return bestIdx;
}

bool CMuHelperManager::IsValidMob(int idx, int mobIdx, int radius)
{
    if (mobIdx < 0) return false;
    OBJECTSTRUCT& self = gObj[idx];
    OBJECTSTRUCT& mob  = gObj[mobIdx];
    if (mob.Type != OBJTYPE_MONSTER) return false;
    if (mob.Life <= 0) return false;
    if (mob.MapNumber != self.MapNumber) return false;
    float dx=(float)(self.X-mob.X); float dy=(float)(self.Y-mob.Y);
    return (dx*dx+dy*dy) <= (float)(radius*radius);
}

// ============================================================
//  RATE CALCULATIONS
// ============================================================
DWORD CMuHelperManager::CalcKillsPerHour(MuHelperSession& s)
{
    DWORD now = GetTickSec();
    DWORD window = MH_EXP_SAMPLE_SEC;
    while (!s.killSamples.empty() && now - s.killSamples.front().first > window)
        s.killSamples.pop_front();
    if (s.killSamples.size() < 2) return 0;
    DWORD dt = now - s.killSamples.front().first;
    if (!dt) return 0;
    WORD  dk = s.killSamples.back().second - s.killSamples.front().second;
    return (DWORD)((DWORD)dk * 3600 / dt);
}

DWORD CMuHelperManager::CalcExpPerHour(MuHelperSession& s)
{
    DWORD now = GetTickSec();
    while (!s.expSamples.empty() && now - s.expSamples.front().first > MH_EXP_SAMPLE_SEC)
        s.expSamples.pop_front();
    if (s.expSamples.size() < 2) return 0;
    DWORD dt = now - s.expSamples.front().first;
    if (!dt) return 0;
    DWORD de = s.expSamples.back().second - s.expSamples.front().second;
    return de * 3600 / dt;
}

WORD CMuHelperManager::CalcSessionMinutes(MuHelperSession& s)
{
    if (!s.dwStartTimeSec) return 0;
    return (WORD)((GetTickSec() - s.dwStartTimeSec) / 60);
}

// ============================================================
//  PACKET HANDLERS
// ============================================================
void CMuHelperManager::OnPktEnable(int idx, const PKT_MuHelper_Enable* p)
{
    CSGuard lk(m_cs);
    auto& s = m_sessions[idx];
    s.bActive = (p->bEnable != 0);
    if (s.bActive)
    {
        s.dwZenPickup=0; s.wItemPickup=0; s.wKillCount=0; s.dwExpGained=0;
        s.nTarget=-1; s.bAttackCount=0; s.bRotationStep=0;
        s.killSamples.clear(); s.expSamples.clear();
        s.dwStartTimeSec = GetTickSec();
        LogAdd("[MuHelper] idx=%d ENABLED", idx);
    }
    else
    {
        LogAdd("[MuHelper] idx=%d DISABLED K=%d I=%d Z=%u",
               idx, s.wKillCount, s.wItemPickup, s.dwZenPickup);
    }
    SendCfgAck(idx, true);
    SendStatus(idx, s);
}

void CMuHelperManager::OnPktCfgSend(int idx, const PKT_MuHelper_CfgSend* p)
{
    CSGuard lk(m_cs);
    auto& s = m_sessions[idx];

    // Sanitize
    MuHelperConfig cfg = p->cfg;
    cfg.bAttackRadius  = min(cfg.bAttackRadius,  (BYTE)MH_MAX_RADIUS);
    cfg.bPickupRadius  = min(cfg.bPickupRadius,  (BYTE)MH_MAX_RADIUS);
    cfg.bHpThreshold   = min(cfg.bHpThreshold,   (BYTE)95);
    cfg.bMpThreshold   = min(cfg.bMpThreshold,   (BYTE)95);
    cfg.bShieldThreshold= min(cfg.bShieldThreshold,(BYTE)95);

    s.cfg = cfg;
    SaveCfgToDB(idx);
    SendCfgAck(idx, true);
}

void CMuHelperManager::OnPktCfgRequest(int idx)
{
    CSGuard lk(m_cs);
    SendCfgReply(idx, m_sessions[idx].cfg);
}

void CMuHelperManager::OnPktProfileSave(int idx, const PKT_MuHelper_ProfileSave* p)
{
    CSGuard lk(m_cs);
    if (p->bSlot >= MUHELPER_MAX_PROFILES) return;
    auto& s = m_sessions[idx];
    s.profiles[p->bSlot].bUsed = true;
    strncpy_s(s.profiles[p->bSlot].szName, p->szName, 15);
    s.profiles[p->bSlot].cfg = p->cfg;
    SaveProfileToDB(idx, p->bSlot);
    SendCfgAck(idx, true);
}

void CMuHelperManager::OnPktProfileLoad(int idx, const PKT_MuHelper_ProfileLoad* p)
{
    CSGuard lk(m_cs);
    if (p->bSlot >= MUHELPER_MAX_PROFILES) return;
    auto& s = m_sessions[idx];
    if (!s.profiles[p->bSlot].bUsed) return;
    s.cfg = s.profiles[p->bSlot].cfg;
    SendCfgReply(idx, s.cfg);
    LogAdd("[MuHelper] idx=%d loaded profile %d", idx, p->bSlot);
}

// ============================================================
//  GAME EVENTS
// ============================================================
void CMuHelperManager::OnCharLoad(int idx, DWORD dwCharDbId)
{
    CSGuard lk(m_cs);
    m_charIds[idx] = dwCharDbId;
    auto& s = m_sessions[idx];

    // Capture the character's class from the game object
    s.bClass = (BYTE)gObj[idx].Class;

    LoadCfgFromDB    (idx, dwCharDbId);
    LoadProfilesFromDB(idx, dwCharDbId);

    // Initialise GG auth session for this client
    InitGGAuthForPlayer(idx);

    const ClassSkillInfo* ci = GetClassSkillInfo(s.bClass);
    if (ci)
        LogAdd("[MuHelper] idx=%d class=%s loaded.", idx, ci->szClassName);
}

void CMuHelperManager::OnCharDisconnect(int idx)
{
    CSGuard lk(m_cs);
    SaveCfgToDB(idx);
    CloseGGAuthForPlayer(idx);
    m_sessions.erase(idx);
    m_charIds.erase(idx);
}

void CMuHelperManager::OnMonsterKilled(int killerIdx, int /*mobIdx*/, DWORD dwExp)
{
    CSGuard lk(m_cs);
    auto it = m_sessions.find(killerIdx);
    if (it == m_sessions.end() || !it->second.bActive) return;
    auto& s = it->second;

    s.wKillCount++;
    s.dwExpGained += dwExp;
    s.nTarget = -1;

    DWORD now = GetTickSec();
    s.killSamples.push_back(std::make_pair(now, s.wKillCount));
    s.expSamples.push_back(std::make_pair(now, s.dwExpGained));
}

void CMuHelperManager::OnItemDropped(int idx, int /*itemIdx*/)
{
    CSGuard lk(m_cs);
    auto it = m_sessions.find(idx);
    if (it == m_sessions.end()) return;
    auto& s = it->second;
    if (s.bActive && s.cfg.bStopOnDrop)
    {
        s.bActive = false;
        SendCfgAck(idx, true);
        SendStatus(idx, s);
        LogAdd("[MuHelper] idx=%d stopped (excellent/ancient drop)", idx);
    }
}

void CMuHelperManager::OnLevelUp(int idx)
{
    CSGuard lk(m_cs);
    auto it = m_sessions.find(idx);
    if (it == m_sessions.end()) return;
    auto& s = it->second;
    if (s.bActive && s.cfg.bStopOnLevelUp)
    {
        s.bActive = false;
        SendCfgAck(idx, true);
        SendStatus(idx, s);
        LogAdd("[MuHelper] idx=%d stopped (level up)", idx);
    }
}

// ============================================================
//  GG AUTH PER-PLAYER
// ============================================================
void CMuHelperManager::InitGGAuthForPlayer(int idx)
{
    if (!m_bGGInitOk || !g_GgSrv.GGAuthInitUser) return;
    auto& s = m_sessions[idx];
    if (s.pGGAuth) CloseGGAuthForPlayer(idx);

    CCSAuth2* pAuth = nullptr;
    s.pGGAuth = g_GgSrv.GGAuthInitUser(&pAuth);
    s.bGGAuthReady = (s.pGGAuth != nullptr);

    if (s.pGGAuth)
        LogAdd("[MuHelper] GG auth session created for idx=%d", idx);
}

void CMuHelperManager::CloseGGAuthForPlayer(int idx)
{
    auto& s = m_sessions[idx];
    if (s.pGGAuth && g_GgSrv.GGAuthCloseUser)
    {
        g_GgSrv.GGAuthCloseUser(s.pGGAuth);
        s.pGGAuth = nullptr;
        s.bGGAuthReady = false;
    }
}

// Static callback when GG auth completes (called by GgSrvDll timer thread)
void CALLBACK CMuHelperManager::OnGGAuthComplete(int nObjIdx, CCSAuth2* pAuth, int nResult)
{
    if (nResult != 0)
        LogAdd("[MuHelper] GG auth FAIL for idx=%d result=%d", nObjIdx, nResult);
    // Optionally kick the player if auth fails repeatedly
}

// ============================================================
//  NETWORK HELPERS
// ============================================================
void CMuHelperManager::SendCfgAck(int idx, bool ok)
{
    PKT_MuHelper_CfgAck p = {};
    p.h=0xC1; p.sz=sizeof(p); p.op=MUHELPER_OPCODE_MAIN; p.sub=MUHELPER_SUB_CFG_ACK;
    p.bResult = ok?0:1;
    DataSend(idx,(BYTE*)&p,sizeof(p));
}

void CMuHelperManager::SendCfgReply(int idx, const MuHelperConfig& cfg)
{
    PKT_MuHelper_CfgReply p = {};
    p.h=0xC1; p.sz=sizeof(p); p.op=MUHELPER_OPCODE_MAIN; p.sub=MUHELPER_SUB_CFG_REPLY;
    p.cfg=cfg;
    DataSend(idx,(BYTE*)&p,sizeof(p));
}

void CMuHelperManager::SendItemPicked(int idx, int itemIdx)
{
    const ITEMSTRUCT& item = gItem[itemIdx];
    PKT_MuHelper_ItemPicked p = {};
    p.h=0xC1; p.sz=sizeof(p); p.op=MUHELPER_OPCODE_MAIN; p.sub=MUHELPER_SUB_ITEM_PICKED;
    p.wItemType  = (WORD)item.Type;
    p.bItemLevel = item.Level;
    p.bItemOpts  = (item.bIsExcellent?0x01:0)|(item.bIsAncient?0x02:0)|(item.bIsSocket?0x04:0);
    p.dwZenAmount= (item.Type==ITEMTYPE_ZEN) ? item.wAmount : 0;
    strncpy_s(p.szItemName, item.szName, 31);
    DataSend(idx,(BYTE*)&p,sizeof(p));
}

void CMuHelperManager::SendStatus(int idx, const MuHelperSession& s)
{
    PKT_MuHelper_Status p = {};
    p.h=0xC1; p.sz=sizeof(p); p.op=MUHELPER_OPCODE_MAIN; p.sub=MUHELPER_SUB_STATUS;
    p.bRunning       = s.bActive?1:0;
    p.dwZenPickup    = s.dwZenPickup;
    p.wItemPickup    = s.wItemPickup;
    p.wKillCount     = s.wKillCount;
    p.dwExpGained    = s.dwExpGained;
    p.dwExpPerHour   = const_cast<CMuHelperManager*>(this)->CalcExpPerHour  (const_cast<MuHelperSession&>(s));
    p.wKillsPerHour  = (WORD)const_cast<CMuHelperManager*>(this)->CalcKillsPerHour(const_cast<MuHelperSession&>(s));
    p.wSessionMinutes= const_cast<CMuHelperManager*>(this)->CalcSessionMinutes(const_cast<MuHelperSession&>(s));
    DataSend(idx,(BYTE*)&p,sizeof(p));
}

void CMuHelperManager::SendSkillStatus(int idx, BYTE slot, WORD cdMs)
{
    PKT_MuHelper_SkillStatus p = {};
    p.h=0xC1; p.sz=sizeof(p); p.op=MUHELPER_OPCODE_MAIN; p.sub=MUHELPER_SUB_SKILL_STATUS;
    p.bSlot=slot; p.wCooldownMs=cdMs;
    DataSend(idx,(BYTE*)&p,sizeof(p));
}

void CMuHelperManager::SendPartyHP(int idx, const MuHelperSession&)
{
    // Actual send happens inside DoPartyHPBroadcast
}

// ============================================================
//  DATABASE HELPERS (MSSQL 2008 R2 via ODBC)
// ============================================================
void CMuHelperManager::LoadCfgFromDB(int idx, DWORD charId)
{
    char sql[256];
    sprintf_s(sql,"SELECT cfg FROM MuHelperConfig WHERE CharIdx=%u", charId);
    DB_RESULT* r = g_DbManager.Query(sql);
    if (r && r->NumRows()>0)
    {
        DB_ROW row = r->FetchRow();
        unsigned long* len = r->FetchLengths();
        if (row[0] && len[0] >= sizeof(MuHelperConfig))
            memcpy(&m_sessions[idx].cfg, row[0], sizeof(MuHelperConfig));
    }
    else
    {
        // Defaults
        MuHelperConfig& cfg = m_sessions[idx].cfg;
        memset(&cfg,0,sizeof(cfg));
        cfg.bAttackRadius   = MH_DEFAULT_RADIUS;
        cfg.bPickupRadius   = MH_DEFAULT_RADIUS;
        cfg.wPickupFlags    = PICKUP_FLAG_ZEN|PICKUP_FLAG_JEWELS|PICKUP_FLAG_EXCELLENT;
        cfg.bPotionFlags    = POTION_FLAG_HP|POTION_FLAG_MP;
        cfg.bHpThreshold    = 30; cfg.bMpThreshold=20; cfg.bShieldThreshold=30;
        cfg.bAttackSkillSlot= 0xFF; cfg.bComboSkillSlot=0xFF;
        cfg.bBuffSkillSlot  = BUFF_SLOT_NONE; cfg.bBuffSkillSlot2=BUFF_SLOT_NONE;
        cfg.bCombatMode     = COMBAT_MODE_ATTACK;
        cfg.bRotationSlot1  = 0xFF; cfg.bRotationSlot2=0xFF; cfg.bRotationSlot3=0xFF;
        cfg.bRotationInterval=3; cfg.bShowStatOverlay=1;
    }
    if (r) r->Free();
}

void CMuHelperManager::SaveCfgToDB(int idx)
{
    std::unordered_map<int,DWORD>::iterator ci = m_charIds.find(idx);
    std::unordered_map<int,MuHelperSession>::iterator si = m_sessions.find(idx);
    if (ci==m_charIds.end()||si==m_sessions.end()) return;
    // MSSQL 2008 R2: use IF EXISTS / UPDATE / ELSE INSERT pattern
    // (MERGE is also available in 2008 R2 but this pattern is simpler
    //  and compatible with the GameServer DatabaseManager ODBC layer)
    char sql[512];
    sprintf_s(sql,
        "IF EXISTS (SELECT 1 FROM MuHelperConfig WHERE CharIdx=%u) "
        "UPDATE MuHelperConfig SET cfg=?, UpdatedAt=GETDATE() WHERE CharIdx=%u "
        "ELSE "
        "INSERT INTO MuHelperConfig(CharIdx,cfg) VALUES(%u,?)",
        ci->second, ci->second, ci->second);
    g_DbManager.QueryBinary(sql,(BYTE*)&si->second.cfg,sizeof(MuHelperConfig));
}

void CMuHelperManager::LoadProfilesFromDB(int idx, DWORD charId)
{
    char sql[256];
    sprintf_s(sql,"SELECT SlotIdx,Name,cfg FROM MuHelperProfiles WHERE CharIdx=%u ORDER BY SlotIdx", charId);
    DB_RESULT* r = g_DbManager.Query(sql);
    if (r)
    {
        while (r->NumRows()>0)
        {
            DB_ROW row=r->FetchRow();
            if (!row[0]) break;
            int slot=atoi(row[0]);
            if (slot<0||slot>=MUHELPER_MAX_PROFILES) { r->FetchRow(); continue; }
            HelperProfile& p=m_sessions[idx].profiles[slot];
            p.bUsed=true;
            strncpy_s(p.szName,row[1]?row[1]:"",15);
            unsigned long* len=r->FetchLengths();
            if (row[2]&&len[2]>=sizeof(MuHelperConfig))
                memcpy(&p.cfg,row[2],sizeof(MuHelperConfig));
        }
        r->Free();
    }
}

void CMuHelperManager::SaveProfileToDB(int idx, BYTE slot)
{
    std::unordered_map<int,DWORD>::iterator ci=m_charIds.find(idx);
    std::unordered_map<int,MuHelperSession>::iterator si=m_sessions.find(idx);
    if (ci==m_charIds.end()||si==m_sessions.end()) return;
    HelperProfile& p=si->second.profiles[slot];
    if (!p.bUsed) return;
    // MSSQL 2008 R2: IF EXISTS / UPDATE / ELSE INSERT
    char sql[512];
    sprintf_s(sql,
        "IF EXISTS (SELECT 1 FROM MuHelperProfiles WHERE CharIdx=%u AND SlotIdx=%d) "
        "UPDATE MuHelperProfiles SET Name='%s',cfg=?,UpdatedAt=GETDATE() WHERE CharIdx=%u AND SlotIdx=%d "
        "ELSE "
        "INSERT INTO MuHelperProfiles(CharIdx,SlotIdx,Name,cfg) VALUES(%u,%d,'%s',?)",
        ci->second,(int)slot,p.szName,
        ci->second,(int)slot,
        ci->second,(int)slot,p.szName);
    g_DbManager.QueryBinary(sql,(BYTE*)&p.cfg,sizeof(MuHelperConfig));
}
