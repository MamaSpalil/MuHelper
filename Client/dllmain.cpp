// ============================================================
//  MuHelper v2 — DLL Entry Point
//  Injects into main.exe (1.02.11 or 1.02.19 via build config)
// ============================================================
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "MuHelperClient.h"
#include "MuHelperUI.h"
#include "HookEngine.h"

HMODULE  g_hModule   = nullptr;
HHOOK    g_hKeyHook  = nullptr;
HWND     g_hGameWnd  = nullptr;

// ── Find the main game window ────────────────────────────────
static HWND FindGameWindow()
{
    // Try common MuOnline window titles
    static const char* titles[] = {
        "MU Online", "main", "MuOnline", nullptr
    };
    for (int i = 0; titles[i]; i++)
    {
        HWND hw = FindWindowA(nullptr, titles[i]);
        if (hw) return hw;
    }
    // Fallback: foreground window
    return GetForegroundWindow();
}

// ── Hotkey hook (F10 = toggle panel, F9 = start/stop) ────────
static LRESULT CALLBACK LowLevelKeyProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION && wParam == WM_KEYDOWN)
    {
        KBDLLHOOKSTRUCT* kb = (KBDLLHOOKSTRUCT*)lParam;
        if (kb->vkCode == VK_F10)
            CMuHelperUI::Instance().ToggleVisible();
        else if (kb->vkCode == VK_F9)
            CMuHelperClient::Instance().SendEnable(
                !CMuHelperClient::Instance().IsRunning());
    }
    return CallNextHookEx(g_hKeyHook, nCode, wParam, lParam);
}

// ── Worker thread ────────────────────────────────────────────
static DWORD WINAPI WorkerThread(LPVOID)
{
    Sleep(3500);  // Wait for game to finish OpenGL context setup

    // Find window
    g_hGameWnd = FindGameWindow();

    // Install hooks (ProcessPacket, DataSend, SwapBuffers)
    HookEngine::InstallAll();

    // Init client logic
    CMuHelperClient::Instance().Init();

    // Init ImGui overlay (needs HWND for Win32 backend)
    CMuHelperUI::Instance().Init(g_hGameWnd);

    // Keyboard hook
    g_hKeyHook = SetWindowsHookExA(WH_KEYBOARD_LL,
                                    LowLevelKeyProc,
                                    g_hModule, 0);
    // Message pump for WH_KEYBOARD_LL
    MSG msg;
    while (GetMessageA(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    return 0;
}

// ── DLL entry ─────────────────────────────────────────────────
BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        g_hModule = hModule;
        DisableThreadLibraryCalls(hModule);
        CreateThread(nullptr, 0, WorkerThread, nullptr, 0, nullptr);
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        if (g_hKeyHook) { UnhookWindowsHookEx(g_hKeyHook); g_hKeyHook = nullptr; }
        HookEngine::UninstallAll();
    }
    return TRUE;
}
