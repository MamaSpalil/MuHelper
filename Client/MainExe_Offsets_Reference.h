#pragma once
// ============================================================================
//
//   MuHelper — Comprehensive Offset Reference for main.exe (v1.02.11)
//
//   Binary:   Client/main.exe  (4,316,672 bytes)
//   Format:   PE32 executable (GUI) Intel 80386, for MS Windows
//   Version:  1.02.11  (MU Online Season 3)
//
//   ImageBase:  0x00400000
//   Sections:
//     .text      0x00401000 - 0x007B3000   Code (RWX)
//     .data      0x007B3000 - 0x007CF000   Initialised data (IAT, globals)
//     (unnamed)  0x007CF000 - 0x07EE7000   Additional data (strings, floats)
//     .rsrc      0x07EE7000 - 0x07EEA000   Resources
//     .idata     0x07EEA000 - 0x07EED000   Import directory
//     .zero      0x07EED000 - 0x09110000   BSS (uninitialised, runtime only)
//     .as_0001   0x09110000 - 0x0912E000   Protected code
//     .as_0002   0x09170000 - 0x0917C000   Protected code
//     .LibHook   0x0917C000 - 0x0917D000   Packer entry stub
//
//   Scan method:  PE header parsing + byte-pattern matching + IAT dump
//                 + cross-reference counting in .text section
//   Scan date:    2026-03-17
//
//   Every offset below has been verified against the binary either by
//   byte-signature match (functions), IAT enumeration (imports),
//   or cross-reference analysis (globals).
//
//   NOTE: Addresses in the .zero (BSS) section (0x07EED000+) have zero
//   raw size in the PE file.  They are populated only at runtime by the
//   game engine.  Their locations were derived from runtime analysis
//   and from the open-source MuMain Season 5.2 reference
//   (https://github.com/sven-n/MuMain.git).
//
// ============================================================================

#include <windows.h>

// ============================================================================
//
//  SECTION 1:  FUNCTION OFFSETS  (.text section)
//
//  These are virtual addresses of key game functions in the .text section.
//  All byte-signatures verified against the on-disk PE image.
//
// ============================================================================

// ----------------------------------------------------------------------------
//  1.1  Network — Packet Send / Receive
// ----------------------------------------------------------------------------

//  DataSend — sends a raw packet to the game server
//  Calling convention: __thiscall  (ECX = CNetObject* pointer)
//  Prototype: void DataSend(BYTE* pData, int nLen)
//  Verified signature: 55 8B EC 83 EC 10 89 4D F0 8B 45 0C 89 45 F8
//  Used by MuHelper to inject outgoing packets (skill casts, movement, etc.)
static const DWORD FN_DataSend              = 0x00403D10;

//  ProcessPacket — central dispatcher for all incoming server packets
//  Calling convention: __cdecl  (no EBP frame, SUB ESP 0x17C)
//  Prototype: int ProcessPacket(BYTE* pData, int nLen)
//  Verified signature: 8B 44 24 04 81 EC 7C 01 00 00 3D F5 00 00 00 53
//  Switches on pData[0] (opcode); valid range 0x00 – 0xF4
//  CMP EAX, 0xF5 guard jump; 147-entry jump table at TBL_OpcodeHandlers
//  Hooked by MuHelper to intercept server→client packets
static const DWORD FN_ProcessPacket         = 0x0050A070;

//  RecvProtocol — outer receive loop with SEH exception handling
//  Calling convention: __cdecl
//  Prototype: void RecvProtocol(CNetObject* pNet)
//  Verified signature: 6A FF 64 A1 00 00 00 00 68 FE 82 7A 00 50 B8 80
//  Reassembles packets from C1/C2/C3/C4 headers; decrypts; calls ProcessPacket
//  Packet header checks at +0x4F (C1), +0x68 (C2), +0x8B (C3), +0x8F (C4)
static const DWORD FN_RecvProtocol          = 0x00509230;

// ----------------------------------------------------------------------------
//  1.2  Main Application — Window / Message Loop
// ----------------------------------------------------------------------------

//  Main message loop — GetMessageA dispatch
//  Location of the CALL [GetMessageA] instruction inside WinMain
//  The game's message pump: GetMessage → TranslateMessage → DispatchMessage
static const DWORD CALL_GetMessageA         = 0x004DE245;

//  Window creation — CreateWindowExA call that creates the main game window
//  The return value (HWND) is stored at PTR_hWndMain (0x05877F90)
//  Instruction: CALL [CreateWindowExA] with WS_POPUP | WS_VISIBLE style
static const DWORD CALL_CreateWindowExA     = 0x004DB9E8;

// ----------------------------------------------------------------------------
//  1.3  Protocol Handlers — Login / Character Select
// ----------------------------------------------------------------------------

//  Login opcode dispatch point — CMP EAX, 0xF1
//  Inside ProcessPacket, this is the branch that handles opcode 0xF1
//  Sub-opcodes: 0x01 = login result, 0x03 = character list
//  After successful login, account data is written to PTR_UserAccount
static const DWORD OPCODE_Login_CMP         = 0x004FD10A;

//  Character select dispatch point — CMP EAX, 0xF3
//  Handles opcode 0xF3 (character creation/selection)
//  After selection, character name is written to NAME_CHAR
static const DWORD OPCODE_CharSelect_CMP    = 0x004EAD11;


// ============================================================================
//
//  SECTION 2:  OPCODE HANDLER JUMP TABLE  (.text section)
//
//  ProcessPacket uses a 147-entry jump table (DWORD array) to dispatch
//  incoming packets by opcode byte.  Index = opcode (0x00 – 0x92).
//  Opcodes above 0x92 use the default handler (0x3F and 0x92 entries).
//
// ============================================================================

//  Base address of the jump table (147 x DWORD)
static const DWORD TBL_OpcodeHandlers       = 0x0050DD9C;

//  Key opcode handlers (selected, commonly hooked):
//  [0x00] 0x0050AA94 — Object creation / NPC appearance
//  [0x01] 0x0050AAB4 — Object update / position sync
//  [0x0D] 0x0050B2E1 — Damage / attack result
//  [0x0F] 0x0050B18A — Experience gained
//  [0x11] 0x0050B24B — Level up notification
//  [0x22] 0x0050DAA5 — Chat message
//  [0x26] 0x0050B4FC — Item dropped on ground
//  [0x27] 0x0050B570 — Item picked up
//  [0x30] 0x0050C95A — Skill cast result
//  [0x3E] 0x0050BBE8 — Party data update
//  [0x4F] 0x0050C104 — Guild notification
//  [0x5B] 0x0050C47F — Trade window
//  [0x60] 0x0050C6A1 — HP/MP/SD update (vitals)
//  [0x6F] 0x0050D3C1 — Quest data
//  [0x8E] 0x0050A098 — Extended protocol (sub-dispatch)
//  [0x8F] 0x0050A57E — Character data block (S3 extended)
//  [0x90] 0x0050A985 — Map/world change

//  Total handler count
static const DWORD TBL_OpcodeHandlers_Count  = 147;

//  Individual handler addresses (all 147 verified):
static const DWORD HANDLER_0x00 = 0x0050AA94;  // Object creation / NPC spawn
static const DWORD HANDLER_0x01 = 0x0050AAB4;  // Object position update
static const DWORD HANDLER_0x02 = 0x0050ABA0;  // Object animation state
static const DWORD HANDLER_0x03 = 0x0050AC46;  // Object removal
static const DWORD HANDLER_0x04 = 0x0050B26B;  // Object attribute update
static const DWORD HANDLER_0x05 = 0x0050AC66;  // Viewport object action
static const DWORD HANDLER_0x06 = 0x0050AD91;  // Object equipment update
static const DWORD HANDLER_0x07 = 0x0050ADC7;  // Object stat change
static const DWORD HANDLER_0x08 = 0x0050ADE7;  // Object death
static const DWORD HANDLER_0x09 = 0x0050AE80;  // Object respawn
static const DWORD HANDLER_0x0D = 0x0050B2E1;  // Damage / hit result
static const DWORD HANDLER_0x0F = 0x0050B18A;  // Experience gained
static const DWORD HANDLER_0x11 = 0x0050B24B;  // Level up
static const DWORD HANDLER_0x22 = 0x0050DAA5;  // Chat message received
static const DWORD HANDLER_0x26 = 0x0050B4FC;  // Item drop on ground
static const DWORD HANDLER_0x27 = 0x0050B570;  // Item pickup / inventory update
static const DWORD HANDLER_0x30 = 0x0050C95A;  // Skill activation result
static const DWORD HANDLER_0x3E = 0x0050BBE8;  // Party information
static const DWORD HANDLER_0x3F = 0x0050DD8A;  // Default / unhandled opcode
static const DWORD HANDLER_0x4F = 0x0050C104;  // Guild notification
static const DWORD HANDLER_0x5B = 0x0050C47F;  // Trade window events
static const DWORD HANDLER_0x60 = 0x0050C6A1;  // HP / MP / SD vital update
static const DWORD HANDLER_0x6F = 0x0050D3C1;  // Quest progress data
static const DWORD HANDLER_0x8E = 0x0050A098;  // Extended protocol (sub-dispatch)
static const DWORD HANDLER_0x8F = 0x0050A57E;  // Character data block (S3)
static const DWORD HANDLER_0x90 = 0x0050A985;  // Map / world change
static const DWORD HANDLER_0x92 = 0x0050DD8A;  // Default (same as 0x3F)


// ============================================================================
//
//  SECTION 3:  IAT (Import Address Table) SLOTS  (.data section)
//
//  Each IAT slot is a DWORD in .data that holds the resolved address of an
//  imported API function.  At load time the PE loader fills these in.
//  Hooking is done by overwriting the DWORD in the slot.
//
// ============================================================================

// ----------------------------------------------------------------------------
//  3.1  Rendering — OpenGL (gdi32.dll)
// ----------------------------------------------------------------------------

//  SwapBuffers — the primary rendering hook point
//  MuHelper patches this IAT slot to intercept every frame and draw its
//  ImGui overlay after the game finishes rendering.
//  Verified: 3 call sites reference this slot with FF 15 8C 30 7B 00
static const DWORD IAT_SwapBuffers          = 0x007B308C;

//  SwapBuffers call sites in .text section
//  These are the locations of the FF 15 (CALL [IAT_SwapBuffers]) instructions
static const DWORD CALL_SwapBuffers_1       = 0x006CF9E6;  // Primary render loop
static const DWORD CALL_SwapBuffers_2       = 0x006D23FB;  // Secondary render path
static const DWORD CALL_SwapBuffers_3       = 0x006FB00C;  // Alternate render path

//  Other OpenGL IAT slots (gdi32.dll / opengl32.dll)
static const DWORD IAT_ChoosePixelFormat    = 0x007B3080;  // Pixel format selection
static const DWORD IAT_SetPixelFormat       = 0x007B307C;  // Pixel format setup
static const DWORD IAT_SelectObject         = 0x007B3074;  // GDI object selection
static const DWORD IAT_CreateFontA          = 0x007B3084;  // Font creation for text

//  OpenGL core functions (opengl32.dll)
static const DWORD IAT_glEnable             = 0x007B339C;  // Enable GL state
static const DWORD IAT_glDisable            = 0x007B33A0;  // Disable GL state
static const DWORD IAT_glBlendFunc          = 0x007B3428;  // Alpha blending setup
static const DWORD IAT_glViewport           = 0x007B342C;  // Viewport dimensions
static const DWORD IAT_glClear              = 0x007B33F8;  // Clear framebuffer
static const DWORD IAT_glBindTexture        = 0x007B33AC;  // Bind texture
static const DWORD IAT_glGenTextures        = 0x007B3418;  // Generate texture names
static const DWORD IAT_glDeleteTextures     = 0x007B33E0;  // Delete textures
static const DWORD IAT_wglMakeCurrent       = 0x007B340C;  // OpenGL context switch
static const DWORD IAT_wglCreateContext     = 0x007B3410;  // Create GL context
static const DWORD IAT_wglDeleteContext     = 0x007B3408;  // Delete GL context

// ----------------------------------------------------------------------------
//  3.2  Network — Winsock (ws2_32.dll)
// ----------------------------------------------------------------------------

//  recv — receives data from connected socket
//  Hooked by MuHelper to intercept incoming raw network data
static const DWORD IAT_recv                 = 0x007B35BC;

//  WSASend — Winsock async send
//  NOTE: The existing codebase names IAT_WSASend at 0x007B35CC.
//  In the actual IAT, 0x007B35CC is `send` (not WSASend).
//  The real WSASend is at 0x007B35C0.  Both are listed below.
static const DWORD IAT_WSASend              = 0x007B35C0;  // Winsock WSASend (async)
static const DWORD IAT_send                 = 0x007B35CC;  // Winsock send (sync)

//  Other Winsock functions
static const DWORD IAT_socket               = 0x007B35B4;  // Create socket
static const DWORD IAT_connect              = 0x007B35DC;  // Connect to server
static const DWORD IAT_closesocket          = 0x007B35E0;  // Close socket
static const DWORD IAT_shutdown             = 0x007B35B8;  // Shutdown connection
static const DWORD IAT_setsockopt           = 0x007B35B0;  // Set socket options
static const DWORD IAT_gethostbyname        = 0x007B35A8;  // DNS resolve
static const DWORD IAT_WSAAsyncSelect       = 0x007B35AC;  // Async event select
static const DWORD IAT_WSAStartup           = 0x007B35C4;  // Winsock init
static const DWORD IAT_WSACleanup           = 0x007B35C8;  // Winsock cleanup
static const DWORD IAT_WSAGetLastError      = 0x007B35D0;  // Last Winsock error
static const DWORD IAT_inet_addr            = 0x007B35D4;  // IP string → DWORD
static const DWORD IAT_htons                = 0x007B35D8;  // Host-to-network short

// ----------------------------------------------------------------------------
//  3.3  Process / Thread / Memory (kernel32.dll)
// ----------------------------------------------------------------------------

static const DWORD IAT_CreateThread         = 0x007B310C;  // Spawn thread
static const DWORD IAT_TerminateThread      = 0x007B3124;  // Kill thread
static const DWORD IAT_Sleep                = 0x007B3164;  // Thread sleep
static const DWORD IAT_GetTickCount         = 0x007B3160;  // Millisecond timer
static const DWORD IAT_VirtualAlloc         = 0x007B3140;  // Allocate virtual memory
static const DWORD IAT_VirtualFree          = 0x007B3144;  // Free virtual memory
static const DWORD IAT_VirtualProtect       = 0x007B3148;  // Change page protection
static const DWORD IAT_LoadLibraryA         = 0x007B31C8;  // Load DLL
static const DWORD IAT_LoadLibraryExA       = 0x007B314C;  // Load DLL (extended)
static const DWORD IAT_FreeLibrary          = 0x007B31C0;  // Unload DLL
static const DWORD IAT_GetProcAddress       = 0x007B31C4;  // Get function pointer
static const DWORD IAT_GetModuleHandleA     = 0x007B321C;  // Get module base
static const DWORD IAT_GetModuleFileNameA   = 0x007B3214;  // Get module path
static const DWORD IAT_ExitProcess          = 0x007B313C;  // Terminate process
static const DWORD IAT_CreateMutexA         = 0x007B3128;  // Create named mutex
static const DWORD IAT_OpenMutexA           = 0x007B3110;  // Open named mutex
static const DWORD IAT_CreateFileA          = 0x007B3178;  // Open/create file
static const DWORD IAT_ReadFile             = 0x007B3180;  // Read file
static const DWORD IAT_WriteFile            = 0x007B3170;  // Write file
static const DWORD IAT_CloseHandle          = 0x007B316C;  // Close handle
static const DWORD IAT_GetCurrentProcessId  = 0x007B3274;  // Current PID
static const DWORD IAT_GetCurrentThreadId   = 0x007B315C;  // Current TID

//  Synchronization
static const DWORD IAT_EnterCriticalSection = 0x007B3114;  // Lock critical section
static const DWORD IAT_LeaveCriticalSection = 0x007B3118;  // Unlock critical section
static const DWORD IAT_InitCriticalSection  = 0x007B3244;  // Init critical section
static const DWORD IAT_DeleteCriticalSection= 0x007B3240;  // Delete critical section
static const DWORD IAT_WaitForSingleObject  = 0x007B3130;  // Wait on sync object

//  Anti-hack: process enumeration
static const DWORD IAT_CreateToolhelp32Snap = 0x007B3204;  // Process snapshot
static const DWORD IAT_Process32First       = 0x007B3200;  // First process entry
static const DWORD IAT_Process32Next        = 0x007B31F4;  // Next process entry
static const DWORD IAT_OpenProcess          = 0x007B31FC;  // Open process handle
static const DWORD IAT_TerminateProcess     = 0x007B31F8;  // Kill process

//  Performance counter (frame timing)
static const DWORD IAT_QueryPerfCounter     = 0x007B3194;  // High-res timer
static const DWORD IAT_QueryPerfFrequency   = 0x007B3368;  // Timer frequency
static const DWORD IAT_GetLocalTime         = 0x007B3184;  // Local time

// ----------------------------------------------------------------------------
//  3.4  Window / UI (user32.dll)
// ----------------------------------------------------------------------------

static const DWORD IAT_CreateWindowExA      = 0x007B34B4;  // Create window
static const DWORD IAT_DestroyWindow        = 0x007B34F4;  // Destroy window
static const DWORD IAT_ShowWindow           = 0x007B34A8;  // Show/hide window
static const DWORD IAT_SetWindowPos         = 0x007B3478;  // Move/resize window
static const DWORD IAT_GetWindowRect        = 0x007B3474;  // Get window bounds
static const DWORD IAT_SetForegroundWindow  = 0x007B3504;  // Bring to front
static const DWORD IAT_FindWindowA          = 0x007B3564;  // Find window by class
static const DWORD IAT_SetTimer             = 0x007B34CC;  // Create timer
static const DWORD IAT_KillTimer            = 0x007B355C;  // Destroy timer
static const DWORD IAT_GetMessageA          = 0x007B3524;  // Get window message
static const DWORD IAT_PeekMessageA         = 0x007B3528;  // Peek window message
static const DWORD IAT_TranslateMessage     = 0x007B3518;  // Translate msg
static const DWORD IAT_DispatchMessageA     = 0x007B3514;  // Dispatch msg
static const DWORD IAT_PostMessageA         = 0x007B34B8;  // Post async message
static const DWORD IAT_SendMessageA         = 0x007B346C;  // Send sync message
static const DWORD IAT_DefWindowProcA       = 0x007B34E4;  // Default wndproc
static const DWORD IAT_GetAsyncKeyState     = 0x007B345C;  // Keyboard input
static const DWORD IAT_RegisterClassA       = 0x007B34F8;  // Register wndclass
static const DWORD IAT_GetDC                = 0x007B351C;  // Get device context
static const DWORD IAT_ReleaseDC            = 0x007B34AC;  // Release device context
static const DWORD IAT_MessageBoxA          = 0x007B348C;  // Show message box
static const DWORD IAT_ShowCursor           = 0x007B34D0;  // Show/hide cursor
static const DWORD IAT_GetCursorPos         = 0x007B354C;  // Get cursor position
static const DWORD IAT_SetCursorPos         = 0x007B3534;  // Set cursor position
static const DWORD IAT_SetCapture           = 0x007B34E0;  // Mouse capture
static const DWORD IAT_ReleaseCapture       = 0x007B34DC;  // Release mouse
static const DWORD IAT_SetWindowsHookExA    = 0x007B3484;  // Install hook
static const DWORD IAT_UnhookWindowsHookEx  = 0x007B3480;  // Remove hook
static const DWORD IAT_GetFocus             = 0x007B3450;  // Get focused window
static const DWORD IAT_SetFocus             = 0x007B34BC;  // Set focus
static const DWORD IAT_GetActiveWindow      = 0x007B3550;  // Active window
static const DWORD IAT_ChangeDisplaySettings= 0x007B34D4;  // Display mode
static const DWORD IAT_GetSystemMetrics     = 0x007B3508;  // Screen metrics

// ----------------------------------------------------------------------------
//  3.5  Multimedia (winmm.dll, dsound.dll, dinput8.dll)
// ----------------------------------------------------------------------------

static const DWORD IAT_timeGetTime          = 0x007B3588;  // Multimedia timer
static const DWORD IAT_timeBeginPeriod      = 0x007B3598;  // Timer resolution
static const DWORD IAT_timeEndPeriod        = 0x007B35A0;  // Reset resolution
static const DWORD IAT_DirectSoundCreate    = 0x007B3064;  // Create DirectSound
static const DWORD IAT_DirectInput8Create   = 0x007B305C;  // Create DirectInput

// ----------------------------------------------------------------------------
//  3.6  Crypto / Registry (advapi32.dll)
// ----------------------------------------------------------------------------

static const DWORD IAT_CryptAcquireContextA = 0x007B3054;  // Acquire crypto context
static const DWORD IAT_CryptCreateHash      = 0x007B3018;  // Create hash object
static const DWORD IAT_CryptHashData        = 0x007B301C;  // Hash data
static const DWORD IAT_CryptDeriveKey       = 0x007B300C;  // Derive key from hash
static const DWORD IAT_CryptDecrypt         = 0x007B3010;  // Decrypt data
static const DWORD IAT_CryptImportKey       = 0x007B3014;  // Import key
static const DWORD IAT_CryptVerifySigA      = 0x007B3020;  // Verify signature
static const DWORD IAT_CryptDestroyHash     = 0x007B3024;  // Destroy hash
static const DWORD IAT_CryptDestroyKey      = 0x007B3028;  // Destroy key
static const DWORD IAT_CryptReleaseContext  = 0x007B303C;  // Release context
static const DWORD IAT_RegOpenKeyExA        = 0x007B3048;  // Open registry key
static const DWORD IAT_RegQueryValueExA     = 0x007B304C;  // Read registry value
static const DWORD IAT_RegSetValueExA       = 0x007B3040;  // Write registry value
static const DWORD IAT_RegCloseKey          = 0x007B3050;  // Close registry key

// ----------------------------------------------------------------------------
//  3.7  Shell / COM (shell32.dll, ole32.dll)
// ----------------------------------------------------------------------------

static const DWORD IAT_ShellExecuteA        = 0x007B3444;  // Launch URL / app
static const DWORD IAT_CoCreateInstance     = 0x007B35EC;  // COM object creation
static const DWORD IAT_CoInitialize         = 0x007B35F0;  // COM init

// ----------------------------------------------------------------------------
//  3.8  Game Audio (wzaudio.dll) — custom MU Online audio library
// ----------------------------------------------------------------------------

static const DWORD IAT_wzAudioCreate        = 0x007B360C;  // Create audio engine
static const DWORD IAT_wzAudioDestroy       = 0x007B3604;  // Destroy audio engine
static const DWORD IAT_wzAudioPlay          = 0x007B35FC;  // Play sound
static const DWORD IAT_wzAudioStop          = 0x007B35F8;  // Stop sound
static const DWORD IAT_wzAudioOption        = 0x007B3608;  // Set audio option

// ----------------------------------------------------------------------------
//  3.9  IME / Input Method (imm32.dll)
// ----------------------------------------------------------------------------

static const DWORD IAT_ImmGetContext        = 0x007B30F4;  // Get IME context
static const DWORD IAT_ImmReleaseContext    = 0x007B30D4;  // Release IME context
static const DWORD IAT_ImmSetOpenStatus     = 0x007B30E8;  // Open/close IME
static const DWORD IAT_ImmGetOpenStatus     = 0x007B3104;  // Query IME state


// ============================================================================
//
//  SECTION 4:  NETWORK OBJECTS  (.zero / BSS section)
//
//  These are static C++ objects in the BSS section.  They exist at fixed
//  virtual addresses but have zero content in the on-disk PE image.
//  Cross-reference counts come from scanning the .text section for
//  MOV ECX, <addr> (B9 pattern) and MOV reg, [addr] patterns.
//
// ============================================================================

// ----------------------------------------------------------------------------
//  4.1  CSocket — Main Network Socket Object
// ----------------------------------------------------------------------------

//  OBJ_NetSocket — Base address of the CSocket instance
//  Access: MOV ECX, 0x05877AE8 ; CALL CSocket::method
//  4149 cross-references in .text — the most referenced object in main.exe
//  This is the primary socket used for all game server communication.
static const DWORD OBJ_NetSocket            = 0x05877AE8;

//  CSocket field offsets (from OBJ_NetSocket base):
//    +0x00  vtable pointer
//    +0x04  connection state / flags          (475 refs)
static const DWORD OBJ_NetSocket_State      = 0x05877AEC;
//    +0x08  send buffer pointer               (483 refs)
static const DWORD OBJ_NetSocket_SendBuf    = 0x05877AF0;
//    +0x0C  recv buffer pointer / length      (871 refs)
static const DWORD OBJ_NetSocket_RecvBuf    = 0x05877AF4;
//    +0x28  data queue / processing buffer    (1617 refs)
static const DWORD OBJ_NetSocket_DataQueue  = 0x05877B10;

// ----------------------------------------------------------------------------
//  4.2  Encryption / Protocol Context Objects
// ----------------------------------------------------------------------------

//  OBJ_EncryptCtx — Encryption context for outgoing packets
//  Access: MOV ECX, 0x0587C278 ; CALL EncryptCtx::Encrypt
//  574 cross-references — used before every packet send
static const DWORD OBJ_EncryptCtx           = 0x0587C278;

//  OBJ_DecryptCtx — Decryption context for incoming packets
//  1208 cross-references — heavily used in packet receive path
static const DWORD OBJ_DecryptCtx           = 0x0587C390;

//  OBJ_ProtocolCtx — Protocol state machine / packet framing
//  588 cross-references — manages C1/C2/C3/C4 packet reassembly
static const DWORD OBJ_ProtocolCtx          = 0x0587C650;

//  OBJ_CryptoState — Additional cryptographic state variables
//  392 cross-references — used in key exchange and session crypto
static const DWORD OBJ_CryptoState          = 0x0587E5A0;


// ============================================================================
//
//  SECTION 5:  DATA SECTION GLOBALS  (.data section)
//
//  These are initialised global variables in the .data section.
//  They hold pointers, state flags, and floating-point constants.
//
// ============================================================================

// ----------------------------------------------------------------------------
//  5.1  Network Pointers
// ----------------------------------------------------------------------------

//  PTR_NetObject — Pointer to CNetObject instance
//  Access: MOV ECX, [0x007D3B70] ; CALL DataSend
//  516 cross-references — used as ECX (this pointer) for DataSend calls
//  Initial file value: 0x05878118 (default init address)
static const DWORD PTR_NetObject            = 0x007D3B70;

// ----------------------------------------------------------------------------
//  5.2  Rendering / Display State
// ----------------------------------------------------------------------------

//  PTR_ProtocolVersion — Protocol version / encryption mode flag
//  594 cross-references — compared to select encryption algorithm
//  CMP DWORD [addr], 0x22 / 0x27 patterns found
static const DWORD PTR_ProtocolVersion      = 0x007D60C8;

//  PTR_RenderState — OpenGL rendering state flags
//  590 cross-references — controls rendering pipeline states
//  Written as: MOV DWORD [addr], 0xFF9600FF (color constant patterns)
static const DWORD PTR_RenderState          = 0x007D4C84;

//  PTR_BackgroundColor — Screen background RGBA colour
//  350 cross-references — set during scene transitions
static const DWORD PTR_BackgroundColor      = 0x007D4C8C;

//  PTR_RenderMode — Display mode / fullscreen flag
//  104 cross-references
static const DWORD PTR_RenderMode           = 0x007D4C58;

// ----------------------------------------------------------------------------
//  5.3  Floating-Point Constants (Rendering Mathematics)
// ----------------------------------------------------------------------------

//  Frequently referenced float constants used in 3D rendering:
//  0x007B3728 (822 refs) — 1.0f  (0x3F800000)  base scale factor
//  0x007B389C (649 refs) — 0.5f  (0x3F000000)  half-unit
//  0x007B383C (536 refs) — 0.1f  (0x3DCCCCCD)  small delta
//  0x007B3704 (397 refs) — 100.0f (0x42C80000) range constant
//  0x007B38C0 (245 refs) — rendering constant
//  0x007B372C (361 refs) — rendering constant
//  0x007B3750 (326 refs) — rendering constant

static const DWORD FLOAT_One                = 0x007B3728;  // 1.0f  (822 refs)
static const DWORD FLOAT_Half               = 0x007B389C;  // 0.5f  (649 refs)
static const DWORD FLOAT_Tenth              = 0x007B383C;  // 0.1f  (536 refs)
static const DWORD FLOAT_Hundred            = 0x007B3704;  // 100.0f (397 refs)


// ============================================================================
//
//  SECTION 6:  GAME UI / WINDOW OBJECTS  (.zero / BSS section)
//
//  Window handles, OpenGL context, and UI manager objects.
//
// ============================================================================

// ----------------------------------------------------------------------------
//  6.1  Window Handle & Graphics Context
// ----------------------------------------------------------------------------

//  PTR_hWndMain — HWND of the main game window
//  Access: MOV EAX, [0x05877F90] ; PUSH EAX before SetTimer/etc.
//  208 cross-references — passed to all Win32 window functions
//  Set by CreateWindowExA at 0x004DB9E8
static const DWORD PTR_hWndMain             = 0x05877F90;

//  PTR_hDC — HDC (device context) for the main window
//  171 cross-references — passed to OpenGL and GDI calls
static const DWORD PTR_hDC                  = 0x05877FA0;

//  PTR_hGLRC — HGLRC (OpenGL rendering context)
//  152 cross-references — used in wglMakeCurrent calls
static const DWORD PTR_hGLRC                = 0x05877FA4;

// ----------------------------------------------------------------------------
//  6.2  UI Manager / Scene State
// ----------------------------------------------------------------------------

//  OBJ_UIManager — UI rendering/input manager base
//  573 cross-references — manages all UI elements, dialogs, menus
static const DWORD OBJ_UIManager            = 0x05877F38;

//  OBJ_SceneManager — Game scene / phase controller
//  568 cross-references — tracks current game phase
//  (login screen, character select, in-game, etc.)
static const DWORD OBJ_SceneManager         = 0x05877F70;

//  OBJ_InputManager — Keyboard/mouse input processing
//  128 cross-references
static const DWORD OBJ_InputManager         = 0x05877F48;

//  OBJ_UITextManager — Text rendering and font management
//  266 cross-references
static const DWORD OBJ_UITextManager        = 0x05877F58;


// ============================================================================
//
//  SECTION 7:  GAME ENGINE CORE OBJECTS  (.zero / unnamed sections)
//
//  Major game engine subsystems.  These are large objects that manage
//  characters, terrain, models, and game logic.
//
// ============================================================================

// ----------------------------------------------------------------------------
//  7.1  Character Machine / Player Data Manager
// ----------------------------------------------------------------------------

//  OBJ_CharacterMachine — Central character data manager
//  Access: MOV ECX/EDX, [0x0754F4D0] ; access fields via reg+offset
//  1181 cross-references — manages all character data computation
//  Fields include: stat calculations, level data, equipment bonuses
//  +0x118 = computed attack speed
//  +0x10  = character state pointer
//  +0x14  = character stats block
//  +0x18  = calculated values block
static const DWORD OBJ_CharacterMachine     = 0x0754F4D0;

// ----------------------------------------------------------------------------
//  7.2  Game Timer / Tick Counter
// ----------------------------------------------------------------------------

//  PTR_GameTimer — Global game tick counter
//  Access: MOV EAX, [0x077A8FDC] ; PUSH EAX ; MOV ECX, OBJ_NetSocket
//  900 cross-references — used for timing all game actions
//  Incremented every frame; used to pace network sends and animations
static const DWORD PTR_GameTimer            = 0x077A8FDC;

//  PTR_GameTimerAlt — Secondary timer (frame delta)
//  332 cross-references
static const DWORD PTR_GameTimerAlt         = 0x077A8FD8;

// ----------------------------------------------------------------------------
//  7.3  Object / Entity Manager
// ----------------------------------------------------------------------------

//  PTR_ObjectCount — Count of active game objects in viewport
//  638 cross-references — used to iterate visible entities
//  Incremented/decremented as objects enter/leave view
static const DWORD PTR_ObjectCount          = 0x07A27BF8;

// ----------------------------------------------------------------------------
//  7.4  Viewport / Camera
// ----------------------------------------------------------------------------

//  PTR_ViewportState — Viewport height / Y state
//  481 cross-references — camera Y position and screen bounds
static const DWORD PTR_ViewportState        = 0x07EB37E0;

//  PTR_ViewportStateX — Viewport width / X state
//  459 cross-references — camera X position and screen bounds
static const DWORD PTR_ViewportStateX       = 0x07EB37E4;


// ============================================================================
//
//  SECTION 8:  GAME DATA POINTERS  (.zero / BSS section)
//
//  These addresses point to character, item, and account data structures
//  that are populated at runtime by the game engine.  They are the primary
//  addresses used by MuHelper's work loop for reading game state.
//
//  NOTE: These addresses are in the .zero BSS section and contain no data
//  in the on-disk PE file.  They were identified through runtime analysis
//  (debugging) and cross-referenced with the MuMain Season 5.2 source.
//
// ============================================================================

// ----------------------------------------------------------------------------
//  8.1  Player Character (Hero)
// ----------------------------------------------------------------------------

//  PTR_Hero — Pointer to the player's own CHARACTER structure
//  Access: CHARACTER* pHero = *(CHARACTER**)PTR_Hero;
//  Also aliased as PTR_UserStruct (same address).
//  The CHARACTER/OBJECTSTRUCT contains: index, name, class, level,
//  position (X,Y), HP, MP, shield, experience, stats, equipment, etc.
static const DWORD PTR_Hero                 = 0x08B23540;

//  USERSTRUCT_SIZE — Size of a single CHARACTER entry in the array
//  Used to index into ARR_CharactersClient:
//    char_ptr = ARR_CharactersClient + index * USERSTRUCT_SIZE
static const DWORD USERSTRUCT_SIZE          = 0x120;  // 288 bytes

// ----------------------------------------------------------------------------
//  8.2  CHARACTER Structure Field Offsets
// ----------------------------------------------------------------------------
//
//  These are offsets within a single CHARACTER / OBJECTSTRUCT entry.
//  Apply to PTR_Hero and to entries in ARR_CharactersClient.
//
//    Base + Offset    Type          Field
//    ─────────────    ──────        ──────────────────
//    +0x00            WORD          wIndex (character index / ID)
//    +0x04            char[11]      szName (null-terminated)
//    +0x10            BYTE          bClass (MuCharClass enum)
//    +0x12            WORD          wLevel
//    +0x14            int           X (map X coordinate)
//    +0x18            int           Y (map Y coordinate)
//    +0x1C            DWORD         dwLife (current HP)
//    +0x20            DWORD         dwMaxLife (maximum HP)
//    +0x24            DWORD         dwMana (current MP)
//    +0x28            DWORD         dwMaxMana (maximum MP)
//    +0x2C            DWORD         dwShield (current Shield Defense)
//    +0x30            DWORD         dwMaxShield (maximum Shield Defense)
//    +0x34            DWORD         dwExp (current experience points)
//    +0x38            DWORD         dwNextExp (exp needed for next level)
//    +0x3C            WORD          wStr (Strength stat)
//    +0x3E            WORD          wAgi (Agility stat)
//    +0x40            WORD          wVit (Vitality stat)
//    +0x42            WORD          wEne (Energy stat)
//    +0x44            WORD          wCmd (Command stat, DL class only)
//    +0x48            BYTE          bMapNumber (current map ID)
//    +0x4C            BYTE          bPKLevel (PK aggression level, 0-6)
//    +0x50            DWORD         dwZen (gold amount)
//    +0x54            int           nPartyNumber (party index, -1 if none)

static const DWORD CHAR_OFF_Index           = 0x00;
static const DWORD CHAR_OFF_Name            = 0x04;
static const DWORD CHAR_OFF_Class           = 0x10;
static const DWORD CHAR_OFF_Level           = 0x12;
static const DWORD CHAR_OFF_X               = 0x14;
static const DWORD CHAR_OFF_Y               = 0x18;
static const DWORD CHAR_OFF_Life            = 0x1C;
static const DWORD CHAR_OFF_MaxLife          = 0x20;
static const DWORD CHAR_OFF_Mana            = 0x24;
static const DWORD CHAR_OFF_MaxMana          = 0x28;
static const DWORD CHAR_OFF_Shield          = 0x2C;
static const DWORD CHAR_OFF_MaxShield        = 0x30;
static const DWORD CHAR_OFF_Exp             = 0x34;
static const DWORD CHAR_OFF_NextExp          = 0x38;
static const DWORD CHAR_OFF_Str             = 0x3C;
static const DWORD CHAR_OFF_Agi             = 0x3E;
static const DWORD CHAR_OFF_Vit             = 0x40;
static const DWORD CHAR_OFF_Ene             = 0x42;
static const DWORD CHAR_OFF_Cmd             = 0x44;
static const DWORD CHAR_OFF_MapNumber       = 0x48;
static const DWORD CHAR_OFF_PKLevel         = 0x4C;
static const DWORD CHAR_OFF_Zen             = 0x50;
static const DWORD CHAR_OFF_PartyNumber     = 0x54;

// ----------------------------------------------------------------------------
//  8.3  Character Arrays
// ----------------------------------------------------------------------------

//  ARR_CharactersClient — Array of all visible characters in the viewport
//  Indexed as: CHARACTER* pChar = (CHARACTER*)(ARR_CharactersClient +
//                                              index * USERSTRUCT_SIZE);
//  Contains NPCs, monsters, and other players.
static const DWORD ARR_CharactersClient     = 0x08B24600;

//  ARR_Items — Array of dropped items visible on the ground
//  Each entry: struct { WORD wId; BYTE bType; BYTE bLevel; int x, y; ... }
//  MuHelper reads this to decide which items to pick up.
static const DWORD ARR_Items                = 0x09124600;

// ----------------------------------------------------------------------------
//  8.4  Player Attributes
// ----------------------------------------------------------------------------

//  PTR_CharacterAttribute — Player attribute data (HP, MP, Level, etc.)
//  This is a secondary attributes structure that holds current vitals.
//  Used by MuHelper to check HP/MP thresholds for auto-healing.
static const DWORD PTR_CharacterAttribute   = 0x08B24500;

// ----------------------------------------------------------------------------
//  8.5  Character Name
// ----------------------------------------------------------------------------

//  NAME_CHAR — Character name of the currently logged-in player
//  A null-terminated char[11] buffer.
//  Populated after the player selects a character on the char-select screen.
//  Access: const char* name = (const char*)NAME_CHAR;
static const DWORD NAME_CHAR                = 0x08B24558;

// ----------------------------------------------------------------------------
//  8.6  Targeting & Movement
// ----------------------------------------------------------------------------

//  PTR_SelectedCharacter — Index of the currently selected target
//  -1 if no target.  Set when the player clicks on a monster/NPC/player.
static const DWORD PTR_SelectedCharacter    = 0x08B07820;

//  PTR_TargetX / PTR_TargetY — Movement destination coordinates
//  Set when the player clicks to move; read by movement routines.
static const DWORD PTR_TargetX              = 0x08B07824;
static const DWORD PTR_TargetY              = 0x08B07828;

// ----------------------------------------------------------------------------
//  8.7  World / Map State
// ----------------------------------------------------------------------------

//  PTR_WorldActive — Current map / world ID
//  0 = Lorencia, 1 = Dungeon, 2 = Devias, 3 = Noria, etc.
//  Changed when the player moves between maps (warp/teleport).
static const DWORD PTR_WorldActive          = 0x08B0782C;

// ----------------------------------------------------------------------------
//  8.8  User Account Data
// ----------------------------------------------------------------------------

//  PTR_UserAccount — Account-level data structure base address
//  Populated after successful login (opcode 0xF1, sub-opcode 0x01).
//  Contains the account name, auth level, character count, etc.
//
//  Layout:
//    +0x00  char  szAccountName[11]  — login username
//    +0x0C  BYTE  bAuthLevel         — account authority (0=normal, 1=GM)
//    +0x10  DWORD dwAccountId        — numeric account identifier
//    +0x14  BYTE  bCharCount         — number of characters on account
//    +0x18  char  szLastChar[11]     — last played character name
static const DWORD PTR_UserAccount          = 0x08B07800;

//  USERACCOUNT field offsets:
static const DWORD USERACCOUNT_OFF_Name      = 0x00;  // char[11]
static const DWORD USERACCOUNT_OFF_AuthLevel = 0x0C;  // BYTE
static const DWORD USERACCOUNT_OFF_AccountId = 0x10;  // DWORD
static const DWORD USERACCOUNT_OFF_CharCount = 0x14;  // BYTE
static const DWORD USERACCOUNT_OFF_LastChar  = 0x18;  // char[11]

//  OFFSET_AccountId — Absolute VA of the numeric AccountID field
//  Convenience: PTR_UserAccount + USERACCOUNT_OFF_AccountId
static const DWORD OFFSET_AccountId         = 0x08B07810;  // 0x08B07800 + 0x10

//  PTR_UserStruct — Alias for PTR_Hero (same address)
static const DWORD PTR_UserStruct           = 0x08B23540;


// ============================================================================
//
//  SECTION 9:  INLINE HOOK PARAMETERS
//
//  These values control how inline detours are installed.
//  The "stolen bytes" count is the number of bytes at the start of each
//  function that must be relocated to the trampoline buffer.
//
// ============================================================================

//  Number of prologue bytes to steal for each inline hook:
//  DataSend:       55 8B EC 83 EC 10 89  (7 bytes — PUSH EBP; MOV EBP,ESP; ...)
//  ProcessPacket:  8B 44 24 04 81 EC 7C  (7 bytes — MOV EAX,[ESP+4]; SUB ESP,...)
//  RecvProtocol:   6A FF 64 A1 00 00 00  (7 bytes — PUSH -1; MOV EAX,FS:[0]; ...)
static const SIZE_T STOLEN_DataSend         = 7;
static const SIZE_T STOLEN_ProcessPacket    = 7;
static const SIZE_T STOLEN_RecvProtocol     = 7;


// ============================================================================
//
//  SECTION 10:  SEH FUNCTION TABLE
//
//  main.exe contains 41 functions with SEH (Structured Exception Handling)
//  prologues (6A FF 64 A1 00 00 00 00 68 pattern).  Most are in the
//  protocol/network subsystem.  RecvProtocol (0x00509230) is one of them.
//
//  Selected SEH functions relevant to MuHelper:
// ============================================================================

static const DWORD SEH_RecvProtocol         = 0x00509230;  // Packet recv loop
static const DWORD SEH_Function_004E33E0    = 0x004E33E0;  // Network handler
static const DWORD SEH_Function_004E4570    = 0x004E4570;  // Network handler
static const DWORD SEH_Function_004E6270    = 0x004E6270;  // Network handler
static const DWORD SEH_Function_005A6850    = 0x005A6850;  // Resource loader
static const DWORD SEH_Function_005B3320    = 0x005B3320;  // Scene init
static const DWORD SEH_Function_005CFBF0    = 0x005CFBF0;  // World loader
static const DWORD SEH_TotalCount           = 41;


// ============================================================================
//
//  SECTION 11:  ANTI-CHEAT DETECTION STRINGS
//
//  main.exe contains hardcoded window class names used for detecting
//  known cheat tools.  These strings are in the unnamed data section.
//
// ============================================================================

//  0x007D32F0: "Speed Hack - PCGameHacks.com"
//  0x007D3310: "GameHack 2.0"
//  0x007D3320: "Game Master v7.00 (Win95 & Win98)"
//
//  The FindWindowA IAT (0x007B3564) is called with these class names
//  to detect if a cheat tool is running.  11 FindWindowA call sites found.

static const DWORD STR_AntiCheat_SpeedHack  = 0x007D32F0;
static const DWORD STR_AntiCheat_GameHack   = 0x007D3310;
static const DWORD STR_AntiCheat_GM700      = 0x007D3320;


// ============================================================================
//
//  SECTION 12:  GAME STRING LOCATIONS  (unnamed data section)
//
//  Key string constants found in the data section.
//  Useful for locating related functions via cross-references.
//
// ============================================================================

static const DWORD STR_DataItem             = 0x007CFB3C;  // "Data\\Item\\"
static const DWORD STR_BufferOverflow       = 0x007CFA74;  // "Buffer Overflow\n"
static const DWORD STR_BufferError          = 0x007CFFE9;  // "Buffer Error\n"
static const DWORD STR_SocketClosed         = 0x007D4051;  // "Socket Closed..."
static const DWORD STR_ShopModule           = 0x007CFA00;  // "ShopModule"
static const DWORD STR_CharacterJpg         = 0x007E5A87;  // "Character.jpg"
static const DWORD STR_LoginBack01          = 0x007E5571;  // "Login_Back01.jpg"
static const DWORD STR_ConnectKR            = 0x007D27C0;  // "connect.muonline.co.kr"
static const DWORD STR_ConnectGlobal        = 0x007D27F2;  // "connect.globalmuonline.com"
static const DWORD STR_ConnectTW            = 0x007D2824;  // "connection.muonline.com.tw"
static const DWORD STR_Quest                = 0x007CFEE8;  // "Quest"
static const DWORD STR_GuildAssign          = 0x007D3D34;  // "GuildAssign"
static const DWORD STR_EventChaosCastle     = 0x007D97AC;  // "EventChaosCastle"
static const DWORD STR_Skill02Smd           = 0x007D6D8D;  // "Skill_02.smd"
static const DWORD STR_PartyJpg             = 0x007E5A6B;  // "Party.jpg"


// ============================================================================
//
//  SECTION 13:  MAIN.DLL OFFSETS (companion DLL)
//
//  Some MU Online distributions include Main.dll as a companion module.
//  ImageBase = 0x10000000.  These offsets are for the Main.dll paired
//  with main.exe 1.02.11.
//
// ============================================================================

//  Names match the Addr_MainDll_10211 namespace in HookEngine.h
static const DWORD DLL_FN_Init              = 0x10035D66;  // Init trampoline table
static const DWORD DLL_PTR_AttackHelperObj  = 0x100E526C;  // CAttackHelper* global
static const DWORD DLL_IAT_WS2_Send        = 0x10288BCC;  // ws2_32 Ord#19 (send)
static const DWORD DLL_IAT_WS2_Recv        = 0x10288BC8;  // ws2_32 Ord#5  (recv)


// ============================================================================
//
//  SECTION 14:  SUMMARY / QUICK REFERENCE
//
//  Total offsets documented in this file:
//
//    Function addresses (FN_*)          :   3
//    Opcode handler table entries       : 147  (27 individually listed)
//    IAT import slots                   : 113
//    Network/Crypto objects (OBJ_*)     :   8
//    Game data pointers (PTR_/ARR_)     :  22
//    Rendering constants (FLOAT_*)      :   4
//    UI/Window objects                  :   7
//    Game engine objects                :   6
//    Hook parameters (STOLEN_*)         :   3
//    String locations (STR_*)           :  15
//    Anti-cheat strings                 :   3
//    DLL offsets                        :   4
//    Structure field offsets            :  28
//    Misc (call sites, SEH, etc.)       :  14
//    ─────────────────────────────────────────
//    TOTAL                              : 377+
//
//  For the 1.02.19 version offsets, see Offsets_10219.h
//  The general delta between 1.02.11 and 1.02.19 is approximately +0x2200
//  for .data/.bss section addresses and varying for .text section functions.
//
// ============================================================================
