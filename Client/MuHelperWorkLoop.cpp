// ============================================================
//  MuHelper — Work Loop Implementation
//  Ported from MuMain Season 5.2 (MuHelper.cpp)
//  Adapted for Season 3 Ep 1 (main.exe 1.02.19)
//  VS2010-compatible (no C++11 containers / atomics)
// ============================================================
#include "MuHelperWorkLoop.h"
#include "HookEngine.h"
#include <cstring>
#include <cmath>
#include <cstdio>
#include <algorithm>

// ── Constants ────────────────────────────────────────────────
static const int MAX_ACTIONABLE_DISTANCE  = 10;
static const int DEFAULT_DURABILITY_THR   = 50;
static const int ITEMS_MAX                = 256;

// ── RAII guard for CRITICAL_SECTION (VS2010 compatible) ──────
struct CSGuard
{
    CRITICAL_SECTION& cs;
    CSGuard(CRITICAL_SECTION& c) : cs(c) { EnterCriticalSection(&cs); }
    ~CSGuard() { LeaveCriticalSection(&cs); }
};

// ── Singleton ────────────────────────────────────────────────
CMuHelperWorkLoop& CMuHelperWorkLoop::Instance()
{
    static CMuHelperWorkLoop inst;
    return inst;
}

CMuHelperWorkLoop::CMuHelperWorkLoop()
    : m_bActive(false)
    , m_iCurrentTarget(-1)
    , m_iCurrentItem(ITEMS_MAX)
    , m_dwCurrentSkill(0)
    , m_iComboState(0)
    , m_iCurrentBuffIndex(0)
    , m_iCurrentBuffPartyIndex(0)
    , m_iCurrentHealPartyIndex(0)
    , m_iHuntingDistance(0)
    , m_iObtainingDistance(0)
    , m_iLoopCounter(0)
    , m_iSecondsElapsed(0)
    , m_iSecondsAway(0)
    , m_iTotalCost(0)
    , m_bTimerActivatedBuffOngoing(false)
    , m_bPetActivated(false)
    , m_posOrigX(0)
    , m_posOrigY(0)
    , m_nTargets(0)
    , m_nItems(0)
{
    MuHelperWorkConfig_Init(&m_config);
    memset(m_targets, -1, sizeof(m_targets));
    memset(m_targetsAttacking, 0, sizeof(m_targetsAttacking));
    memset(m_items, -1, sizeof(m_items));
    memset(m_itemX, 0, sizeof(m_itemX));
    memset(m_itemY, 0, sizeof(m_itemY));
    InitializeCriticalSection(&m_csTargets);
    InitializeCriticalSection(&m_csItems);
}

void CMuHelperWorkLoop::Init()
{
    MuHelperWorkConfig_Init(&m_config);
}

// ── Timer callback (~200 ms interval = 5 ticks/sec) ─────────
void CALLBACK CMuHelperWorkLoop::TimerProc(
    HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
    CMuHelperWorkLoop::Instance().WorkLoop();
}

// ── Start / Stop / Toggle ────────────────────────────────────
void CMuHelperWorkLoop::Start()
{
    if (m_bActive) return;

    m_iTotalCost = 0;
    m_iComboState = 0;
    m_iCurrentBuffIndex = 0;
    m_iCurrentBuffPartyIndex = 0;
    m_iCurrentHealPartyIndex = 0;
    m_iCurrentTarget = -1;
    m_dwCurrentSkill = m_config.aiSkill[0];
    m_iCurrentItem = ITEMS_MAX;

    // Store original position for regroup
    // In a real integration these come from the Hero global
    m_posOrigX = 0;
    m_posOrigY = 0;

    m_iHuntingDistance   = ComputeDistanceByRange(m_config.iHuntingRange);
    m_iObtainingDistance = ComputeDistanceByRange(m_config.iObtainingRange);

    m_iSecondsElapsed = 0;
    m_iSecondsAway = 0;
    m_bTimerActivatedBuffOngoing = false;
    m_bPetActivated = false;
    m_iLoopCounter = 0;

    m_bActive = true;
}

void CMuHelperWorkLoop::Stop()
{
    m_bActive = false;
}

void CMuHelperWorkLoop::Toggle()
{
    if (m_bActive) Stop(); else Start();
}

// ── Work loop (called every ~200 ms) ─────────────────────────
void CMuHelperWorkLoop::WorkLoop()
{
    if (!m_bActive) return;

    Work();

    if (++m_iLoopCounter >= 5)
    {
        m_iSecondsElapsed++;
        // Track time away from original position (for regroup)
        m_iLoopCounter = 0;
    }
}

void CMuHelperWorkLoop::Work()
{
    // Sequence matches MuMain:
    //   ActivatePet → Buff → RecoverHealth → ObtainItem →
    //   Regroup → Attack → RepairEquipments
    // Each step returns 0 if it wants to block the rest.
    if (!ActivatePet())    return;
    if (!Buff())           return;
    if (!RecoverHealth())  return;
    if (!ObtainItem())     return;
    if (!Regroup())        return;
    Attack();
    RepairEquipments();
}

// ── Target management ────────────────────────────────────────
void CMuHelperWorkLoop::AddTarget(int id, bool bIsAttacking)
{
    if (!m_bActive) return;

    CSGuard lock(m_csTargets);
    // Check if already tracked
    for (int i = 0; i < m_nTargets; i++)
        if (m_targets[i] == id)
        {
            if (bIsAttacking) m_targetsAttacking[i] = true;
            return;
        }
    if (m_nTargets < MAX_TRACKED)
    {
        m_targets[m_nTargets] = id;
        m_targetsAttacking[m_nTargets] = bIsAttacking;
        m_nTargets++;
    }
}

void CMuHelperWorkLoop::DeleteTarget(int id)
{
    CSGuard lock(m_csTargets);
    for (int i = 0; i < m_nTargets; i++)
    {
        if (m_targets[i] == id)
        {
            m_targets[i] = m_targets[m_nTargets - 1];
            m_targetsAttacking[i] = m_targetsAttacking[m_nTargets - 1];
            m_nTargets--;
            break;
        }
    }
    if (id == m_iCurrentTarget)
        m_iCurrentTarget = -1;
}

void CMuHelperWorkLoop::DeleteAllTargets()
{
    CSGuard lock(m_csTargets);
    m_nTargets = 0;
}

// ── Item management ──────────────────────────────────────────
void CMuHelperWorkLoop::AddItem(int id, int x, int y)
{
    CSGuard lock(m_csItems);
    for (int i = 0; i < m_nItems; i++)
        if (m_items[i] == id) return;
    if (m_nItems < MAX_TRACKED)
    {
        m_items[m_nItems] = id;
        m_itemX[m_nItems] = x;
        m_itemY[m_nItems] = y;
        m_nItems++;
    }
}

void CMuHelperWorkLoop::DeleteItem(int id)
{
    CSGuard lock(m_csItems);
    for (int i = 0; i < m_nItems; i++)
    {
        if (m_items[i] == id)
        {
            m_items[i] = m_items[m_nItems - 1];
            m_itemX[i] = m_itemX[m_nItems - 1];
            m_itemY[i] = m_itemY[m_nItems - 1];
            m_nItems--;
            break;
        }
    }
    if (id == m_iCurrentItem)
        m_iCurrentItem = ITEMS_MAX;
}

// ── Distance helpers ─────────────────────────────────────────
int CMuHelperWorkLoop::ComputeDistanceByRange(int range)
{
    return ComputeDistanceBetween(0, 0, range, range);
}

int CMuHelperWorkLoop::ComputeDistanceBetween(int x1, int y1, int x2, int y2)
{
    int dx = x1 - x2;
    int dy = y1 - y2;
    return (int)ceil(sqrt((double)(dx * dx + dy * dy)));
}

// ── ActivatePet (Dark Spirit / Dark Raven) ───────────────────
// Ported from MuMain: CMuHelper::ActivatePet()
int CMuHelperWorkLoop::ActivatePet()
{
    if (!m_config.bUseDarkRaven) return 1;
    if (m_bPetActivated) return 1;

    // In a real integration, send pet command packet here:
    // SendPetCommandRequest(PET_TYPE_DARK_SPIRIT, mode, 0xFFFF)
    m_bPetActivated = true;
    return 1;
}

// ── Buff (self / party) ──────────────────────────────────────
// Ported from MuMain: CMuHelper::Buff()
int CMuHelperWorkLoop::Buff()
{
    bool hasBuff = false;
    for (int i = 0; i < 3; i++)
        if (m_config.aiBuff[i] != 0) { hasBuff = true; break; }
    if (!hasBuff) return 1;

    // Timer-activated buff check
    if (!m_config.bBuffDuration
        && m_config.iBuffCastInterval != 0
        && m_iSecondsElapsed > 0
        && (m_iSecondsElapsed % m_config.iBuffCastInterval) == 0)
    {
        m_bTimerActivatedBuffOngoing = true;
    }

    // Cycle through buff slots
    m_iCurrentBuffIndex = (m_iCurrentBuffIndex + 1) % 3;
    if (m_iCurrentBuffIndex == 0)
        m_bTimerActivatedBuffOngoing = false;

    return 1;
}

// ── Recover Health (Heal + DrainLife + Potion) ───────────────
// Ported from MuMain: CMuHelper::RecoverHealth()
int CMuHelperWorkLoop::RecoverHealth()
{
    if (!Heal())        return 0;
    if (!DrainLife())   return 0;
    if (!ConsumePotion()) return 0;
    return 1;
}

int CMuHelperWorkLoop::Heal()
{
    if (!m_config.bAutoHeal) return 1;
    // In a real integration: check HP%, cast heal skill if below threshold
    return 1;
}

int CMuHelperWorkLoop::DrainLife()
{
    if (!m_config.bUseDrainLife) return 1;
    // In a real integration: check HP%, cast drain life on nearest target
    return 1;
}

int CMuHelperWorkLoop::ConsumePotion()
{
    if (!m_config.bUseHealPotion) return 1;
    // In a real integration: check HP%, use potion item from inventory
    return 1;
}

// ── Attack ───────────────────────────────────────────────────
// Ported from MuMain: CMuHelper::Attack()
int CMuHelperWorkLoop::Attack()
{
    if (m_iCurrentTarget == -1)
    {
        if (m_nTargets > 0)
        {
            CleanupTargets();

            if (m_config.bLongRangeCounterAttack)
                m_iCurrentTarget = GetFarthestAttackingTarget();

            if (m_iCurrentTarget == -1)
                m_iCurrentTarget = GetNearestTarget();
        }
        else
        {
            m_iComboState = 0;
            return 0;
        }
    }

    if (m_config.bUseCombo)
        return SimulateComboAttack();

    m_dwCurrentSkill = SelectAttackSkill();
    if (m_dwCurrentSkill != 0)
        SimulateAttack(m_dwCurrentSkill);

    return 1;
}

// ── SelectAttackSkill ────────────────────────────────────────
// Ported from MuMain: CMuHelper::SelectAttackSkill()
DWORD CMuHelperWorkLoop::SelectAttackSkill()
{
    // Try skill 2 conditions
    if (m_config.aiSkill[1] > 0)
    {
        if ((m_config.aiSkillCondition[1] & SkillCond::ON_TIMER)
            && m_config.aiSkillInterval[1] != 0
            && m_iSecondsElapsed > 0
            && (DWORD)m_iSecondsElapsed % m_config.aiSkillInterval[1] == 0)
        {
            return m_config.aiSkill[1];
        }

        if (m_config.aiSkillCondition[1] & SkillCond::ON_CONDITION)
        {
            int cnt = m_nTargets;
            if ((m_config.aiSkillCondition[1] & SkillCond::ON_MORE_THAN_TWO)   && cnt >= 2) return m_config.aiSkill[1];
            if ((m_config.aiSkillCondition[1] & SkillCond::ON_MORE_THAN_THREE) && cnt >= 3) return m_config.aiSkill[1];
            if ((m_config.aiSkillCondition[1] & SkillCond::ON_MORE_THAN_FOUR)  && cnt >= 4) return m_config.aiSkill[1];
            if ((m_config.aiSkillCondition[1] & SkillCond::ON_MORE_THAN_FIVE)  && cnt >= 5) return m_config.aiSkill[1];
        }
    }

    // Try skill 3 conditions
    if (m_config.aiSkill[2] > 0)
    {
        if ((m_config.aiSkillCondition[2] & SkillCond::ON_TIMER)
            && m_config.aiSkillInterval[2] != 0
            && m_iSecondsElapsed > 0
            && (DWORD)m_iSecondsElapsed % m_config.aiSkillInterval[2] == 0)
        {
            return m_config.aiSkill[2];
        }

        if (m_config.aiSkillCondition[2] & SkillCond::ON_CONDITION)
        {
            int cnt = m_nTargets;
            if ((m_config.aiSkillCondition[2] & SkillCond::ON_MORE_THAN_TWO)   && cnt >= 2) return m_config.aiSkill[2];
            if ((m_config.aiSkillCondition[2] & SkillCond::ON_MORE_THAN_THREE) && cnt >= 3) return m_config.aiSkill[2];
            if ((m_config.aiSkillCondition[2] & SkillCond::ON_MORE_THAN_FOUR)  && cnt >= 4) return m_config.aiSkill[2];
            if ((m_config.aiSkillCondition[2] & SkillCond::ON_MORE_THAN_FIVE)  && cnt >= 5) return m_config.aiSkill[2];
        }
    }

    // Default: basic skill
    if (m_config.aiSkill[0] > 0)
        return m_config.aiSkill[0];

    return 0;
}

// ── SimulateAttack / SimulateComboAttack ─────────────────────
int CMuHelperWorkLoop::SimulateAttack(DWORD skill)
{
    // In a real integration: set movement skill, find target,
    // pathfind, execute skill via game functions
    (void)skill;
    return 1;
}

int CMuHelperWorkLoop::SimulateComboAttack()
{
    for (int i = 0; i < 3; i++)
        if (m_config.aiSkill[i] == 0) return 0;

    if (SimulateAttack(m_config.aiSkill[m_iComboState]))
        m_iComboState = (m_iComboState + 1) % 3;

    return 1;
}

// ── Target lookup ────────────────────────────────────────────
int CMuHelperWorkLoop::GetNearestTarget()
{
    // Placeholder: in real integration, iterate targets and
    // compute distances from Hero position
    CSGuard lock(m_csTargets);
    if (m_nTargets > 0) return m_targets[0];
    return -1;
}

int CMuHelperWorkLoop::GetFarthestAttackingTarget()
{
    CSGuard lock(m_csTargets);
    for (int i = m_nTargets - 1; i >= 0; i--)
        if (m_targetsAttacking[i]) return m_targets[i];
    return -1;
}

void CMuHelperWorkLoop::CleanupTargets()
{
    // In real integration: remove dead/out-of-range targets
}

// ── RepairEquipments ─────────────────────────────────────────
// Ported from MuMain: CMuHelper::RepairEquipments()
int CMuHelperWorkLoop::RepairEquipments()
{
    if (!m_config.bRepairItem) return 1;
    // In real integration: iterate equipment, check durability,
    // send repair request if below threshold
    return 1;
}

// ── Regroup (return to original position) ────────────────────
// Ported from MuMain: CMuHelper::Regroup()
int CMuHelperWorkLoop::Regroup()
{
    if (m_config.bReturnToOriginalPosition
        && m_iSecondsAway > m_config.iMaxSecondsAway)
    {
        // Move back to m_posOrigX, m_posOrigY
        m_iSecondsAway = 0;
        m_iComboState  = 0;
        m_iCurrentTarget = -1;
    }
    return 1;
}

// ── ObtainItem (pickup) ──────────────────────────────────────
// Ported from MuMain: CMuHelper::ObtainItem()
int CMuHelperWorkLoop::ObtainItem()
{
    if (m_iCurrentItem == ITEMS_MAX)
    {
        m_iCurrentItem = SelectItemToObtain();
        if (m_iCurrentItem == ITEMS_MAX) return 1;
    }

    // In real integration: walk to item, send pickup request
    // For now, just consume the item ID
    DeleteItem(m_iCurrentItem);
    return 1;
}

bool CMuHelperWorkLoop::ShouldObtainItem(int iItemId)
{
    (void)iItemId;
    // In real integration: check item properties against config
    // (zen, jewels, excellent, ancient, extra items, etc.)
    return m_config.bPickAllItems;
}

int CMuHelperWorkLoop::SelectItemToObtain()
{
    CSGuard lock(m_csItems);
    int closest = ITEMS_MAX;
    // In real integration: find nearest item within obtaining range
    if (m_nItems > 0 && ShouldObtainItem(m_items[0]))
        closest = m_items[0];
    return closest;
}

// ── Serialization: WorkConfig ↔ MUHELPER_NET_DATA ────────────
// Ported from MuMain: ConfigDataSerDe::Serialize/Deserialize
void MuHelperWorkConfig_ToNet(const MuHelperWorkConfig& cfg,
                              MUHELPER_NET_DATA& net)
{
    memset(&net, 0, sizeof(net));

    net.HuntingRange       = (BYTE)(cfg.iHuntingRange & 0x0F);
    net.ObtainRange        = (BYTE)(cfg.iObtainingRange & 0x0F);
    net.DistanceMin        = (WORD)(cfg.iMaxSecondsAway & 0x0F);
    net.LongDistanceAttack = cfg.bLongRangeCounterAttack ? 1 : 0;
    net.OriginalPosition   = cfg.bReturnToOriginalPosition ? 1 : 0;

    net.BasicSkill1       = (WORD)(cfg.aiSkill[0] & 0xFFFF);
    net.ActivationSkill1  = (WORD)(cfg.aiSkill[1] & 0xFFFF);
    net.ActivationSkill2  = (WORD)(cfg.aiSkill[2] & 0xFFFF);
    net.DelayMinSkill1    = (WORD)(cfg.aiSkillInterval[1] & 0xFFFF);
    net.DelayMinSkill2    = (WORD)(cfg.aiSkillInterval[2] & 0xFFFF);

    // Skill 1 conditions
    net.Skill1Delay  = (cfg.aiSkillCondition[1] & SkillCond::ON_TIMER)     ? 1 : 0;
    net.Skill1Con    = (cfg.aiSkillCondition[1] & SkillCond::ON_CONDITION)  ? 1 : 0;
    net.Skill1PreCon = (cfg.aiSkillCondition[1] & SkillCond::ON_MOBS_ATTACKING) ? 1 : 0;
    if      (cfg.aiSkillCondition[1] & SkillCond::ON_MORE_THAN_FIVE)  net.Skill1SubCon = 3;
    else if (cfg.aiSkillCondition[1] & SkillCond::ON_MORE_THAN_FOUR)  net.Skill1SubCon = 2;
    else if (cfg.aiSkillCondition[1] & SkillCond::ON_MORE_THAN_THREE) net.Skill1SubCon = 1;
    else                                                               net.Skill1SubCon = 0;

    // Skill 2 conditions
    net.Skill2Delay  = (cfg.aiSkillCondition[2] & SkillCond::ON_TIMER)     ? 1 : 0;
    net.Skill2Con    = (cfg.aiSkillCondition[2] & SkillCond::ON_CONDITION)  ? 1 : 0;
    net.Skill2PreCon = (cfg.aiSkillCondition[2] & SkillCond::ON_MOBS_ATTACKING) ? 1 : 0;
    if      (cfg.aiSkillCondition[2] & SkillCond::ON_MORE_THAN_FIVE)  net.Skill2SubCon = 3;
    else if (cfg.aiSkillCondition[2] & SkillCond::ON_MORE_THAN_FOUR)  net.Skill2SubCon = 2;
    else if (cfg.aiSkillCondition[2] & SkillCond::ON_MORE_THAN_THREE) net.Skill2SubCon = 1;
    else                                                               net.Skill2SubCon = 0;

    net.Combo = cfg.bUseCombo ? 1 : 0;

    net.BuffSkill0NumberID = (WORD)(cfg.aiBuff[0] & 0xFFFF);
    net.BuffSkill1NumberID = (WORD)(cfg.aiBuff[1] & 0xFFFF);
    net.BuffSkill2NumberID = (WORD)(cfg.aiBuff[2] & 0xFFFF);
    net.BuffDuration       = cfg.bBuffDuration ? 1 : 0;
    net.BuffDurationforAllPartyMembers = cfg.bBuffDurationParty ? 1 : 0;
    net.CastingBuffMin     = (WORD)cfg.iBuffCastInterval;

    net.AutoHeal           = cfg.bAutoHeal ? 1 : 0;
    net.HPStatusAutoHeal   = (BYTE)((cfg.iHealThreshold / 10) & 0x0F);
    net.AutoPotion         = cfg.bUseHealPotion ? 1 : 0;
    net.HPStatusAutoPotion = (BYTE)((cfg.iPotionThreshold / 10) & 0x0F);
    net.DrainLife          = cfg.bUseDrainLife ? 1 : 0;
    net.Party              = cfg.bSupportParty ? 1 : 0;
    net.PreferenceOfPartyHeal = cfg.bAutoHealParty ? 1 : 0;
    net.HPStatusOfPartyMembers = (BYTE)((cfg.iHealPartyThreshold / 10) & 0x0F);
    net.HPStatusDrainLife  = (BYTE)((cfg.iHealThreshold / 10) & 0x0F);

    net.UseDarkSpirits     = cfg.bUseDarkRaven ? 1 : 0;
    net.PetAttack          = (BYTE)cfg.iDarkRavenMode;
    net.RepairItem         = cfg.bRepairItem ? 1 : 0;

    net.PickAllNearItems   = cfg.bPickAllItems ? 1 : 0;
    net.PickSelectedItems  = cfg.bPickSelectItems ? 1 : 0;
    net.JewelOrGem         = cfg.bPickJewel ? 1 : 0;
    net.Zen                = cfg.bPickZen ? 1 : 0;
    net.ExcellentItem      = cfg.bPickExcellent ? 1 : 0;
    net.SetItem            = cfg.bPickAncient ? 1 : 0;
    net.AddExtraItem       = cfg.bPickExtraItems ? 1 : 0;
}

void MuHelperWorkConfig_FromNet(const MUHELPER_NET_DATA& net,
                                MuHelperWorkConfig& cfg)
{
    memset(&cfg, 0, sizeof(cfg));

    cfg.iHuntingRange              = (int)net.HuntingRange;
    cfg.iObtainingRange            = (int)net.ObtainRange;
    cfg.iMaxSecondsAway            = (int)net.DistanceMin;
    cfg.bLongRangeCounterAttack    = (net.LongDistanceAttack != 0);
    cfg.bReturnToOriginalPosition  = (net.OriginalPosition != 0);

    cfg.aiSkill[0]        = (DWORD)net.BasicSkill1;
    cfg.aiSkill[1]        = (DWORD)net.ActivationSkill1;
    cfg.aiSkill[2]        = (DWORD)net.ActivationSkill2;
    cfg.aiSkillInterval[0]= 0;
    cfg.aiSkillInterval[1]= (DWORD)net.DelayMinSkill1;
    cfg.aiSkillInterval[2]= (DWORD)net.DelayMinSkill2;

    // Skill 1 conditions
    cfg.aiSkillCondition[1] = 0;
    if (net.Skill1Delay) cfg.aiSkillCondition[1] |= SkillCond::ON_TIMER;
    if (net.Skill1Con)   cfg.aiSkillCondition[1] |= SkillCond::ON_CONDITION;
    cfg.aiSkillCondition[1] |= (net.Skill1PreCon == 0)
        ? SkillCond::ON_MOBS_NEARBY : SkillCond::ON_MOBS_ATTACKING;
    switch (net.Skill1SubCon)
    {
    case 0: cfg.aiSkillCondition[1] |= SkillCond::ON_MORE_THAN_TWO;   break;
    case 1: cfg.aiSkillCondition[1] |= SkillCond::ON_MORE_THAN_THREE; break;
    case 2: cfg.aiSkillCondition[1] |= SkillCond::ON_MORE_THAN_FOUR;  break;
    case 3: cfg.aiSkillCondition[1] |= SkillCond::ON_MORE_THAN_FIVE;  break;
    }

    // Skill 2 conditions
    cfg.aiSkillCondition[2] = 0;
    if (net.Skill2Delay) cfg.aiSkillCondition[2] |= SkillCond::ON_TIMER;
    if (net.Skill2Con)   cfg.aiSkillCondition[2] |= SkillCond::ON_CONDITION;
    cfg.aiSkillCondition[2] |= (net.Skill2PreCon == 0)
        ? SkillCond::ON_MOBS_NEARBY : SkillCond::ON_MOBS_ATTACKING;
    switch (net.Skill2SubCon)
    {
    case 0: cfg.aiSkillCondition[2] |= SkillCond::ON_MORE_THAN_TWO;   break;
    case 1: cfg.aiSkillCondition[2] |= SkillCond::ON_MORE_THAN_THREE; break;
    case 2: cfg.aiSkillCondition[2] |= SkillCond::ON_MORE_THAN_FOUR;  break;
    case 3: cfg.aiSkillCondition[2] |= SkillCond::ON_MORE_THAN_FIVE;  break;
    }

    cfg.bUseCombo = (net.Combo != 0);

    cfg.aiBuff[0] = (DWORD)net.BuffSkill0NumberID;
    cfg.aiBuff[1] = (DWORD)net.BuffSkill1NumberID;
    cfg.aiBuff[2] = (DWORD)net.BuffSkill2NumberID;
    cfg.bBuffDuration      = (net.BuffDuration != 0);
    cfg.bBuffDurationParty = (net.BuffDurationforAllPartyMembers != 0);
    cfg.iBuffCastInterval  = (int)net.CastingBuffMin;

    cfg.bAutoHeal           = (net.AutoHeal != 0);
    cfg.iHealThreshold      = (int)net.HPStatusAutoHeal * 10;
    cfg.bUseHealPotion      = (net.AutoPotion != 0);
    cfg.iPotionThreshold    = (int)net.HPStatusAutoPotion * 10;
    cfg.bUseDrainLife       = (net.DrainLife != 0);
    cfg.bSupportParty       = (net.Party != 0);
    cfg.bAutoHealParty      = (net.PreferenceOfPartyHeal != 0);
    cfg.iHealPartyThreshold = (int)net.HPStatusOfPartyMembers * 10;

    cfg.bUseDarkRaven  = (net.UseDarkSpirits != 0);
    cfg.iDarkRavenMode = (int)net.PetAttack;
    cfg.bRepairItem    = (net.RepairItem != 0);

    cfg.bPickAllItems    = (net.PickAllNearItems != 0);
    cfg.bPickSelectItems = (net.PickSelectedItems != 0);
    cfg.bPickJewel       = (net.JewelOrGem != 0);
    cfg.bPickZen         = (net.Zen != 0);
    cfg.bPickExcellent   = (net.ExcellentItem != 0);
    cfg.bPickAncient     = (net.SetItem != 0);
    cfg.bPickExtraItems  = (net.AddExtraItem != 0);
}
