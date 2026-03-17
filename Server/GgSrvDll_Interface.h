#pragma once
// ============================================================
//  GgSrvDll.dll + GgAuth.dll Interface
//  Verified by binary analysis of GgSrvDll.dll (56K)
//  GameServer 1.00.19
// ============================================================

// ── GgSrvDll.dll exports (GameGuard server-side auth) ────────

// GGQUERY: 4 DWORDs sent TO the client as auth challenge
struct GGQUERY { DWORD dw[4]; };

// GGANSWER: 4 DWORDs received FROM the client as auth response
struct GGANSWER { DWORD dw[4]; };

// CCSAuth2 struct layout (verified from constructor at 0x100012B0):
//   +0x00  CCSAuth2*  pPrev          (linked list)
//   +0x04  DWORD      state_flag
//   +0x08  DWORD      ref_count
//   +0x0C  DWORD      query[0]       (challenge data, 4 DWORDs)
//   +0x10  DWORD      query[1]
//   +0x14  DWORD      query[2]
//   +0x18  DWORD      query[3]
//   +0x1C  DWORD      answer[0]      (response data, 4 DWORDs)
//   +0x20  DWORD      answer[1]
//   +0x24  DWORD      answer[2]
//   +0x28  DWORD      answer[3]
//   +0x2C  (end of struct, total 0x2C = 44 bytes)
struct CCSAuth2
{
    CCSAuth2* pPrev;
    DWORD     dwStateFlag;
    DWORD     dwRefCount;
    DWORD     dwQuery[4];   // Filled by GetAuthQuery
    DWORD     dwAnswer[4];  // Filled by CheckAuthAnswer
};

// ── GgSrvDll key globals (VA = base 0x10000000 + offset) ─────
//   g_szDllPath  0x1000BD30  char*  (ggauth.dll path)
//   g_pCallback  0x1000BD34  void*  (GameServer callback fn)
//   g_pAuthList  0x1000BD38  CCSAuth2*  (linked list head)
//   g_bInited    0x1000BD3C  BYTE   (0/1)

// ── Function typedefs ────────────────────────────────────────

// InitGameguardAuth(const char* szDllPath, void* pfnCallback) -> BOOL
//   szDllPath = path to ggauth.dll (e.g. "ggauth.dll")
//   pfnCallback = GameServer callback for auth completion
typedef BOOL  (__cdecl *fnInitGGAuth)(const char* szDllPath, void* pfnCallback);

// CleanupGameguardAuth() -> void
typedef void  (__cdecl *fnCleanupGGAuth)();

// AddAuthProtocol(BYTE opcode, void* pfnHandler) -> int
//   Registers a per-opcode handler; returns 0=ok 1=alloc_fail 2=init_fail
//   Node struct: {BYTE opcode, DWORD pfnQuery, DWORD pfnCheck, ...} 28 bytes
typedef int   (__cdecl *fnAddAuthProtocol)(BYTE opcode, void* pfnHandler);

// GGAuthInitUser(CCSAuth2** ppOut) -> CCSAuth2*
//   Allocates a new auth session for a connecting client
//   Returns NULL on failure
typedef CCSAuth2* (__cdecl *fnGGAuthInitUser)(CCSAuth2** ppOut);

// GGAuthCloseUser(CCSAuth2* pAuth) -> void
typedef void  (__cdecl *fnGGAuthCloseUser)(CCSAuth2* pAuth);

// GGAuthGetQuery(CCSAuth2* pAuth, GGQUERY* pOut) -> void
//   Fills pOut with challenge data to send to client
typedef void  (__cdecl *fnGGAuthGetQuery)(CCSAuth2* pAuth, GGQUERY* pOut);

// GGAuthCheckAnswer(CCSAuth2* pAuth, const GGANSWER* pIn) -> void
//   Stores client's answer into pAuth->dwAnswer[]
typedef void  (__cdecl *fnGGAuthCheckAnswer)(CCSAuth2* pAuth, const GGANSWER* pIn);

// CCSAuth2::Init() -> void  (thiscall)
typedef void  (__thiscall *fnCCSAuth2_Init)(CCSAuth2* pThis);

// CCSAuth2::GetAuthQuery() -> DWORD  (thiscall, fills query[])
typedef DWORD (__thiscall *fnCCSAuth2_GetQuery)(CCSAuth2* pThis);

// CCSAuth2::CheckAuthAnswer() -> DWORD  (thiscall, ret 0=ok 4=fail)
typedef DWORD (__thiscall *fnCCSAuth2_Check)(CCSAuth2* pThis);

// ── GgAuth.dll exports (lower-level protocol wrappers) ───────
//   PrtcGetVersion(DWORD* pOut) -> int
//   PrtcGetAuthQuery(DWORD* pState, GGQUERY* pOut) -> int
//   PrtcCheckAuthAnswer(DWORD* pState, const GGANSWER* pIn, ...) -> int
//   Version string: "PrtcLibVer 1.32"

// ── Helper: resolve all exports at runtime ───────────────────
struct GgSrvDll_API
{
    HMODULE           hMod;
    fnInitGGAuth      InitGameguardAuth;
    fnCleanupGGAuth   CleanupGameguardAuth;
    fnAddAuthProtocol AddAuthProtocol;
    fnGGAuthInitUser  GGAuthInitUser;
    fnGGAuthCloseUser GGAuthCloseUser;
    fnGGAuthGetQuery  GGAuthGetQuery;
    fnGGAuthCheckAnswer GGAuthCheckAnswer;

    GgSrvDll_API() : hMod(NULL), InitGameguardAuth(NULL), CleanupGameguardAuth(NULL),
        AddAuthProtocol(NULL), GGAuthInitUser(NULL), GGAuthCloseUser(NULL),
        GGAuthGetQuery(NULL), GGAuthCheckAnswer(NULL) {}

    bool Load(const char* szPath = "GgSrvDll.dll")
    {
        hMod = GetModuleHandleA(szPath);
        if (!hMod) hMod = LoadLibraryA(szPath);
        if (!hMod) return false;

        #define G(n) n = (fn##n)GetProcAddress(hMod, #n)
        G(InitGameguardAuth);
        G(CleanupGameguardAuth);
        G(AddAuthProtocol);
        G(GGAuthInitUser);
        G(GGAuthCloseUser);
        G(GGAuthGetQuery);
        G(GGAuthCheckAnswer);
        #undef G

        return (InitGameguardAuth && GGAuthInitUser);
    }
};

// Global instance
extern GgSrvDll_API g_GgSrv;
