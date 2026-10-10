# Глава 11. clangd и compile_commands

## Зачем эта тема

Красные подчёркивания в Cursor при **зелёном** **`cmake --build`** означают: **language server не знает флаги** твоего target. **`AGENTS.md`** упоминает **`compile-commands-dir=build`**. **clangd** читает **`compile_commands.json`** — для каждого **`.cpp`** точная команда с **`-std=c++17`**, **`-I`**, defines — и совпадает с реальной сборкой **`multithread/`**.

Inline-подсказки у хозяина репозитория **отключены** намеренно — clangd **не** подсказывает готовый код в task, но **ошибки типов** и «не найден header» должны быть правдивыми. Ложные ошибки на **`std::thread`** без **`-pthread`** в json — трата времени.

## Генерация compile_commands.json

```bash
cd multithread
cmake -S . -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
```

Файл: **`build/compile_commands.json`**. Массив объектов: **`directory`**, **`file`**, **`command`** или **`arguments`**.

Настройка Cursor/VS Code (пример):

```json
"clangd.arguments": ["--compile-commands-dir=build"]
```

Путь **относительно** того, как открыт workspace: корень **`training`** vs **`training/multithread`**. Если открыт **`multithread`**, **`build`** рядом с **`CMakeLists.txt`**.

Симлинк в корень multithread (опционально):

```bash
ln -sf build/compile_commands.json compile_commands.json
```

clangd ищет json вверх по дереву каталогов.

## Что внутри записи для advanced

Для **`advanced/01_thread_pool/thread_pool.cpp`** команда содержит:

- полный путь к **`g++`** или **`clang++`**;
- **`-std=c++17`** (из **`CMAKE_CXX_STANDARD`**);
- **`-I.../advanced/01_thread_pool`**;
- **`-Wall -Wextra -Wpedantic`**;
- **`-c`** и путь к файлу.

Без записи для **`smithy.cpp`** clangd **не** знает include path — «file not found» на **`order.hpp`**, хотя CMake собирает.

## После смены CMakeLists.txt

Добавил **`add_exercise`**, новый **`.cpp`**, сменил **`target_include_directories`** — **re-run configure**:

```bash
cmake -S . -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
```

Restart clangd (Command Palette → **clangd: Restart**) если подчёркивания залипли.

## clangd vs сборка g++

clangd использует **Clang** frontend для анализа, даже если **`CMAKE_CXX_COMPILER`** = **`g++`**. Для C++17 multithread расхождения **редки**; template diagnostics иногда **лучше** в clangd.

Если **только clangd** ругается — проверь json. Если **только g++** — верь компилятору.

## compile_flags.txt fallback

Без CMake в корне можно **`compile_flags.txt`**:

```
-std=c++17
-pthread
-Wall
-Ibeginner/03_oracle
```

**Один** набор на всё дерево — грубо. Для **`06`** с тремя TU и разными **`-I`** — **недостаточно**. Multithread — **только compile_commands**.

## Файл .clangd

В **`multithread/.clangd`** (если создашь):

```yaml
CompileFlags:
  Add: [-Wno-unknown-warning-option]
```

Только при необходимости; не маскируй реальные ошибки в **`task.cpp`**.

## Связь с главой 10

**`CMAKE_EXPORT_COMPILE_COMMANDS=ON`** — не default в старых привычках; включай явно на dev-машине. CI может не генерировать json — локальная IDE задача.

## Связь с томом 6

GDB смотрит **binary** с **`-g`**; clangd — **исходники** с теми же **include** paths. Оба согласованы через **CMake target**.

## Сценарий: «thread не member of std»

1. **`cmake --build`** — OK?
2. Есть ли **`task.cpp`** в **`compile_commands.json`**?
3. Есть **`-std=c++17`** в command?
4. Открыт ли файл **вне** workspace root?

## К собесу

Зачем **compile_commands**. Что делать после правки CMake. Разница **compile_flags.txt** vs json.

## Типичные заблуждения

«IDE error = не соберётся» — проверь build. «json в git» — обычно **gitignore**, генерируй локально. «clangd компилирует проект» — только analysis.


## .vscode/settings.json в репо

В **`training/.vscode/settings.json`** (если есть) может быть **`compile-commands-dir`**. Не коммить секреты; путь **`build`** относительно **`multithread`**. Если workspace root — **`training`**, путь может быть **`multithread/build`**.

## FallbackCompileFlags

Если clangd не находит json, он может использовать defaults **без** **`‑pthread`** — симптом: **`thread`** «unknown type» только в IDE. Fix json, не **`// NOLINT`** на всём файле.

## Background index

Первое открытие большого **`thread_pool.hpp`** — clangd индексирует; подожди, не паникуй от временных squiggles.


## Query-driver

If system headers wrong, **`--query-driver=/usr/bin/g++`** in clangd args — advanced. Fedora default usually works.

## Inactive regions

**`#if 0`** code grayed out — clangd may skip; not a build skip if accidentally **`#if 1`** wrong macro.

## Углубление: связь с пройденным курсом multithread

Ты уже реализовал **`01_rocket_launch`** — там важно было **`join`**, не pipeline. С точки зрения toolchain это один **`task.cpp`**, один **`test.cpp`**, один link с **`-pthread`**. **`02_ticket_office`** — тот же каркас; mutex не меняет флаги, но если забудешь **`-std=c++17`**, **`std::mutex`** всё равно есть (C++11), зато другие привычки (bindings, optional в других файлах) поплывут.

**`03_oracle`** — **`async`**; link как у thread. **`04_elevator`** — первый раз, когда Debug с **`-g`** реально нужен для lost wakeup: без символов **`wait`** в gdb — адрес. **`05_highscore`** — atomics; Release **`-O2`** не заменяет CAS, но UB без atomics проявится иначе на Release.

**`06_thread_pool`** — первый target с **тремя** `.cpp` в **`CMakeLists.txt`**. Ошибка «не тот список файлов» — глава 8 и 10. **`07_pipeline`** — два `.cpp` и header-only queue. **`08_parallel_map`** — один test TU, шаблон в hpp; **`Threads`** всё равно на link, потому что **`parallel_map`** создаёт **`std::thread`**.

Когда перечитываешь том 5 после сдачи advanced, отметь галочкой: «эта глава объясняет мой симptom N». Так книга становится справочником, не только линейным чтением.

## Углубление: чтение вывода cmake --build

Первые строки compile — **`Building CXX object`**, имя **`.o`**, путь к **`.cpp`**. Link — **`Linking CXX executable`**, имя **`06_thread_pool`**. Если compile прошёл, а link упал — scroll к **link** stanza, не к первому warning compile.

Warning в **`test.cpp`**, который «не трогаешь» — всё равно **твой** build; CI не разделит «виноват test». Репетитор может сказать «игнорируй test», но toolchain всё равно показывает warning — решай, нужен ли **`‑Wno`** или fix в **`task`**.

## Углубление: дисциплина build directory

Один **`build/`** на ветку git — норма. Два clone репо — два **`build/`**. Не копируй **`build/`** между машинами. **`compile_commands.json`** привязан к absolute paths на машине — на другом ноутбуке regenerate.

## Мини-кейс для самостоятельной проверки toolchain

Возьми **`multithread/beginner/02_ticket_office/`** (или любую сданную задачу). Собери через CMake target **`02_ticket_office`**. Затем собери одной строкой **`g++`** из главы 13 с **`-Wextra -Wpedantic`**. Оба run должны быть зелёными. Если расходятся — выпиши diff флагов из **`compile_commands.json`** и ручной команды. Повтори для **`06_thread_pool`** только через CMake (ручная строка длиннее). Запиши один вывод **`ldd`** на любой binary. Это занимает 15 минут и закрепляет том лучше, чем перечитывание без terminal.

## К собесу (дополнение)

`CMAKE_EXPORT_COMPILE_COMMANDS`. Почему IDE расходится с build. `compile_flags.txt` limits.

## Краткое резюме

**`CMAKE_EXPORT_COMPILE_COMMANDS=ON`**, **`--compile-commands-dir=build`**. Обновляй после CMake. Advanced **06–08** без json — ложные ошибки. Дальше — **`-D`/`-I`**.

## Вопросы для самопроверки

1. Где лежит **`compile_commands.json`** после configure?
2. Почему clangd не видит **`thread_pool.hpp`** paths?
3. Что делать после правки **`CMakeLists.txt`**?
4. Чем **`compile_flags.txt`** хуже для **`06_thread_pool`**?
5. Какая настройка clangd упоминается в **`AGENTS.md`**?

## Обсуждение с репетитором

**Том:** 5 · **Глава:** `ch11` · **Файл:** `ch11-clangd.md`

**Дальше по курсу:** [Глава 12. Флаги препроцессора](./ch12-preprocessor-flags.md)
