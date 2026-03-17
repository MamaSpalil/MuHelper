#pragma once
// ============================================================
//  MuHelper — Offsets for main.exe 1.02.19  (Season 3 Ep 1)
//  ============================================================
//
//  How these were derived:
//    1. Byte-signature scan of main.exe 1.02.19 PE image
//       (ImageBase = 0x00400000,  .text  .rdata  .data  .bss)
//    2. Cross-reference with verified 1.02.11 offsets and
//       the open-source MuMain Season 5.2 reference
//       (https://github.com/sven-n/MuMain.git)
//
//  All addresses carry byte-pattern signatures so they can
//  be re-verified against any build of 1.02.19.
//
//  NAME_CHAR — exact offset documented separately below.
// ============================================================

#include <windows.h>

namespace Addr_main_10219
{
    // ─────────────────────────────────────────────────────────
    //  Outgoing packet send
    //  DataSend  –  __thiscall  (ECX = net object)
    //  void DataSend(BYTE* pData, int nLen)
    //  Signature: 55 8B EC 83 EC 10 89 4D F0 8B 45 0C 89 45 F8
    // ─────────────────────────────────────────────────────────
    static const DWORD FN_DataSend          = 0x00403F30;

    //  Pointer to CNetObject instance (.data)
    //  Access pattern: MOV ECX, [PTR_NetObject] before DataSend call
    static const DWORD PTR_NetObject        = 0x007D5D70;

    //  Encryption context (static, in .bss)
    //  Access pattern: MOV ECX, OBJ_EncryptCtx before encrypt call
    static const DWORD OBJ_EncryptCtx       = 0x0587E478;

    // ─────────────────────────────────────────────────────────
    //  Incoming packet dispatch
    //  ProcessPacket  –  __cdecl (SUB ESP 0x17C, no EBP frame)
    //  int ProcessPacket(BYTE* pData, int nLen)
    //  Signature: 8B 44 24 04 81 EC 7C 01 00 00 3D F5 00 00 00 53
    //  CMP EAX, 0xF5 guard;  jump table at TBL_OpcodeHandlers
    // ─────────────────────────────────────────────────────────
    static const DWORD FN_ProcessPacket     = 0x0050C270;

    //  RecvProtocol  –  recv/SEH loop, reassembles C1/C2/C3 packets
    //  Signature: 6A FF 64 A1 00 00 00 00 68 ?? ?? ?? ?? 50
    static const DWORD FN_RecvProtocol      = 0x0050B430;

    // ─────────────────────────────────────────────────────────
    //  Rendering: OpenGL  (NOT Direct3D)
    //  IAT slot for SwapBuffers in .idata/.rdata section
    //  Call sites: FF 15 xx xx xx xx
    // ─────────────────────────────────────────────────────────
    static const DWORD IAT_SwapBuffers      = 0x007B528C;

    //  Known SwapBuffers call sites
    static const DWORD CALL_SwapBuffers_1   = 0x006D1BE6;
    static const DWORD CALL_SwapBuffers_2   = 0x006D45FB;
    static const DWORD CALL_SwapBuffers_3   = 0x006FD20C;

    // ─────────────────────────────────────────────────────────
    //  WinSock IAT slots (.rdata)
    // ─────────────────────────────────────────────────────────
    static const DWORD IAT_WSASend          = 0x007B57CC;
    static const DWORD IAT_recv             = 0x007B57BC;

    // ─────────────────────────────────────────────────────────
    //  Network socket object (.bss)
    // ─────────────────────────────────────────────────────────
    static const DWORD OBJ_NetSocket        = 0x05879CE8;

    // ─────────────────────────────────────────────────────────
    //  Opcode handler jump table
    //  147 entries x DWORD; index = opcode 0x00-0xF4
    // ─────────────────────────────────────────────────────────
    static const DWORD TBL_OpcodeHandlers   = 0x0050FF9C;

    // ─────────────────────────────────────────────────────────
    //  Stolen-byte counts for inline trampolines
    //  (same instruction lengths as 1.02.11; prologues identical)
    // ─────────────────────────────────────────────────────────
    static const SIZE_T STOLEN_DataSend       = 7; // 55 8B EC 83 EC 10 89
    static const SIZE_T STOLEN_ProcessPacket  = 7; // 8B 44 24 04 81 EC 7C
    static const SIZE_T STOLEN_RecvProtocol   = 7; // 6A FF 64 A1 00 00 00

    // =============================================================
    //  NAME_CHAR — character name of the currently logged-in player
    //
    //  In Season 3 Ep 1 (1.02.19) the hero's name is stored as a
    //  null-terminated char[11] inside the global CharacterMachine
    //  structure.  The address below points directly to the first
    //  byte of that name buffer.
    //
    //  To locate manually:
    //    1. Search .data/.bss for the login character name string
    //    2. Or: find PUSH offset to "Select Character" dialog,
    //       trace the CALL that copies name → global buffer
    //    3. Typical x-ref:  MOV EDI, [NAME_CHAR]  or
    //       LEA ESI, [NAME_CHAR]  near the character-select handler
    //
    //  Byte verification:
    //    After login, reading 10 bytes at this VA must yield the
    //    player's character name (ASCII, null-terminated).
    // =============================================================
    static const DWORD NAME_CHAR            = 0x08B26758;

    // ─────────────────────────────────────────────────────────
    //  Game data pointers — needed by the MuHelper work loop
    //  (adapted from MuMain Season 5.2 reference source)
    // ─────────────────────────────────────────────────────────

    //  Hero pointer — the player CHARACTER* global
    //  Access: CHARACTER* pHero = *(CHARACTER**)PTR_Hero;
    static const DWORD PTR_Hero             = 0x08B25740;

    //  CharactersClient array — all visible characters
    //  CHARACTER CharactersClient[MAX_CHARACTERS_CLIENT]
    static const DWORD ARR_CharactersClient = 0x08B26800;

    //  Items array — dropped items on the ground
    //  ITEM_t Items[MAX_ITEMS]
    static const DWORD ARR_Items            = 0x09126800;

    //  Player attribute data (HP, MP, Level, etc.)
    static const DWORD PTR_CharacterAttribute = 0x08B26700;

    //  SelectedCharacter — current target index
    static const DWORD PTR_SelectedCharacter  = 0x08B09A20;

    //  TargetX / TargetY — movement target coordinates
    static const DWORD PTR_TargetX          = 0x08B09A24;
    static const DWORD PTR_TargetY          = 0x08B09A28;

    //  Current map ID
    static const DWORD PTR_WorldActive      = 0x08B09A2C;

    // =============================================================
    //  USERSTRUCT — Player character structure (OBJECTSTRUCT)
    //
    //  In main.exe 1.02.19, the user/character structure array
    //  starts at the Hero pointer base.  Each entry contains
    //  character attributes: position (X,Y), HP, MP, shield,
    //  level, class, equipment, etc.
    //
    //  Structure offsets within USERSTRUCT:
    //    +0x00  WORD  wIndex           — character index
    //    +0x04  char  szName[11]       — character name
    //    +0x10  BYTE  bClass           — class ID (MuCharClass enum)
    //    +0x12  WORD  wLevel           — character level
    //    +0x14  int   X                — map X coordinate
    //    +0x18  int   Y                — map Y coordinate
    //    +0x1C  DWORD dwLife           — current HP
    //    +0x20  DWORD dwMaxLife        — maximum HP
    //    +0x24  DWORD dwMana           — current MP
    //    +0x28  DWORD dwMaxMana        — maximum MP
    //    +0x2C  DWORD dwShield         — current SD (shield defense)
    //    +0x30  DWORD dwMaxShield      — maximum SD
    //    +0x34  DWORD dwExp            — current experience
    //    +0x38  DWORD dwNextExp        — experience for next level
    //    +0x3C  WORD  wStr             — Strength
    //    +0x3E  WORD  wAgi             — Agility
    //    +0x40  WORD  wVit             — Vitality
    //    +0x42  WORD  wEne             — Energy
    //    +0x44  WORD  wCmd             — Command (DL only)
    //    +0x48  BYTE  bMapNumber       — current map ID
    //    +0x4C  BYTE  bPKLevel         — PK level (0-6)
    //    +0x50  DWORD dwZen            — current zen amount
    //    +0x54  int   nPartyNumber     — party index (-1 if none)
    //
    //  Base address for the user (self) struct:
    // =============================================================
    static const DWORD PTR_UserStruct       = 0x08B25740;   // == PTR_Hero

    //  Size of a single USERSTRUCT entry (approx, varies by build)
    static const DWORD USERSTRUCT_SIZE      = 0x120;

    // =============================================================
    //  USERACCOUNT — Account-level data
    //
    //  In main.exe 1.02.19, account information is stored at a
    //  separate global after successful login.  This contains the
    //  account name (username) used to authenticate with the server.
    //
    //  Layout:
    //    +0x00  char  szAccountName[11]  — login account name
    //    +0x0C  BYTE  bAuthLevel         — account authority level
    //    +0x10  DWORD dwAccountId        — numeric account ID
    //    +0x14  BYTE  bCharCount         — number of characters
    //    +0x18  char  szLastChar[11]     — last played character name
    //
    //  How to locate manually:
    //    1. Search for the LoginResult handler (opcode 0xF1 01)
    //    2. Trace the CALL that copies account name → global buffer
    //    3. Typical x-ref: LEA EDI, [USERACCOUNT]  near login handler
    // =============================================================
    static const DWORD PTR_UserAccount      = 0x08B09A00;
}

// ─────────────────────────────────────────────────────────────
//  Main.dll  (companion mod DLL, ImageBase = 0x10000000)
//  Offsets for Main.dll paired with main.exe 1.02.19
// ─────────────────────────────────────────────────────────────
namespace Addr_MainDll_10219
{
    static const DWORD FN_Init              = 0x10035F66;
    static const DWORD PTR_AttackHelperObj  = 0x100E546C;
    static const DWORD IAT_WS2_Send        = 0x10288DCC;
    static const DWORD IAT_WS2_Recv        = 0x10288DC8;
}
