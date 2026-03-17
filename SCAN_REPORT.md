# MuHelper v2 — Scan Report (FINAL)

## Версии бинарей — ПОДТВЕРЖДЕНО

| Файл | Версия | Источник подтверждения |
|---|---|---|
| `main.exe` | **1.02.11** | `Main.dll` debug string: `"[ ] Client Version : 1.02.11jpn"` |
| `Main.dll` | companion mod | `Init()` → 50 E9-трамплинов, `CPacketManager`, `CAttackHelper` |
| `GameServer.exe` | **1.00.18** | Указано пользователем |
| `GgSrvDll.dll` | — | Экспорты `GGAuthInitUser/GetQuery/CheckAnswer` |
| `GgAuth.dll` | PrtcLibVer **1.32** | Строка `"PrtcLibVer 1.32"` в .data (0x0000E030) |

---

## Верификация всех адресов (main.exe 1.02.11)

Все адреса верифицированы по реальным байтам бинаря:

| Адрес | Функция | Верификация |
|---|---|---|
| `0x00403D10` | `DataSend` (`__thiscall`) | Пролог `55 8B EC 83 EC 10 89 4D F0` ✓ |
| `0x0050A070` | `ProcessPacket` (опкод-диспатч) | Пролог `8B 44 24 04 81 EC 7C 01 00 00` ✓ |
| `0x00509230` | `RecvProtocol` (recv-луп SEH) | Пролог `6A FF 64 A1 00 00 00 00 68` ✓ |
| `0x007B308C` | IAT-слот `SwapBuffers` (.data) | `FF 15 8C 30 7B 00` в коде ✓ |
| `0x007B35CC` | IAT-слот `WSASend` (.data) | DWORD=`0x07AEC028` ✓ |
| `0x007B35BC` | IAT-слот `recv` (.data) | DWORD=`0x07AEBFFA` ✓ |
| `0x007D3B70` | Глобальный указатель на net-объект | `MOV ECX,[0x007D3B70]` перед DataSend ✓ |
| `0x0050DD9C` | Таблица обработчиков опкодов | 147 записей, entry[0]=`0x0050AA94` ✓ |

---

## GgSrvDll.dll — полная карта API

### Экспорты (верифицированы дизассемблированием)

| Функция | VA | Прототип |
|---|---|---|
| `InitGameguardAuth` | `0x10001020` | `BOOL __cdecl(const char* dllPath, void* pfnCallback)` |
| `CleanupGameguardAuth` | `0x100010D0` | `void __cdecl()` |
| `AddAuthProtocol` | `0x10001110` | `int __cdecl(BYTE opcode, void* handler)` |
| `GGAuthInitUser` | `0x10001440` | `CCSAuth2* __cdecl(CCSAuth2** ppOut)` |
| `GGAuthCloseUser` | `0x10001460` | `void __cdecl(CCSAuth2*)` |
| `GGAuthGetQuery` | `0x10001480` | `void __cdecl(CCSAuth2*, GGQUERY* pOut)` |
| `GGAuthCheckAnswer` | `0x100014B0` | `void __cdecl(CCSAuth2*, const GGANSWER* pIn)` |
| `CCSAuth2::ctor` | `0x100012B0` | `__thiscall` |
| `CCSAuth2::dtor` | `0x10001300` | `__thiscall` |
| `CCSAuth2::Init` | `0x10001330` | `void __thiscall()` |
| `CCSAuth2::GetAuthQuery` | `0x10001370` | `DWORD __thiscall()` |
| `CCSAuth2::CheckAuthAnswer` | `0x100013D0` | `DWORD __thiscall()` — ret 0=OK 4=FAIL |

### Структура `CCSAuth2` (44 байта, верифицирована из конструктора)

```
+0x00  CCSAuth2*  pPrev            (связанный список)
+0x04  DWORD      dwStateFlag
+0x08  DWORD      dwRefCount
+0x0C  DWORD      dwQuery[0]       (challenge: заполняется GetAuthQuery)
+0x10  DWORD      dwQuery[1]
+0x14  DWORD      dwQuery[2]
+0x18  DWORD      dwQuery[3]
+0x1C  DWORD      dwAnswer[0]      (response: заполняется CheckAuthAnswer)
+0x20  DWORD      dwAnswer[1]
+0x24  DWORD      dwAnswer[2]
+0x28  DWORD      dwAnswer[3]
```

### Глобалы GgSrvDll

| VA | Назначение |
|---|---|
| `0x1000BD30` | `char* g_szDllPath` — путь к ggauth.dll |
| `0x1000BD34` | `void* g_pfnCallback` — GameServer callback |
| `0x1000BD38` | `CCSAuth2* g_pAuthList` — голова linked list |
| `0x1000BD3C` | `BYTE g_bInited` — флаг инициализации |

---

## GgAuth.dll — низкоуровневые обёртки

| Функция | VA | Прототип |
|---|---|---|
| `PrtcGetVersion` | `0x10007470` | `int(DWORD* pOut)` |
| `PrtcGetAuthQuery` | `0x10007490` | `int(DWORD* pState, GGQUERY* pOut)` |
| `PrtcCheckAuthAnswer` | `0x10007510` | `int(DWORD* pState, const GGANSWER*, ...)` |

Внутри грузит `ggauth32.dll` и делегирует туда.

---

## Рендеринг: OpenGL

Игра использует **OpenGL**, не Direct3D. Хук рендеринга:
- IAT-слот `SwapBuffers` по VA `0x007B308C`
- Основной вызов: `0x006CF9E6` (`FF 15 8C 30 7B 00`)

---

## Архитектура MuHelper vs GameGuard

```
GameServer
  └─ InitGameguardAuth("ggauth.dll", callback)   [startup]
  └─ AddAuthProtocol(opcode, handler)             [per GG opcode]
  └─ GGAuthInitUser(&pAuth)                       [client connect]
  └─ GGAuthGetQuery(pAuth, &query) → send to client
  └─ GGAuthCheckAnswer(pAuth, &answer)            [client response]
  └─ GGAuthCloseUser(pAuth)                       [disconnect]

MuHelper опкод 0x97:
  └─ НЕ регистрируется через AddAuthProtocol
  └─ Перехватывается до GG dispatch table
  └─ Полностью прозрачен для GgSrvDll
```

---

## Сигнатуры для portable scan (другие сборки)

```
DataSend:
  55 8B EC 83 EC 10 89 4D F0 8B 45 0C 89 45 F8

ProcessPacket:
  8B 44 24 04 81 EC 7C 01 00 00 3D F5 00 00 00 53

RecvProtocol (SEH frame):
  6A FF 64 A1 00 00 00 00 68 ?? ?? ?? ?? 50
  (потом 3C C1 / 3C C2 / 3C C3 — проверка заголовков пакетов)

SwapBuffers IAT:
  Найти строку SwapBuffers в .idata → IAT-запись → FF 15 [slot]
```
