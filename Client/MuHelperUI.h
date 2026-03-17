#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// Dear ImGui forward declarations
struct ImFont;

// ============================================================
//  MuHelper v2 — ImGui OpenGL Overlay
//  Requires: imgui + imgui_impl_opengl2 + imgui_impl_win32
// ============================================================

class CMuHelperUI
{
public:
    static CMuHelperUI& Instance();

    void Init(HWND hWnd);
    void Render(HDC hdc);        // Called from SwapBuffers hook
    void ToggleVisible();
    bool IsVisible() const { return m_bVisible; }

private:
    CMuHelperUI() = default;

    // Panel sections
    void DrawMainPanel();
    void DrawTopBar(bool running);
    void DrawStatOverlay();
    void DrawSkillCooldownBar(float slotW);

    // Tabs
    void TabCombat();
    void TabPickup();
    void TabPotions();
    void TabBuffs();
    void TabParty();
    void TabStats();
    void TabLog();
    void TabProfiles();

    // State
    bool     m_bInited  = false;
    bool     m_bVisible = false;
    bool     m_bDirty   = false;
    HWND     m_hWnd     = nullptr;

    // Working copy of config (applied on Save)
    struct MuHelperConfig* m_pEditCfg = nullptr;
    struct MuHelperConfig  m_editCfg  = {};

    // ImGui fonts
    ImFont*  m_pFontNormal = nullptr;
    ImFont*  m_pFontSmall  = nullptr;
    ImFont*  m_pFontTitle  = nullptr;
};
