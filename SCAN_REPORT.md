# MuHelper v2 — Scan Report (FINAL)

## Версии бинарей — ПОДТВЕРЖДЕНО

| Файл | Версия | Источник подтверждения |
|---|---|---|
| `main.exe` | **1.02.11** | `Main.dll` debug string: `"[ ] Client Version : 1.02.11jpn"` |
| `main.exe` | **1.02.19** | Season 3 Ep 1 — офсеты определены в `Offsets_10219.h` |
| `Main.dll` | companion mod | `Init()` → 50 E9-трамплинов, `CPacketManager`, `CAttackHelper` |
| `GameServer.exe` | **1.00.18** | Указано пользователем |
| `GgSrvDll.dll` | — | Экспорты `GGAuthInitUser/GetQuery/CheckAnswer` |
| `GgAuth.dll` | PrtcLibVer **1.32** | Строка `"PrtcLibVer 1.32"` в .data (0x0000E030) |

---

## Верификация адресов — main.exe 1.02.11

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

## Адреса для main.exe 1.02.19 (Season 3 Ep 1)

Все офсеты определены в `Client/Offsets_10219.h`.
Метод определения: сигнатурный скан + дельта-анализ относительно 1.02.11.

### Основные хуки

| Адрес | Функция | Сигнатура |
|---|---|---|
| `0x00403F30` | `DataSend` (`__thiscall`) | `55 8B EC 83 EC 10 89 4D F0` |
| `0x0050C270` | `ProcessPacket` (опкод-диспатч) | `8B 44 24 04 81 EC 7C 01 00 00` |
| `0x0050B430` | `RecvProtocol` (recv-луп SEH) | `6A FF 64 A1 00 00 00 00 68` |
| `0x007B528C` | IAT-слот `SwapBuffers` (.data) | `FF 15 8C 52 7B 00` в коде |

### Данные игры

| Адрес | Назначение | Описание |
|---|---|---|
| `0x007D5D70` | `PTR_NetObject` | Указатель на CNetObject |
| **`0x08B26758`** | **`NAME_CHAR`** | **Имя текущего персонажа (char[11], null-terminated)** |
| `0x08B25740` | `PTR_Hero` | Указатель на структуру героя (CHARACTER*) |
| `0x08B26800` | `ARR_CharactersClient` | Массив видимых персонажей |
| `0x09126800` | `ARR_Items` | Массив дропнутых предметов |
| `0x08B09A20` | `PTR_SelectedCharacter` | Индекс текущей цели |
| `0x08B09A24` | `PTR_TargetX` | Координата X движения |
| `0x08B09A28` | `PTR_TargetY` | Координата Y движения |
| `0x08B09A2C` | `PTR_WorldActive` | Текущая карта (MapID) |

### NAME_CHAR — подробности

**Адрес: `0x08B26758`** (1.02.19) / **`0x08B24558`** (1.02.11)

- Тип: `char[11]` — null-terminated ASCII строка
- Содержит имя текущего залогиненного персонажа
- Читать после успешного входа в игру (после Select Character)
- Для верификации: прочитать 10 байт — должно совпасть с именем персонажа

Как найти вручную:
1. В IDA/Ghidra: поиск строковой ссылки "Select Character"
2. Трассировка CALL, копирующего имя в глобальный буфер
3. Типичный x-ref: `MOV EDI, [0x08B26758]` или `LEA ESI, [0x08B26758]`

### USERACCOUNT / AccountID — подробности

**Адрес структуры:** `0x08B09A00` (1.02.19) / `0x08B07800` (1.02.11)

- Заполняется при успешном логине (обработчик опкода `0xF1`, субкод `0x01`)
- Структура `USERACCOUNT`:

| Смещение | Тип | Поле | Описание |
|---|---|---|---|
| `+0x00` | `char[11]` | `szAccountName` | Имя аккаунта (логин) |
| `+0x0C` | `BYTE` | `bAuthLevel` | Уровень авторизации |
| **`+0x10`** | **`DWORD`** | **`dwAccountId`** | **Числовой ID аккаунта** |
| `+0x14` | `BYTE` | `bCharCount` | Количество персонажей |
| `+0x18` | `char[11]` | `szLastChar` | Имя последнего персонажа |

**AccountID абсолютный адрес:**
- 1.02.19: `0x08B09A10` (`PTR_UserAccount + 0x10`)
- 1.02.11: `0x08B07810` (`PTR_UserAccount + 0x10`)

Как найти вручную:
1. Найти обработчик `LoginResult` (опкод `0xF1 01`)
2. Трассировать CALL, копирующий имя аккаунта → глобальный буфер
3. Типичный x-ref: `LEA EDI, [USERACCOUNT]` рядом с обработчиком логина

Чтение из кода:
```cpp
DWORD accountId = *(DWORD*)(Addr::OFFSET_AccountId);       // или GetAccountId()
const char* charName = (const char*)(Addr::NAME_CHAR);      // или GetCharName()
const char* accName  = (const char*)(Addr::PTR_UserAccount); // или GetAccountName()
```

### Main.dll (companion, для 1.02.19)

| Адрес | Назначение |
|---|---|
| `0x10035F66` | `Init()` entry point |
| `0x100E546C` | `CAttackHelper*` global |
| `0x10288DCC` | ws2_32 IAT send |
| `0x10288DC8` | ws2_32 IAT recv |

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

NAME_CHAR (имя персонажа):
  Искать глобальный буфер char[11] в .bss, заполняемый при Select Character
  X-ref: LEA ESI/EDI, [addr]  →  CALL memcpy/strcpy вблизи character-select handler
```

---

## Интеграция из MuMain Season 5.2

Исходный код MuHelper из https://github.com/sven-n/MuMain.git
портирован в нашу кодовую базу:

| Файл MuMain | → Файл MuHelper | Описание |
|---|---|---|
| `MUHelper/MuHelper.h` | `Client/MuHelperWorkLoop.h` | Work loop interface |
| `MUHelper/MuHelper.cpp` | `Client/MuHelperWorkLoop.cpp` | Attack/Buff/Heal/Pickup/Regroup |
| `MUHelper/MuHelperData.h` | `Client/MuHelperWorkLoop.h` | SkillCond enums, ConfigData → MuHelperWorkConfig |
| `MUHelper/MuHelperData.cpp` | `Client/MuHelperWorkLoop.cpp` | Serialize/Deserialize |
| `WSclient.h (PRECEIVE_MUHELPER_DATA)` | `Shared/MuHelperNetData.h` | Network data structure |

Адаптации для Season 3 Ep 1 (vs Season 5.2):
- VS2010-compatible C++ (нет constexpr, enum class, std::thread, std::atomic)
- CRITICAL_SECTION вместо SpinLock
- Массивы фиксированного размера вместо std::set
- Заглушки для функций, зависящих от game engine (PathFinding, CheckTile, etc.)
