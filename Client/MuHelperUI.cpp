// ============================================================
//  MuHelper v2 — ImGui OpenGL Overlay  (MuHelperUI.cpp)
//
//  Dependencies (drop into project/imgui subfolder):
//    imgui.h/.cpp, imgui_draw.cpp, imgui_tables.cpp,
//    imgui_widgets.cpp, imgui_impl_opengl2.h/.cpp,
//    imgui_impl_win32.h/.cpp
//    https://github.com/ocornut/imgui  tag v1.90.x
//
//  Link: opengl32.lib
// ============================================================
#include "MuHelperUI.h"
#include "MuHelperClient.h"
#include "HookEngine.h"
#include "../Shared/MuHelperPackets.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_opengl2.h"

#include <GL/gl.h>
#include <algorithm>
#include <cstdio>
#include <cstring>

#pragma comment(lib, "opengl32.lib")

// ============================================================
//  Colour palette  —  dark MuOnline fantasy aesthetic
// ============================================================
namespace Mu
{
    static ImVec4 BG_DEEP     {0.04f,0.04f,0.09f,0.96f};
    static ImVec4 BG_PANEL    {0.07f,0.07f,0.14f,1.00f};
    static ImVec4 BG_HEADER   {0.10f,0.09f,0.20f,1.00f};
    static ImVec4 BG_ITEM     {0.09f,0.09f,0.16f,1.00f};
    static ImVec4 BG_ITEM_HOV {0.14f,0.13f,0.25f,1.00f};
    static ImVec4 GOLD        {1.00f,0.80f,0.20f,1.00f};
    static ImVec4 GOLD_DIM    {0.70f,0.56f,0.14f,1.00f};
    static ImVec4 BLUE        {0.30f,0.70f,1.00f,1.00f};
    static ImVec4 GREEN       {0.20f,0.90f,0.40f,1.00f};
    static ImVec4 RED         {1.00f,0.25f,0.20f,1.00f};
    static ImVec4 CYAN        {0.20f,0.90f,0.90f,1.00f};
    static ImVec4 PURPLE      {0.65f,0.30f,1.00f,1.00f};
    static ImVec4 ORANGE      {1.00f,0.55f,0.10f,1.00f};
    static ImVec4 GRAY        {0.55f,0.55f,0.60f,1.00f};
    static ImVec4 WHITE       {0.92f,0.92f,0.95f,1.00f};
    // Bars
    static ImVec4 BAR_HP_LO   {0.75f,0.12f,0.12f,1.00f};
    static ImVec4 BAR_HP_HI   {1.00f,0.30f,0.20f,1.00f};
    static ImVec4 BAR_MP_LO   {0.10f,0.30f,0.90f,1.00f};
    static ImVec4 BAR_MP_HI   {0.30f,0.60f,1.00f,1.00f};
    static ImVec4 BAR_SD      {0.15f,0.65f,0.80f,1.00f};
    static ImVec4 BAR_EXP_LO  {0.45f,0.15f,0.70f,1.00f};
    static ImVec4 BAR_EXP_HI  {0.75f,0.30f,1.00f,1.00f};
    static ImVec4 BAR_CD      {0.70f,0.45f,0.10f,1.00f};
    // Item quality
    static ImVec4 IT_NORM     {0.80f,0.80f,0.80f,1.00f};
    static ImVec4 IT_EXC      {0.30f,0.80f,1.00f,1.00f};
    static ImVec4 IT_ANC      {1.00f,0.75f,0.20f,1.00f};
    static ImVec4 IT_SOC      {0.55f,1.00f,0.55f,1.00f};
    static ImVec4 IT_ZEN      {1.00f,0.85f,0.10f,1.00f};
}

static ImU32 V4(ImVec4 v, float a = 1.0f)
{
    return IM_COL32((int)(v.x*255),(int)(v.y*255),(int)(v.z*255),(int)(v.w*a*255));
}

// ============================================================
//  Style
// ============================================================
static void ApplyStyle()
{
    ImGuiStyle& s = ImGui::GetStyle();
    s.WindowRounding  = 7.0f;  s.FrameRounding   = 4.0f;
    s.PopupRounding   = 5.0f;  s.TabRounding     = 5.0f;
    s.GrabRounding    = 3.0f;  s.ScrollbarRounding=4.0f;
    s.WindowBorderSize= 1.0f;  s.FrameBorderSize  = 0.0f;
    s.ItemSpacing     = {8.0f,5.0f}; s.FramePadding = {6.0f,4.0f};
    s.WindowPadding   = {10.0f,10.0f}; s.ScrollbarSize= 10.0f;
    s.GrabMinSize     = 8.0f;  s.IndentSpacing    = 16.0f;

    ImVec4* c = s.Colors;
    c[ImGuiCol_WindowBg]          = Mu::BG_DEEP;
    c[ImGuiCol_ChildBg]           = Mu::BG_PANEL;
    c[ImGuiCol_PopupBg]           = Mu::BG_PANEL;
    c[ImGuiCol_Border]            = {0.25f,0.22f,0.50f,0.80f};
    c[ImGuiCol_FrameBg]           = Mu::BG_ITEM;
    c[ImGuiCol_FrameBgHovered]    = Mu::BG_ITEM_HOV;
    c[ImGuiCol_FrameBgActive]     = {0.20f,0.18f,0.38f,1.00f};
    c[ImGuiCol_TitleBg]           = Mu::BG_HEADER;
    c[ImGuiCol_TitleBgActive]     = {0.14f,0.12f,0.28f,1.00f};
    c[ImGuiCol_CheckMark]         = Mu::GOLD;
    c[ImGuiCol_SliderGrab]        = Mu::GOLD_DIM;
    c[ImGuiCol_SliderGrabActive]  = Mu::GOLD;
    c[ImGuiCol_Button]            = {0.16f,0.14f,0.32f,1.00f};
    c[ImGuiCol_ButtonHovered]     = {0.26f,0.22f,0.52f,1.00f};
    c[ImGuiCol_ButtonActive]      = {0.35f,0.28f,0.68f,1.00f};
    c[ImGuiCol_Header]            = {0.20f,0.16f,0.42f,1.00f};
    c[ImGuiCol_HeaderHovered]     = {0.28f,0.22f,0.56f,1.00f};
    c[ImGuiCol_HeaderActive]      = {0.35f,0.28f,0.68f,1.00f};
    c[ImGuiCol_Separator]         = {0.22f,0.20f,0.44f,0.80f};
    c[ImGuiCol_Tab]               = {0.12f,0.10f,0.24f,1.00f};
    c[ImGuiCol_TabHovered]        = {0.26f,0.22f,0.52f,1.00f};
    c[ImGuiCol_TabActive]         = {0.20f,0.16f,0.42f,1.00f};
    c[ImGuiCol_TabUnfocused]      = {0.09f,0.08f,0.17f,1.00f};
    c[ImGuiCol_TabUnfocusedActive]= {0.16f,0.13f,0.32f,1.00f};
    c[ImGuiCol_ScrollbarBg]       = {0.04f,0.04f,0.08f,0.80f};
    c[ImGuiCol_ScrollbarGrab]     = {0.25f,0.22f,0.48f,1.00f};
    c[ImGuiCol_ScrollbarGrabHovered]={0.35f,0.30f,0.65f,1.00f};
    c[ImGuiCol_Text]              = Mu::WHITE;
    c[ImGuiCol_TextDisabled]      = Mu::GRAY;
    c[ImGuiCol_ResizeGrip]        = {0.25f,0.22f,0.50f,0.50f};
    c[ImGuiCol_ResizeGripHovered] = {0.40f,0.35f,0.75f,0.75f};
    c[ImGuiCol_ResizeGripActive]  = Mu::GOLD_DIM;
    c[ImGuiCol_PlotLines]         = Mu::GOLD_DIM;
    c[ImGuiCol_PlotHistogram]     = {0.30f,0.55f,0.90f,1.00f};
}

// ============================================================
//  Helper widgets
// ============================================================
static void GBar(float frac,
                 ImVec4 cLo, ImVec4 cHi,
                 float w, float h, const char* ov = nullptr)
{
    frac = std::max(0.0f, std::min(1.0f, frac));
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImVec2 sz{w < 0 ? ImGui::GetContentRegionAvail().x : w, h};
    ImDrawList* dl = ImGui::GetWindowDrawList();

    dl->AddRectFilled(p, {p.x+sz.x,p.y+sz.y}, IM_COL32(20,18,36,200), 3.f);
    float fw = sz.x * frac;
    if (fw > 2.f)
    {
        dl->AddRectFilledMultiColor(p, {p.x+fw,p.y+sz.y},
            V4(cLo),V4(cHi),V4(cHi,0.85f),V4(cLo,0.85f));
        dl->AddRectFilled(p,{p.x+fw,p.y+sz.y*0.35f},IM_COL32(255,255,255,18),0.f);
    }
    dl->AddRect(p,{p.x+sz.x,p.y+sz.y},IM_COL32(60,52,120,180),3.f,0,1.f);
    if (ov)
    {
        ImVec2 ts = ImGui::CalcTextSize(ov);
        dl->AddText({p.x+(sz.x-ts.x)*.5f,p.y+(sz.y-ts.y)*.5f},
                    IM_COL32(230,230,230,220),ov);
    }
    ImGui::Dummy(sz);
}

static void SecHead(const char* t, ImVec4 c = {1.f,.8f,.2f,1.f})
{
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Text, c);
    ImGui::TextUnformatted(t);
    ImGui::PopStyleColor();
    ImGui::Separator();
    ImGui::Spacing();
}

static bool SkillCombo(const char* id, BYTE* slot)
{
    const char* items[] = {"Slot 1","Slot 2","Slot 3","Slot 4","Slot 5","Slot 6","Slot 7","Slot 8","(none)"};
    int cur = (*slot==0xFF) ? 8 : (int)*slot;
    bool changed = ImGui::Combo(id, &cur, items, 9);
    if (changed) *slot = (cur==8) ? 0xFF : (BYTE)cur;
    return changed;
}

// ============================================================
//  Singleton
// ============================================================
CMuHelperUI& CMuHelperUI::Instance()
{
    static CMuHelperUI inst;
    return inst;
}

// ============================================================
//  Init
// ============================================================
void CMuHelperUI::Init(HWND hWnd)
{
    m_hWnd = hWnd;
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename  = "MuHelper.ini";
    io.Fonts->AddFontDefault();
    ImFontConfig fc; fc.SizePixels = 13.f;
    m_pFontNormal = io.Fonts->AddFontDefault(&fc);
    fc.SizePixels = 11.f; m_pFontSmall = io.Fonts->AddFontDefault(&fc);
    fc.SizePixels = 15.f; m_pFontTitle = io.Fonts->AddFontDefault(&fc);
    ApplyStyle();
    ImGui_ImplWin32_Init(hWnd);
    ImGui_ImplOpenGL2_Init();
    m_editCfg = CMuHelperClient::Instance().GetConfig();
    CMuHelperClient::Instance().SetOnCfgReply([this](const MuHelperConfig& c){
        m_editCfg = c; m_bDirty = false;
    });
    m_bInited = true;
}

void CMuHelperUI::ToggleVisible()
{
    m_bVisible = !m_bVisible;
    if (m_bVisible) m_editCfg = CMuHelperClient::Instance().GetConfig();
}

// ============================================================
//  Render
// ============================================================
void CMuHelperUI::Render(HDC /*hdc*/)
{
    if (!m_bInited) return;
    ImGui_ImplOpenGL2_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    if (m_bVisible) DrawMainPanel();
    if (CMuHelperClient::Instance().GetConfig().bShowStatOverlay)
        DrawStatOverlay();

    ImGui::Render();
    ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
}

// ============================================================
//  Stat overlay (always visible mini-HUD)
// ============================================================
void CMuHelperUI::DrawStatOverlay()
{
    auto& cl = CMuHelperClient::Instance();
    auto& st = cl.GetStats();
    bool run = cl.IsRunning();

    ImGuiWindowFlags fl =
        ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoInputs|
        ImGuiWindowFlags_NoNav|ImGuiWindowFlags_NoMove|
        ImGuiWindowFlags_NoBringToFrontOnFocus|
        ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_AlwaysAutoResize;

    ImGui::SetNextWindowPos({8.f,8.f}, ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.80f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,{8.f,6.f});
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,{4.f,3.f});

    if (ImGui::Begin("##ov", nullptr, fl))
    {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 p = ImGui::GetCursorScreenPos();
        ImVec4 dc = run ? Mu::GREEN : Mu::RED;
        dl->AddCircleFilled({p.x+6,p.y+7},4.5f,V4(dc));
        ImGui::Dummy({14,0}); ImGui::SameLine(0,4);
        ImGui::PushStyleColor(ImGuiCol_Text, run ? Mu::GREEN : Mu::GRAY);
        ImGui::Text("MuHelper %s", run ? "ON" : "OFF");
        ImGui::PopStyleColor();
        if (run)
        {
            ImGui::Separator();
            ImGui::TextColored(Mu::GOLD,  "Kill %u (%u/h)", st.wKillCount, st.wKillsPerHour);
            if (st.dwZenPickup)  ImGui::TextColored(Mu::IT_ZEN, "Zen  %u", st.dwZenPickup);
            if (st.wItemPickup)  ImGui::TextColored(Mu::IT_EXC, "Item %u", st.wItemPickup);
            if (st.wSessionMinutes) ImGui::TextColored(Mu::GRAY,"Time %um", st.wSessionMinutes);
        }
        ImGui::TextColored(Mu::GRAY,"[F10] panel");
    }
    ImGui::End();
    ImGui::PopStyleVar(2);
}

// ============================================================
//  Main panel
// ============================================================
void CMuHelperUI::DrawMainPanel()
{
    bool run = CMuHelperClient::Instance().IsRunning();
    char title[64];
    snprintf(title,64," MuHelper v2   %s###mhpanel", run ? "[ACTIVE]" : "[IDLE]");

    ImGui::SetNextWindowSize({500,560},ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSizeConstraints({420,460},{720,900});
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive,
        run ? ImVec4{0.08f,0.28f,0.10f,1.f} : ImVec4{0.14f,0.12f,0.28f,1.f});

    if (ImGui::Begin(title, &m_bVisible, ImGuiWindowFlags_NoCollapse))
    {
        // ── TOP BAR ──────────────────────────────────────────
        {
            float bw = 108.f;
            if (run)
            {
                ImGui::PushStyleColor(ImGuiCol_Button,        {0.50f,0.09f,0.09f,1.f});
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.80f,0.14f,0.12f,1.f});
                if (ImGui::Button("  STOP  ",{bw,28}))
                    CMuHelperClient::Instance().SendEnable(false);
                ImGui::PopStyleColor(2);
            }
            else
            {
                ImGui::PushStyleColor(ImGuiCol_Button,        {0.09f,0.42f,0.14f,1.f});
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.13f,0.64f,0.20f,1.f});
                if (ImGui::Button("  START  ",{bw,28}))
                    CMuHelperClient::Instance().SendEnable(true);
                ImGui::PopStyleColor(2);
            }
            auto& st = CMuHelperClient::Instance().GetStats();
            ImGui::SameLine(0,12);
            ImGui::TextColored(Mu::GOLD,  "K:%u",  st.wKillCount);  ImGui::SameLine(0,8);
            ImGui::TextColored(Mu::IT_ZEN,"Z:%u",  st.dwZenPickup); ImGui::SameLine(0,8);
            ImGui::TextColored(Mu::IT_EXC,"I:%u",  st.wItemPickup); ImGui::SameLine(0,8);
            ImGui::TextColored(Mu::GRAY,  "%um",   st.wSessionMinutes);

            // Skill cooldown mini-bars (right side)
            float avail = ImGui::GetContentRegionAvail().x;
            if (avail > 160.f)
            {
                float sw = std::min(30.f, avail / 8.f);
                ImGui::SameLine(ImGui::GetWindowWidth() - sw*8 - 14);
                DrawSkillCooldownBar(sw);
            }
        }
        ImGui::Spacing();

        // ── TABS ─────────────────────────────────────────────
        if (ImGui::BeginTabBar("##tb"))
        {
            if (ImGui::BeginTabItem(" Combat "))   { TabCombat();   ImGui::EndTabItem(); }
            if (ImGui::BeginTabItem(" Pickup "))   { TabPickup();   ImGui::EndTabItem(); }
            if (ImGui::BeginTabItem(" Potions "))  { TabPotions();  ImGui::EndTabItem(); }
            if (ImGui::BeginTabItem(" Buffs "))    { TabBuffs();    ImGui::EndTabItem(); }
            if (ImGui::BeginTabItem(" Party "))    { TabParty();    ImGui::EndTabItem(); }
            if (ImGui::BeginTabItem(" Stats "))    { TabStats();    ImGui::EndTabItem(); }
            if (ImGui::BeginTabItem(" Log "))      { TabLog();      ImGui::EndTabItem(); }
            if (ImGui::BeginTabItem(" Profiles ")) { TabProfiles(); ImGui::EndTabItem(); }
            ImGui::EndTabBar();
        }

        // ── SAVE BAR ─────────────────────────────────────────
        if (m_bDirty)
        {
            ImGui::Separator(); ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Button,        {0.18f,0.38f,0.18f,1.f});
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.26f,0.54f,0.26f,1.f});
            if (ImGui::Button("  Apply & Save Config  ",{-1,0}))
            { CMuHelperClient::Instance().SendConfig(m_editCfg); m_bDirty=false; }
            ImGui::PopStyleColor(2);
        }
    }
    ImGui::End();
    ImGui::PopStyleColor();
}

// ── Skill CD bar ──────────────────────────────────────────────
void CMuHelperUI::DrawSkillCooldownBar(float sw)
{
    auto& cl = CMuHelperClient::Instance();
    for (int i = 0; i < 8; i++)
    {
        auto& cd = cl.GetSkillCD(i);
        float frac = 1.f;
        if (cd.wRemMs > 0 && cd.dwLastUpdate)
        {
            DWORD el = GetTickCount() - cd.dwLastUpdate;
            DWORD rem = (cd.wRemMs > el) ? cd.wRemMs - el : 0;
            frac = 1.f - (float)rem / (float)cd.wRemMs;
        }
        char lbl[4]; snprintf(lbl,4,"%d",i+1);
        ImVec2 p = ImGui::GetCursorScreenPos();
        float h = 20.f;
        ImDrawList* dl = ImGui::GetWindowDrawList();
        dl->AddRectFilled(p,{p.x+sw-2,p.y+h},IM_COL32(20,16,40,200),2.f);
        if (frac > 0.01f)
        {
            ImVec4 fc = (frac > 0.99f) ? Mu::GREEN : Mu::BAR_CD;
            dl->AddRectFilled(p,{p.x+(sw-2)*frac,p.y+h},V4(fc,0.85f),2.f);
        }
        dl->AddRect(p,{p.x+sw-2,p.y+h},IM_COL32(60,50,120,180),2.f,0,1.f);
        ImVec2 ts = ImGui::CalcTextSize(lbl);
        dl->AddText({p.x+(sw-2-ts.x)*.5f,p.y+(h-ts.y)*.5f},IM_COL32(200,190,220,220),lbl);
        ImGui::Dummy({sw,h}); ImGui::SameLine(0,0);
    }
    ImGui::NewLine();
}

// ============================================================
//  TAB: COMBAT
// ============================================================
void CMuHelperUI::TabCombat()
{
    ImGui::BeginChild("##cbt",{0,0},false);
    auto& cfg = m_editCfg;
    auto Mark = [&](){ m_bDirty = true; };

    SecHead("Auto Attack");
    {
        bool v = !!(cfg.bCombatMode & COMBAT_MODE_ATTACK);
        if (ImGui::Checkbox("Auto Attack##aa",&v))   { v?cfg.bCombatMode|=COMBAT_MODE_ATTACK:cfg.bCombatMode&=~COMBAT_MODE_ATTACK; Mark(); }
        bool sk = !!(cfg.bCombatMode & COMBAT_MODE_SKILL);
        if (ImGui::Checkbox("Use Skill##sk",&sk))    { sk?cfg.bCombatMode|=COMBAT_MODE_SKILL:cfg.bCombatMode&=~COMBAT_MODE_SKILL; Mark(); }
        if (sk)
        {
            ImGui::Indent();
            ImGui::SetNextItemWidth(140);
            if (SkillCombo("##atksl",&cfg.bAttackSkillSlot)) Mark();
            ImGui::SameLine(0,8); ImGui::TextColored(Mu::GRAY,"Attack skill slot");
            ImGui::Unindent();
        }
        bool cb = !!(cfg.bCombatMode & COMBAT_MODE_COMBO);
        if (ImGui::Checkbox("Use Combo##cb",&cb))    { cb?cfg.bCombatMode|=COMBAT_MODE_COMBO:cfg.bCombatMode&=~COMBAT_MODE_COMBO; Mark(); }
        if (cb)
        {
            ImGui::Indent();
            ImGui::SetNextItemWidth(140);
            if (SkillCombo("##cbsl",&cfg.bComboSkillSlot)) Mark();
            ImGui::SameLine(0,8); ImGui::TextColored(Mu::GRAY,"Combo skill slot");
            ImGui::Unindent();
        }
        bool aoe = !!(cfg.bCombatMode & COMBAT_MODE_AOE);
        if (ImGui::Checkbox("Prefer AoE Skills##aoe",&aoe)) { aoe?cfg.bCombatMode|=COMBAT_MODE_AOE:cfg.bCombatMode&=~COMBAT_MODE_AOE; Mark(); }
    }

    SecHead("Skill Rotation", Mu::CYAN);
    ImGui::TextColored(Mu::GRAY,"Cycle these slots every N attacks:"); ImGui::Spacing();
    ImGui::SetNextItemWidth(110); if (SkillCombo("##r1",&cfg.bRotationSlot1)) Mark(); ImGui::SameLine(0,6);
    ImGui::SetNextItemWidth(110); if (SkillCombo("##r2",&cfg.bRotationSlot2)) Mark(); ImGui::SameLine(0,6);
    ImGui::SetNextItemWidth(110); if (SkillCombo("##r3",&cfg.bRotationSlot3)) Mark();
    ImGui::Spacing();
    int ri = cfg.bRotationInterval;
    ImGui::SetNextItemWidth(220);
    if (ImGui::SliderInt("Interval (attacks)##ri",&ri,1,10)) { cfg.bRotationInterval=(BYTE)ri; Mark(); }

    SecHead("Radius & Misc");
    int ar = cfg.bAttackRadius;
    ImGui::SetNextItemWidth(220);
    if (ImGui::SliderInt("Attack Radius##ar",&ar,1,MUHELPER_MAX_RADIUS)) { cfg.bAttackRadius=(BYTE)ar; Mark(); }
    bool pvp=(cfg.bAvoidPvP!=0); if(ImGui::Checkbox("Avoid PvP##pvp",&pvp)){cfg.bAvoidPvP=pvp?1:0;Mark();}
    bool sl=(cfg.bStopOnLevelUp!=0); if(ImGui::Checkbox("Pause on Level Up##sl",&sl)){cfg.bStopOnLevelUp=sl?1:0;Mark();}
    bool sd=(cfg.bStopOnDrop!=0);   if(ImGui::Checkbox("Pause on EXC/ANC Drop##sd",&sd)){cfg.bStopOnDrop=sd?1:0;Mark();}
    bool ar2=(cfg.bAutoRepair!=0);  if(ImGui::Checkbox("Auto Repair (< 10% dur)##rp",&ar2)){cfg.bAutoRepair=ar2?1:0;Mark();}

    ImGui::EndChild();
}

// ============================================================
//  TAB: PICKUP
// ============================================================
void CMuHelperUI::TabPickup()
{
    ImGui::BeginChild("##pku",{0,0},false);
    auto& cfg = m_editCfg;
    auto Mark = [&](){ m_bDirty = true; };

    SecHead("Item Filter");

    struct FR { const char* lbl; WORD fl; ImVec4 col; };
    static const FR rows[] = {
        {"Zen / Gold",         PICKUP_FLAG_ZEN,      Mu::IT_ZEN},
        {"Jewels",             PICKUP_FLAG_JEWELS,   Mu::BLUE},
        {"Excellent Items",    PICKUP_FLAG_EXCELLENT,Mu::IT_EXC},
        {"Ancient Items",      PICKUP_FLAG_ANCIENT,  Mu::IT_ANC},
        {"Set Items",          PICKUP_FLAG_SET,      Mu::GREEN},
        {"Socket Items",       PICKUP_FLAG_SOCKETS,  Mu::IT_SOC},
        {"Wings",              PICKUP_FLAG_WINGS,    Mu::PURPLE},
        {"Pets",               PICKUP_FLAG_PET,      Mu::ORANGE},
        {"Misc (scrolls)",     PICKUP_FLAG_MISC,     Mu::GRAY},
    };
    ImGui::Columns(2,nullptr,false);
    for (auto& r : rows)
    {
        bool v = !!(cfg.wPickupFlags & r.fl);
        char id[24]; snprintf(id,24,"##pf%04X",r.fl);
        if (ImGui::Checkbox(id,&v)) { v?cfg.wPickupFlags|=r.fl:cfg.wPickupFlags&=~r.fl; Mark(); }
        ImGui::SameLine(0,6); ImGui::TextColored(r.col,"%s",r.lbl);
        ImGui::NextColumn();
    }
    ImGui::Columns(1);
    ImGui::Spacing();

    ImGui::PushStyleColor(ImGuiCol_Button,{0.10f,0.36f,0.14f,1.f});
    if (ImGui::Button("Pick ALL",{110,0}))  { cfg.wPickupFlags=PICKUP_FLAG_ALL;  Mark(); }
    ImGui::PopStyleColor();
    ImGui::SameLine(0,8);
    ImGui::PushStyleColor(ImGuiCol_Button,{0.36f,0.10f,0.10f,1.f});
    if (ImGui::Button("Pick NONE",{110,0})) { cfg.wPickupFlags=PICKUP_FLAG_NONE; Mark(); }
    ImGui::PopStyleColor();

    SecHead("Pickup Radius");
    int pr = cfg.bPickupRadius;
    ImGui::SetNextItemWidth(220);
    if (ImGui::SliderInt("Pickup Radius##pr",&pr,1,MUHELPER_MAX_RADIUS)) { cfg.bPickupRadius=(BYTE)pr; Mark(); }

    ImGui::EndChild();
}

// ============================================================
//  TAB: POTIONS
// ============================================================
void CMuHelperUI::TabPotions()
{
    ImGui::BeginChild("##pot",{0,0},false);
    auto& cfg = m_editCfg;
    auto Mark = [&](){ m_bDirty = true; };

    SecHead("Auto Potion");
    struct PR { const char* id; const char* lbl; BYTE fl; BYTE* thr; ImVec4 lo,hi; };
    PR rows[] = {
        {"##hp","HP Potion", POTION_FLAG_HP,     &cfg.bHpThreshold,     Mu::BAR_HP_LO,Mu::BAR_HP_HI},
        {"##mp","MP Potion", POTION_FLAG_MP,     &cfg.bMpThreshold,     Mu::BAR_MP_LO,Mu::BAR_MP_HI},
        {"##sd","Shield",    POTION_FLAG_SHIELD, &cfg.bShieldThreshold, Mu::BAR_SD,   Mu::BAR_SD},
    };
    for (auto& r : rows)
    {
        bool v = !!(cfg.bPotionFlags & r.fl);
        if (ImGui::Checkbox(r.id,&v)) { v?cfg.bPotionFlags|=r.fl:cfg.bPotionFlags&=~r.fl; Mark(); }
        ImGui::SameLine(0,6); ImGui::TextUnformatted(r.lbl);
        if (v)
        {
            ImGui::Indent(22.f);
            char ov[20]; snprintf(ov,20,"Use at %d%%",*r.thr);
            GBar(*r.thr/100.f, r.lo, r.hi, -1, 14, ov);
            int t = *r.thr;
            if (ImGui::SliderInt((std::string("##ts")+r.id).c_str(),&t,5,95)) { *r.thr=(BYTE)t; Mark(); }
            ImGui::Unindent(22.f);
            ImGui::Spacing();
        }
    }
    ImGui::Separator(); ImGui::Spacing();
    bool an=(cfg.bPotionFlags&POTION_FLAG_ANTIDOTE)!=0;
    if(ImGui::Checkbox("Auto Antidote##an",&an)){an?cfg.bPotionFlags|=POTION_FLAG_ANTIDOTE:cfg.bPotionFlags&=~POTION_FLAG_ANTIDOTE;Mark();}
    bool ph=(cfg.bUsePartyHeal!=0);
    if(ImGui::Checkbox("Heal Party Members (Elf/SM)##ph",&ph)){cfg.bUsePartyHeal=ph?1:0;Mark();}

    ImGui::EndChild();
}

// ============================================================
//  TAB: BUFFS
// ============================================================
void CMuHelperUI::TabBuffs()
{
    ImGui::BeginChild("##buf",{0,0},false);
    auto& cfg = m_editCfg;
    auto Mark = [&](){ m_bDirty = true; };

    SecHead("Auto Buff");
    bool sb=(cfg.bSelfBuff!=0);
    if(ImGui::Checkbox("Self Buff before combat##sb",&sb)){cfg.bSelfBuff=sb?1:0;Mark();}
    if (sb)
    {
        ImGui::Indent();
        ImGui::SetNextItemWidth(140); if(SkillCombo("##bs1",&cfg.bBuffSkillSlot))  Mark();
        ImGui::SameLine(0,8); ImGui::TextColored(Mu::GRAY,"Buff slot 1");
        ImGui::SetNextItemWidth(140); if(SkillCombo("##bs2",&cfg.bBuffSkillSlot2)) Mark();
        ImGui::SameLine(0,8); ImGui::TextColored(Mu::GRAY,"Buff slot 2");
        ImGui::Unindent();
    }
    ImGui::TextColored(Mu::GRAY,"Rebuff interval: 30 s");

    SecHead("Movement");
    bool fl=(cfg.bFollowPartyLeader!=0);
    if(ImGui::Checkbox("Follow Party Leader##fl",&fl)){cfg.bFollowPartyLeader=fl?1:0;Mark();}

    SecHead("Display");
    bool ov=(cfg.bShowStatOverlay!=0);
    if(ImGui::Checkbox("Show Stats Overlay (top-left)##ov",&ov)){cfg.bShowStatOverlay=ov?1:0;Mark();}

    ImGui::EndChild();
}

// ============================================================
//  TAB: PARTY
// ============================================================
void CMuHelperUI::TabParty()
{
    ImGui::BeginChild("##pty",{0,0},false);
    SecHead("Party HP / MP Bars");

    auto& ph = CMuHelperClient::Instance().GetPartyHP();
    if (!ph.bCount)
    {
        ImGui::TextColored(Mu::GRAY,"(No party data yet)");
    }
    else
    {
        static const char* cls[] = {"DW","DK","Elf","MG","DL","Sum","RF","GL","Slayer","GC"};
        for (int i = 0; i < ph.bCount && i < 5; i++)
        {
            auto& m = ph.members[i];
            ImGui::Spacing();
            ImGui::TextColored(Mu::GOLD,"%-12s [%s]",
                m.szName, m.bClass<10 ? cls[m.bClass]:"?");
            char hpl[20]; snprintf(hpl,20,"HP %d%%",m.bHpPct);
            GBar(m.bHpPct/100.f, Mu::BAR_HP_LO, Mu::BAR_HP_HI, -1, 13, hpl);
            char mpl[20]; snprintf(mpl,20,"MP %d%%",m.bMpPct);
            GBar(m.bMpPct/100.f, Mu::BAR_MP_LO, Mu::BAR_MP_HI, -1, 11, mpl);
        }
    }
    ImGui::EndChild();
}

// ============================================================
//  TAB: STATS
// ============================================================
void CMuHelperUI::TabStats()
{
    ImGui::BeginChild("##sts",{0,0},false);
    auto& st = CMuHelperClient::Instance().GetStats();
    SecHead("Session Statistics");

    if (st.dwExpGained > 0)
    {
        char el[32]; snprintf(el,32,"%u EXP",st.dwExpGained);
        GBar(std::min(1.f,st.dwExpGained/5000000.f), Mu::BAR_EXP_LO, Mu::BAR_EXP_HI, -1, 16, el);
        ImGui::Spacing();
    }

    ImGui::Columns(2,nullptr,false);
    auto Row=[](const char* l, const char* v, ImVec4 c){
        ImGui::TextColored({0.55f,0.55f,0.60f,1.f},"%s",l);
        ImGui::NextColumn();
        ImGui::TextColored(c,"%s",v);
        ImGui::NextColumn();
    };
    char b[32];
    snprintf(b,32,"%u",st.wKillCount);           Row("Kills total",   b,Mu::GOLD);
    snprintf(b,32,"%u / hr",st.wKillsPerHour);   Row("Kill rate",     b,Mu::GOLD);
    snprintf(b,32,"%u",st.wItemPickup);           Row("Items picked",  b,Mu::IT_EXC);
    snprintf(b,32,"%u",st.dwZenPickup);           Row("Zen collected", b,Mu::IT_ZEN);
    snprintf(b,32,"%u / hr",st.dwExpPerHour);     Row("EXP / hr",      b,Mu::PURPLE);
    snprintf(b,32,"%u min",st.wSessionMinutes);   Row("Session time",  b,Mu::GRAY);
    ImGui::Columns(1);

    ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
    ImGui::TextColored(Mu::GRAY,"Resets when helper is re-enabled.");
    ImGui::EndChild();
}

// ============================================================
//  TAB: LOG
// ============================================================
void CMuHelperUI::TabLog()
{
    ImGui::BeginChild("##lg",{0,0},false);
    static char filt[64]={};
    ImGui::SetNextItemWidth(-60);
    ImGui::InputText("Filter##lf",filt,64);
    ImGui::SameLine();
    if(ImGui::Button("Clear")) filt[0]='\0';
    ImGui::Separator();

    static const ImVec4 lc[] = {
        Mu::WHITE,Mu::IT_ZEN,Mu::GREEN,Mu::RED,Mu::CYAN,Mu::GRAY
    };
    ImGui::BeginChild("##lgsc",{0,-4},false,ImGuiWindowFlags_HorizontalScrollbar);
    for (auto& e : CMuHelperClient::Instance().GetLog())
    {
        if (filt[0] && !strstr(e.sText.c_str(),filt)) continue;
        DWORD s = e.dwTimestamp/1000;
        ImGui::PushStyleColor(ImGuiCol_Text,Mu::GRAY);
        ImGui::Text("[%02d:%02d]",(s/60)%60,s%60);
        ImGui::PopStyleColor();
        ImGui::SameLine(0,6);
        ImGui::TextColored(lc[std::min((int)e.color,5)],"%s",e.sText.c_str());
    }
    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()-20.f)
        ImGui::SetScrollHereY(1.f);
    ImGui::EndChild();
    ImGui::EndChild();
}

// ============================================================
//  TAB: PROFILES
// ============================================================
void CMuHelperUI::TabProfiles()
{
    ImGui::BeginChild("##prof",{0,0},false);
    SecHead("Config Profiles");
    ImGui::TextColored(Mu::GRAY,"Save up to 5 named configurations."); ImGui::Spacing();

    auto& profiles = CMuHelperClient::Instance().GetProfiles();
    for (int i = 0; i < MUHELPER_MAX_PROFILES; i++)
    {
        auto& p = profiles[i];
        ImGui::PushID(i);
        ImGui::TextColored(p.bUsed ? Mu::GOLD : Mu::GRAY,"[%d] %-16s",
                           i+1, p.bUsed ? p.szName : "(empty)");
        ImGui::SameLine();
        if (p.bUsed)
        {
            if (ImGui::Button("Load")) CMuHelperClient::Instance().LoadProfile((BYTE)i);
            ImGui::SameLine(0,6);
        }
        static char names[5][16] = {"Grind1","Grind2","Grind3","Grind4","Grind5"};
        ImGui::SetNextItemWidth(110);
        ImGui::InputText("##pn",names[i],16);
        ImGui::SameLine(0,6);
        ImGui::PushStyleColor(ImGuiCol_Button,{0.14f,0.36f,0.16f,1.f});
        if (ImGui::Button("Save"))
            CMuHelperClient::Instance().SaveProfile((BYTE)i, names[i],
                CMuHelperClient::Instance().GetConfig());
        ImGui::PopStyleColor();
        ImGui::PopID();
        ImGui::Spacing();
    }
    ImGui::EndChild();
}
