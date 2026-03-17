// ============================================================
//  MuHelper v2 — GameServer Integration Points
//  GameServer 1.00.18 + GgSrvDll.dll
// ============================================================
#include "MuHelperManager.h"

// ============================================================
//  PACKET DISPATCH  (add to GameServer RecvProtocol)
// ============================================================
//
//  In GameServer RecvProtocol() switch-case, add:
//
//    case MUHELPER_OPCODE_MAIN:      // 0x97
//        MuHelper_DispatchPacket(aIndex, (BYTE*)lpMsg, nMsgSize);
//        return;
//
// IMPORTANT: 0x97 is NOT registered with AddAuthProtocol(),
// so GgSrvDll.dll will NOT intercept or forward this opcode.
// Our handler runs BEFORE any GG auth check.
// ============================================================
void MuHelper_DispatchPacket(int nObjIndex, BYTE* lpMsg, int nMsgSize)
{
    if (nMsgSize < 4) return;

    switch (lpMsg[3])
    {
    case MUHELPER_SUB_ENABLE:
        if (nMsgSize >= (int)sizeof(PKT_MuHelper_Enable))
            g_MuHelper.OnPktEnable(nObjIndex, (PKT_MuHelper_Enable*)lpMsg);
        break;

    case MUHELPER_SUB_CFG_SEND:
        if (nMsgSize >= (int)sizeof(PKT_MuHelper_CfgSend))
            g_MuHelper.OnPktCfgSend(nObjIndex, (PKT_MuHelper_CfgSend*)lpMsg);
        break;

    case MUHELPER_SUB_CFG_REQUEST:
        g_MuHelper.OnPktCfgRequest(nObjIndex);
        break;

    case MUHELPER_SUB_PROFILE_SAVE:
        if (nMsgSize >= (int)sizeof(PKT_MuHelper_ProfileSave))
            g_MuHelper.OnPktProfileSave(nObjIndex, (PKT_MuHelper_ProfileSave*)lpMsg);
        break;

    case MUHELPER_SUB_PROFILE_LOAD:
        if (nMsgSize >= (int)sizeof(PKT_MuHelper_ProfileLoad))
            g_MuHelper.OnPktProfileLoad(nObjIndex, (PKT_MuHelper_ProfileLoad*)lpMsg);
        break;
    }
}

// ============================================================
//  INTEGRATION CHECKLIST
// ============================================================
//
//  1. GameServer main() / WinMain() STARTUP:
//       g_MuHelper.OnServerStart();
//
//  2. GameServer main loop (e.g. CGameServerManager::Run, every ~500ms):
//       g_MuHelper.OnServerTick();
//
//  3. GameServer shutdown:
//       g_MuHelper.OnServerShutdown();
//
//  4. Character fully loaded from DB (after all DB queries):
//       g_MuHelper.OnCharLoad(nObjIdx, dwCharDbId);
//
//  5. Client disconnect / CloseSocket:
//       g_MuHelper.OnCharDisconnect(nObjIdx);
//
//  6. Monster killed (CMob::Die or equivalent):
//       g_MuHelper.OnMonsterKilled(nKillerIdx, nThisMobIdx, dwExp);
//
//  7. Item dropped as excellent/ancient (in item drop handler):
//       if (item.bIsExcellent || item.bIsAncient)
//           g_MuHelper.OnItemDropped(nDropperIdx, nItemIdx);
//
//  8. Character levels up (in AddExp / level-up handler):
//       g_MuHelper.OnLevelUp(nObjIdx);
//
//  9. RecvProtocol opcode dispatch (see top of file):
//       case 0x97: MuHelper_DispatchPacket(...); return;
//
// ============================================================
//  GgSrvDll.dll — How MuHelper coexists with GameGuard
// ============================================================
//
//  GgSrvDll lifecycle (already done by GameServer, don't change):
//    InitGameguardAuth("ggauth.dll", pCallback);   // startup
//    AddAuthProtocol(opcode, handler);             // per GG opcode
//    GGAuthInitUser(&pAuth);                       // per client connect
//    GGAuthGetQuery(pAuth, &query);                // send challenge
//    GGAuthCheckAnswer(pAuth, &answer);            // verify response
//    GGAuthCloseUser(pAuth);                       // client disconnect
//    CleanupGameguardAuth();                       // shutdown
//
//  MuHelper ADDS:
//    OnCharLoad -> calls GGAuthInitUser for its session tracking
//    OnCharDisconnect -> calls GGAuthCloseUser for cleanup
//    The MuHelper session stores the CCSAuth2* pointer for
//    potential future auth-aware features.
//
//  MuHelper opcode 0x97 is TRANSPARENT to GgSrvDll — it is
//  handled entirely by our code before the GG dispatch table.
//
// ============================================================
//  SQL (MSSQL 2008 R2 — run once, see muhelper_schema_mssql.sql)
// ============================================================
//
//  CREATE TABLE [dbo].[MuHelperConfig] (
//      [CharIdx]   INT           NOT NULL,
//      [cfg]       VARBINARY(96) NOT NULL,
//      [UpdatedAt] DATETIME      NOT NULL DEFAULT GETDATE(),
//      CONSTRAINT [PK_MuHelperConfig] PRIMARY KEY CLUSTERED ([CharIdx])
//  );
//
//  CREATE TABLE [dbo].[MuHelperProfiles] (
//      [CharIdx]   INT           NOT NULL,
//      [SlotIdx]   TINYINT       NOT NULL,
//      [Name]      VARCHAR(16)   NOT NULL DEFAULT '',
//      [cfg]       VARBINARY(96) NOT NULL,
//      [UpdatedAt] DATETIME      NOT NULL DEFAULT GETDATE(),
//      CONSTRAINT [PK_MuHelperProfiles] PRIMARY KEY CLUSTERED ([CharIdx], [SlotIdx])
//  );
//
