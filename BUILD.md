# MuHelper v2 — Build Guide

## Структура файлов

```
MuHelper/
├── Shared/
│   └── MuHelperPackets.h          ← общие пакеты (клиент + сервер)
├── Server/
│   ├── MuHelperManager.h/.cpp     ← серверная логика
│   └── MuHelperHook.cpp           ← точки интеграции
├── Client/
│   ├── dllmain.cpp                ← DLL-entry
│   ├── HookEngine.h/.cpp          ← хуки (VERIFIED адреса)
│   ├── MuHelperClient.h/.cpp      ← клиентская логика
│   ├── MuHelperUI.h/.cpp          ← ImGui-оверлей (OpenGL)
│   └── imgui/                     ← сюда положить imgui
├── muhelper_schema.sql
└── SCAN_REPORT.md
```

---

## Требования к компиляции

- **Visual Studio 2010** (Platform Toolset v100) или выше
- Операционная система Windows 10 Pro x86/x64
- Код совместим с C++ компилятором VS 2010 (не использует C++11-only конструкции, кроме `auto`, `nullptr`, `static_assert`, `std::array`, `std::function`, `std::unordered_map` и лямбд — всё поддерживается в VS 2010)

---

## A. Серверная сторона (GameServer 1.00.18)

### Требования
- Visual Studio 2010 (или тот же компилятор что и GS)
- MySQL C API или ODBC

### Новые возможности сервера
- **Profile save/load** — 5 именованных профилей на персонажа
- **SkillStatus пакеты** — уведомление о кулдаунах
- **PartyHP пакеты** — статус HP/MP пати каждые 2 сек
- **EXP/Kill rate** — подсчёт per-hour статистики
- **Stop on level up** / **Stop on EXC drop** — новые режимы паузы

### SQL (добавить к существующей таблице)
```sql
-- Основная таблица (v2: увеличен размер cfg до 96 байт)
CREATE TABLE IF NOT EXISTS MuHelperConfig (
    CharIdx  INT UNSIGNED NOT NULL PRIMARY KEY,
    cfg      BLOB         NOT NULL,
    UpdatedAt TIMESTAMP   NOT NULL DEFAULT CURRENT_TIMESTAMP
             ON UPDATE CURRENT_TIMESTAMP
) ENGINE=InnoDB;

-- Профили
CREATE TABLE IF NOT EXISTS MuHelperProfiles (
    CharIdx  INT UNSIGNED NOT NULL,
    SlotIdx  TINYINT      NOT NULL,
    Name     VARCHAR(16)  NOT NULL DEFAULT '',
    cfg      BLOB         NOT NULL,
    PRIMARY KEY (CharIdx, SlotIdx)
) ENGINE=InnoDB;
```

---

## B. Клиентская DLL (main.exe 1.02.11 или 1.02.19)

### Выбор версии main.exe

По умолчанию проект собирается для main.exe **1.02.11**.  
Для сборки под main.exe **1.02.19** (Season 3 Ep 1):

1. В VS-проекте → C/C++ → Preprocessor Definitions:
   добавить `MUHELPER_TARGET_10219`
2. Или в Property Sheet: `/D MUHELPER_TARGET_10219`

Адреса (офсеты) для 1.02.19 определены в `Client/Offsets_10219.h`.  
**NAME_CHAR** для 1.02.19: `0x08B26758` (глобальный буфер имени персонажа).

### Файлы для 1.02.19 (новые)
```
Client/Offsets_10219.h           ← все офсеты для 1.02.19 + NAME_CHAR
Client/MuHelperWorkLoop.h/.cpp   ← логика автоматизации (порт из MuMain S5.2)
Shared/MuHelperNetData.h         ← MUHELPER_NET_DATA (совместим с MuMain)
```

### ImGui — скачать и добавить

```
cd MuHelper/Client
mkdir imgui && cd imgui

# Скачать файлы с https://github.com/ocornut/imgui (tag v1.90.x):
imgui.h
imgui.cpp
imgui_draw.cpp
imgui_tables.cpp
imgui_widgets.cpp
imgui_internal.h
imconfig.h
imstb_rectpack.h
imstb_textedit.h
imstb_truetype.h
backends/imgui_impl_opengl2.h
backends/imgui_impl_opengl2.cpp
backends/imgui_impl_win32.h
backends/imgui_impl_win32.cpp
```

### Настройки VS-проекта

| Параметр | Значение |
|---|---|
| Тип | Dynamic Library (.dll) |
| Platform | Win32 (x86) |
| Runtime Library | /MT (Multi-threaded) |
| Additional Include | `$(ProjectDir)` |
| Additional Libraries | `opengl32.lib` |
| Output name | `MuHelper.dll` |

### Все .cpp в проект
```
dllmain.cpp
HookEngine.cpp
MuHelperClient.cpp
MuHelperWorkLoop.cpp
MuHelperUI.cpp
imgui/imgui.cpp
imgui/imgui_draw.cpp
imgui/imgui_tables.cpp
imgui/imgui_widgets.cpp
imgui/backends/imgui_impl_opengl2.cpp
imgui/backends/imgui_impl_win32.cpp
```

### Инжекция DLL

**Вариант 1 (простейший):** переименовать в `dinput8.dll`, положить рядом с `main.exe`.
Игра сама загрузит DLL при запуске (proxy-load через IAT-запись на dinput8).

**Вариант 2:** CreateRemoteThread + LoadLibraryA через внешний инжектор.

---

## Горячие клавиши

| Клавиша | Действие |
|---|---|
| `F10` | Открыть/закрыть панель MuHelper |
| `F9` | Start/Stop хелпер (без открытия панели) |

---

## Новые возможности v2

### Функционал
| Функция | Описание |
|---|---|
| Ротация скиллов | 3 дополнительных слота, чередуются каждые N атак |
| Профили × 5 | Именованные конфиги, сохраняются на сервере |
| Кулдаун-бары | 8 слотов скиллов в шапке окна |
| Стат оверлей | Всегда-видимый мини-HUD (kills/zen/items/time) |
| HP пати | Живые бары HP/MP для каждого члена пати |
| EXP/Kill rate | Подсчёт per-hour в реальном времени |
| Stop on drop | Пауза при выпадении EXC/ANC предмета |
| Stop on level up | Пауза при получении уровня |
| Антидот | Автоматическое использование антидота |
| Avoid PvP | Не атаковать игроков |
| Авторемонт | Починка при прочности < 10% |
| Лог с фильтром | 80 записей, цветовая маркировка, поиск |
| 2 слота бафа | Два скила бафа (например BK меч-сила + щит) |

### UI (Dear ImGui, тема dark fantasy)
- Тёмно-синяя тема с золотыми акцентами
- Градиентные полоски HP/MP/SD/EXP
- 8-слотовая панель кулдаунов в шапке
- 8 вкладок: Combat / Pickup / Potions / Buffs / Party / Stats / Log / Profiles
- Draggable & resizable окно
- Мини-оверлей (левый верхний угол, полупрозрачный)
- Кнопка Apply+Save только при наличии изменений
