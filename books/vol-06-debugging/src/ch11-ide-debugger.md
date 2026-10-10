# Глава 11. Отладчик в IDE

## Зачем эта тема

В Cursor / VS Code удобно ставить breakpoint кликом и смотреть Variables. Под Linux это почти всегда **C/C++ extension** + **GDB** (`cppdbg`). Настройка один раз экономит время на каждом упражнении `multithread/`.

## Зачем IDE, если есть терминал

Breakpoint мышкой на строке **`wait`** быстрее, чем считать номер для **`break file:line`**. Панель Variables экономит **`info locals`**. Но **многопоточная** диагностика часто всё равно уезжает в Debug Console с **`thread apply all bt`** — IDE не заменяет знание GDB команд. Используй IDE для **скорости входа**, терминал/cgdb — для **полного контроля** в SSH.

## Минимальный launch.json

В workspace `.vscode/launch.json` (если открыт `training/multithread` или корень `training`):

```json
{
  "version": "0.2.0",
  "configurations": [
    {
      "name": "Debug 03_oracle",
      "type": "cppdbg",
      "request": "launch",
      "program": "${workspaceFolder}/build/03_oracle",
      "args": [],
      "cwd": "${workspaceFolder}",
      "environment": [],
      "externalConsole": false,
      "MIMode": "gdb",
      "miDebuggerPath": "/usr/bin/gdb",
      "setupCommands": [
        {
          "text": "-enable-pretty-printing",
          "ignoreFailures": true
        }
      ],
      "preLaunchTask": "cmake build oracle"
    }
  ]
}
```

Пути подстрой под то, как открыт проект: `workspaceFolder` может быть `.../training` или `.../training/multithread`.

## preLaunchTask

В `tasks.json` задача собирает target перед debug:

```json
{
  "label": "cmake build oracle",
  "type": "shell",
  "command": "cmake --build build --target 03_oracle",
  "options": { "cwd": "${workspaceFolder}/multithread" }
}
```

Без task — вручную `cmake --build` перед F5.

## CMake Tools

Расширение **CMake Tools** даёт «Debug» на выбранном target без ручного `program` — выбираешь kit (GCC), build type Debug, target `06_thread_pool`. Меньше ручного JSON — предпочтительно, если уже пользуешься.

## Breakpoints и threads

Панель **Call Stack**, **Variables**, **Watch**. Для потоков — dropdown thread в VS Code debug toolbar (если включено). Иначе Debug Console: `-exec thread apply all bt`.

## clangd vs debugger

**clangd** (том 5) — статический анализ; **GDB** — runtime. Красная точка на несуществующей строке — пересобери Debug.

## Inline suggest отключён

По твоим настройкам Cursor Tab выключен — отладка не заменяет чтение кода самому.

## Типичные заблуждения

«F5 без сборки отладит новый код» — только после build. «IDE не использует gdb» — использует. «launch.json в репо обязателен» — локальная настройка.

## Краткое резюме

`cppdbg` + `gdb`, `program` на `build/target`, Debug build, optional preLaunchTask. CMake Tools упрощает. Debug Console для GDB-команд. Дальше — STL print.

## workspaceFolder и multithread

Если корень workspace **`training`**, program **`${workspaceFolder}/multithread/build/03_oracle`**, cwd **`${workspaceFolder}/multithread`**. Если workspace только **`multithread`**, убери префикс. Неверный path — №1 ошибка F5.

## miDebuggerPath

**`/usr/bin/gdb`**. Clang **lldb** — другой **`MIMode`**, не смешивай без причины.

## setupCommands

**`-enable-pretty-printing`** — STL. **`ignoreFailures`: true** — если python printers нет, debug не падает.

## Debug Console

**`-exec bt`**, **`-exec thread apply all bt`**. Префикс **`-exec`** обязателен для raw GDB в VS Code/Cursor.

## CMake Tools Debug

Kit GCC, variant Debug, target из списка. **`cmake.debugConfig`** merge — смотри доку расширения. Меньше ручного JSON — меньше ошибок path.

## Breakpoints и пересборка

Красный breakpoint «unverified» — нет символов или exe stale. **`cmake --build`**.

## sourceFileMap

Если исходники на другой машине — mapping для CI artifacts; для локального training не нужно.

## § К собесу

IDE debugger on Linux = GDB + MI.

## justMyCode (concept)

Some debuggers skip system code; cppdbg settings vary — if step skips weird, check extension docs.

## conditional breakpoints in UI

Right-click breakpoint → condition — same as **`break ... if`**.

## watch panel

GUI watch = gdb watch with same caveats multithread.

## launch multiple configs

One json entry per multithread target — 01..08 — copy-paste with program path change.

## core dump debugging in IDE

Possible with **`coreDumpPath`** — advanced; terminal gdb enough for course.

## Cursor-specific

Same as VS Code cppdbg; workspace trust for `.vscode`.

## stopAtEntry

Launch flag stop at entry — like **`starti`** lite.

## environment sanitizer

Future tom 7 may add env in launch — separate config name **`Debug TSan`**.

## compound tasks

Build all vs one target — match **`check.sh`** scope when debugging integration.

## detach debugging

Stop debugging without killing process — rare.

## Практическое закрепление (5–10 минут)

Открой Debug-сборку любого **зелёного** target из `multithread/build/`. Запусти GDB, поставь **`break main`**, **`run`**, выполни **`next`** пять раз, **`bt`**, **`info locals`**, **`quit`**. Повтори на другом target — muscle memory важнее заучивания глав. Если что-то не совпало с текстом главы, проверь cwd и путь к exe: nine из десяти «GDB сломан» — неверный путь или сборка без **`-g`**.

## Связь с главой тома 3 без спойлера кода

Вернись мысленно к инварианту упражнения, которое отлаживаешь: что должно быть истинно перед выходом из **`wait`**, когда очередь считается пустой, когда **`submit`** должен молчать. GDB показывает **момент**, когда инвариант уже нарушен или ещё не достигнут. Исправление остаётся в голове и в **`task.cpp`**, не в команде **`continue`**.

## § Коротко к собесу (повтор)

Сформулируй вслух одним предложением главную мысль этой главы и назови **одну** команду GDB, без которой тема не работает. Если не можешь — перечитай «Краткое резюме» и вернись сюда завтра; spaced repetition лучше зубрёжки за вечер.

## Вопросы для самопроверки

1. Что такое `MIMode": "gdb"`?
2. Зачем preLaunchTask?
3. Где указать путь к `06_thread_pool`?
4. Чем отладчик отличается от clangd?
5. Как выполнить `thread apply all bt` из IDE?

## Обсуждение с репетитором

**Том:** 6 · **Глава:** `ch11` · **Файл:** `ch11-ide-debugger.md`

**Дальше по курсу:** [Глава 12. Красивый вывод STL](./ch12-pretty-print.md)
