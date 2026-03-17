#pragma once
// ============================================================
//  MuHelper — Hook Engine  (VERIFIED ADDRESSES)
//  Target: main.exe 1.02.11 (confirmed)  +  Main.dll (companion mod)
//  Scan date: 2026-03-17
//  Scanner: automated PE + byte-pattern analysis
// ============================================================
//
//  Rendering engine : OpenGL  (NOT Direct3D!)
//  Hook strategy    : SwapBuffers IAT + inline-detour on
//                     ProcessPacket + thiscall DataSend shim
//
// ============================================================
#include <windows.h>

// ─────────────────────────────────────────────────────────────
//  main.exe  (ImageBase = 0x00400000, confirmed PE header)
// ─────────────────────────────────────────────────────────────
namespace Addr_main_10211
{
    // ── Outgoing packet send ─────────────────────────────────
    //  DataSend  –  __thiscall  (ECX = net object)
    //  void DataSend(BYTE* pData, int nLen)
    //  Confirmed by SendBuffer Overflow xref + WSASend IAT call
    //  Prologue: 55 8B EC 83 EC 10 89 4D F0 …
    constexpr DWORD FN_DataSend          = 0x00403D10;

    //  Pointer to the CNetObject instance stored in .data
    //  Read at runtime: CNetObject* pNet = *(void**)PTR_NetObject
    constexpr DWORD PTR_NetObject        = 0x007D3B70;

    //  Encrypt context object (static VA in .bss)
    //  MOV ECX, 0x0587C278  before the encrypt call
    constexpr DWORD OBJ_EncryptCtx      = 0x0587C278;

    // ── Incoming packet dispatch ─────────────────────────────
    //  ProcessPacket  –  __cdecl (no EBP frame, SUB ESP 0x17C)
    //  int ProcessPacket(BYTE* pData, int nLen)
    //  Switch on pData[0]; 147-entry jump table at 0x0050DD9C
    //  CMP EAX, 0xF5 guard; opcode range 0x00-0xF4
    constexpr DWORD FN_ProcessPacket     = 0x0050A070;

    //  RecvProtocol  – outer recv/SEH loop, reassembles packets
    //  void RecvProtocol(CNetObject* pNet)
    //  Checks for C1/C2/C3 packet headers; decrypts; calls above
    constexpr DWORD FN_RecvProtocol      = 0x00509230;

    // ── Rendering: OpenGL (NOT Direct3D) ────────────────────
    //  IAT slot in .data section (DWORD holding import address)
    constexpr DWORD IAT_SwapBuffers      = 0x007B308C;

    //  Call sites: FF 15 8C 30 7B 00
    constexpr DWORD CALL_SwapBuffers_1   = 0x006CF9E6;  // primary render loop
    constexpr DWORD CALL_SwapBuffers_2   = 0x006D23FB;
    constexpr DWORD CALL_SwapBuffers_3   = 0x006FB00C;

    // ── WinSock IAT slots in .data ───────────────────────────
    constexpr DWORD IAT_WSASend          = 0x007B35CC;
    constexpr DWORD IAT_recv             = 0x007B35BC;

    // ── Network socket object in .bss ───────────────────────
    constexpr DWORD OBJ_NetSocket        = 0x05877AE8;

    // ── Opcode handler table ─────────────────────────────────
    //  147 entries x DWORD; index = opcode byte (0x00-0xF4)
    constexpr DWORD TBL_OpcodeHandlers   = 0x0050DD9C;

    // ── Stolen-byte counts for inline trampolines ────────────
    constexpr SIZE_T STOLEN_DataSend       = 7; // 55 8B EC 83 EC 10 89
    constexpr SIZE_T STOLEN_ProcessPacket  = 7; // 8B 44 24 04 81 EC 7C
    constexpr SIZE_T STOLEN_RecvProtocol   = 7; // 6A FF 64 A1 00 00 00
}

// ─────────────────────────────────────────────────────────────
//  Main.dll  (companion mod DLL, ImageBase = 0x10000000)
// ─────────────────────────────────────────────────────────────
namespace Addr_MainDll_10211
{
    constexpr DWORD FN_Init              = 0x10035D66; // E9 jump-table (50 trampolines)
    constexpr DWORD PTR_AttackHelperObj  = 0x100E526C; // CAttackHelper* global
    constexpr DWORD IAT_WS2_Send        = 0x10288BCC; // ws2_32 Ord#19 (send)
    constexpr DWORD IAT_WS2_Recv        = 0x10288BC8; // ws2_32 Ord#5  (recv)
}

// ─────────────────────────────────────────────────────────────
//  Function typedefs
// ─────────────────────────────────────────────────────────────
typedef void (__thiscall *fnDataSend)      (void* pNet, BYTE* pData, int nLen);
typedef int  (__cdecl   *fnProcessPacket)  (BYTE* pData, int nLen);
typedef void (__cdecl   *fnRecvProtocol)   (void* pNetObj);
typedef BOOL (__stdcall *fnSwapBuffers)    (HDC hdc);

// ─────────────────────────────────────────────────────────────
//  Runtime helper: resolve net object
// ─────────────────────────────────────────────────────────────
inline void* GetNetObject()
{
    return *reinterpret_cast<void**>(Addr_main_10211::PTR_NetObject);
}

// ─────────────────────────────────────────────────────────────
//  HookEngine
// ─────────────────────────────────────────────────────────────
class HookEngine
{
public:
    static void InstallAll();
    static void UninstallAll();

    static fnDataSend       OrigDataSend;
    static fnProcessPacket  OrigProcessPacket;
    static fnRecvProtocol   OrigRecvProtocol;
    static fnSwapBuffers    OrigSwapBuffers;

private:
    static BYTE s_trampolineDataSend   [16];
    static BYTE s_trampolineProcessPkt [16];
    static BYTE s_trampolineRecvProto  [16];
};
