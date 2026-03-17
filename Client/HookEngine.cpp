// ============================================================
//  MuHelper — Hook Engine Implementation
//  Uses version-selected offsets via Addr:: namespace alias
// ============================================================
#include "HookEngine.h"
#include "MuHelperClient.h"
#include "MuHelperUI.h"
#include "../Shared/MuHelperPackets.h"
#include <cstring>
#include <cstdio>

using namespace Addr;

// ── Statics ──────────────────────────────────────────────────
fnDataSend       HookEngine::OrigDataSend      = nullptr;
fnProcessPacket  HookEngine::OrigProcessPacket = nullptr;
fnRecvProtocol   HookEngine::OrigRecvProtocol  = nullptr;
fnSwapBuffers    HookEngine::OrigSwapBuffers    = nullptr;

BYTE HookEngine::s_trampolineDataSend   [16] = {};
BYTE HookEngine::s_trampolineProcessPkt [16] = {};
BYTE HookEngine::s_trampolineRecvProto  [16] = {};

// ── Minimal 5-byte JMP patcher ───────────────────────────────
namespace Patch
{
    static bool MakeExec(void* p, SIZE_T n)
    {
        DWORD old;
        return VirtualProtect(p, n, PAGE_EXECUTE_READWRITE, &old) != 0;
    }

    // Write JMP rel32 at src→dst, saving stolenBytes into trampolineOut
    // trampolineOut must be PAGE_EXECUTE_READWRITE already
    static bool WriteJmp(DWORD src, DWORD dst,
                         BYTE* trampolineOut, SIZE_T stolenBytes)
    {
        DWORD old;
        if (!VirtualProtect((void*)src, stolenBytes + 1, PAGE_EXECUTE_READWRITE, &old))
            return false;

        // Build trampoline: [stolen bytes] + [JMP back]
        memcpy(trampolineOut, (void*)src, stolenBytes);
        trampolineOut[stolenBytes]     = 0xE9;
        *(DWORD*)(trampolineOut + stolenBytes + 1) =
            (src + (DWORD)stolenBytes) -
            ((DWORD)(trampolineOut + stolenBytes + 1) + 4);

        // Patch src with JMP dst
        *(BYTE*)src        = 0xE9;
        *(DWORD*)(src + 1) = dst - (src + 5);
        for (SIZE_T i = 5; i < stolenBytes; i++)
            *(BYTE*)(src + i) = 0x90;

        VirtualProtect((void*)src, stolenBytes + 1, old, &old);
        return true;
    }

    static bool RestoreBytes(DWORD src, BYTE* savedBytes, SIZE_T n)
    {
        DWORD old;
        if (!VirtualProtect((void*)src, n, PAGE_EXECUTE_READWRITE, &old))
            return false;
        memcpy((void*)src, savedBytes, n);
        VirtualProtect((void*)src, n, old, &old);
        return true;
    }

    // IAT hook: replaces the function pointer stored at *iatSlot
    static bool HookIAT(DWORD iatSlot, DWORD newFn, DWORD* oldFnOut)
    {
        DWORD* pSlot = (DWORD*)iatSlot;
        DWORD old;
        if (!VirtualProtect(pSlot, 4, PAGE_READWRITE, &old))
            return false;
        *oldFnOut = *pSlot;
        *pSlot    = newFn;
        VirtualProtect(pSlot, 4, old, &old);
        return true;
    }
}

// ============================================================
//  Hook: ProcessPacket  –  intercept all incoming packets
// ============================================================
static int __cdecl Hooked_ProcessPacket(BYTE* pData, int nLen)
{
    if (nLen >= 4 && pData[2] == MUHELPER_OPCODE_MAIN)
    {
        // Consume MuHelper packets; don't pass to base game
        CMuHelperClient::Instance().OnPacketReceived(pData, nLen);
        return 1;
    }
    return HookEngine::OrigProcessPacket(pData, nLen);
}

// ============================================================
//  Hook: DataSend  –  intercept / inject outgoing packets
//  NOTE: This is __thiscall so we use a naked trampoline stub.
//        The stub restores ECX before jumping to trampoline.
// ============================================================
// We wrap via a small __cdecl proxy that receives the args in
// a predictable way, then re-invokes with correct ECX.
static void __cdecl Hooked_DataSend_Proxy(void* pNet, BYTE* pData, int nLen)
{
    // Optional: log or filter outgoing packets here
    // Pass through to original (trampoline preserves __thiscall)
    HookEngine::OrigDataSend(pNet, pData, nLen);
}

// ============================================================
//  Hook: SwapBuffers  –  render overlay after each frame
// ============================================================
static BOOL __stdcall Hooked_SwapBuffers(HDC hdc)
{
    CMuHelperUI::Instance().Render(hdc);
    return HookEngine::OrigSwapBuffers(hdc);
}

// ============================================================
//  Install / Uninstall
// ============================================================
void HookEngine::InstallAll()
{
    // Make trampolines executable
    DWORD dwOldProt;
    VirtualProtect(s_trampolineDataSend,   16, PAGE_EXECUTE_READWRITE, &dwOldProt);
    VirtualProtect(s_trampolineProcessPkt, 16, PAGE_EXECUTE_READWRITE, &dwOldProt);
    VirtualProtect(s_trampolineRecvProto,  16, PAGE_EXECUTE_READWRITE, &dwOldProt);

    // 1. Hook ProcessPacket (highest priority – intercepts all opcodes)
    if (Patch::WriteJmp(FN_ProcessPacket,
                        (DWORD)Hooked_ProcessPacket,
                        s_trampolineProcessPkt,
                        STOLEN_ProcessPacket))
    {
        OrigProcessPacket = (fnProcessPacket)(void*)s_trampolineProcessPkt;
    }

    // 2. Expose DataSend (we store trampoline to CALL it directly for injecting our packets)
    if (Patch::WriteJmp(FN_DataSend,
                        (DWORD)Hooked_DataSend_Proxy,  // no-op passthrough
                        s_trampolineDataSend,
                        STOLEN_DataSend))
    {
        OrigDataSend = (fnDataSend)(void*)s_trampolineDataSend;
    }

    // 3. Hook SwapBuffers via IAT
    DWORD oldSwap = 0;
    if (Patch::HookIAT(IAT_SwapBuffers, (DWORD)Hooked_SwapBuffers, &oldSwap))
        OrigSwapBuffers = (fnSwapBuffers)oldSwap;
}

void HookEngine::UninstallAll()
{
    // Restore ProcessPacket
    if (OrigProcessPacket)
        Patch::RestoreBytes(FN_ProcessPacket, s_trampolineProcessPkt, STOLEN_ProcessPacket);

    // Restore DataSend
    if (OrigDataSend)
        Patch::RestoreBytes(FN_DataSend, s_trampolineDataSend, STOLEN_DataSend);

    // Restore SwapBuffers IAT
    if (OrigSwapBuffers)
    {
        DWORD* pSlot = (DWORD*)IAT_SwapBuffers;
        DWORD old;
        VirtualProtect(pSlot, 4, PAGE_READWRITE, &old);
        *pSlot = (DWORD)OrigSwapBuffers;
        VirtualProtect(pSlot, 4, old, &old);
    }
}
