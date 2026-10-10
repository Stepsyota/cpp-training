# Глава 14. Отличия MSVC

## Зачем эта тема

Серия и **`training/multithread/`** ориентированы на **Fedora + GCC/Clang**. Код на работе или в open source часто собирают **MSVC** (**`cl.exe`**, Visual Studio, CMake с генератором **Visual Studio**). Цель главы — **перевести** привычные флаги из глав 1–13, не превратить том в руководство по IDE Microsoft.

Если ты учишься на Fedora, как автор репо, MSVC — **контекст для чтения чужих CI** и собеседований «Windows vs Linux toolchain». Эталон практики курса остаётся **`g++`** и **`multithread/CMakeLists.txt`**.

## Компилятор и стандарт

| GCC/Clang (Linux) | MSVC (приблизительно) |
|-------------------|------------------------|
| **`g++ file.cpp`** | **`cl /EHsc file.cpp`** |
| **`-std=c++17`** | **`/std:c++17`** |
| **`-Wall -Wextra -Wpedantic`** | **`/W4`**, **`/permissive-`** |
| **`-Werror`** | **`/WX`** |
| **`-O2`** | **`/O2`** |
| **`-g`** | **`/Zi`** + **`/DEBUG`** на link |
| **`-DFOO=1`** | **`/DFOO=1`** |
| **`-Ipath`** | **`/Ipath`** |
| **`-c`** | **`/c`** |
| **`-o exe`** | **`/Fe:exe`** |

**`/EHsc`** — модель исключений C++; без неё historical MSVC ломает RAII-ожидания для **`try/catch`**.

## __cplusplus на MSVC

Historically **`cl`** при **`/std:c++17`** сообщал **`__cplusplus < 201703`**, пока не включишь **`/Zc:__cplusplus`**. Код с **`#if __cplusplus >= 201703L`** вёл себя иначе, чем на GCC. Современные версии VS лучше; для кросс-платформенных **`#if`** — тестируй оба toolchain или используй **`<version>`** **(C++20)**.

## pthread и потоки

На Windows **нет `-pthread`** в том же виде: **`std::thread`** в MSVC через **Win32 threads**. CMake **`Threads::Threads`** подставляет правильное **на Windows**. Флаг **`-pthread`** из **`AGENTS.md`** — **Linux-specific**.

Перенос **`multithread/`** на Windows — не только замена флагов: paths, **`check.sh`**, возможно WSL как «эталон курса».

## Линковка и runtime

**`.lib`** — import для **DLL** или static lib; **`.dll`** — runtime. Аналог **`.so`/`.a`** в другой экосистеме. **libstdc++** нет — **MSVC runtime** (**vcruntime**, **msvcp**). Смешивать **`.obj`** от **`g++`** и **`cl`** в одном exe **нельзя** — **разный ABI C++**.

## Предупреждения

MSVC: коды **C4xxx**. **`/permissive-`** — ближе к ISO, аналог spirit **`-Wpedantic`**. Некоторые GNU extensions из книги **не** компилируются на **`cl`**.

Условный CMake (не в текущем репо, но типичный перенос):

```cmake
if(MSVC)
  add_compile_options(/W4 /permissive-)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()
```

Текущий **`multithread/CMakeLists.txt`** ветвит только **GNU|Clang** — для Fedora достаточно.

## Sanitizers

**ASan** на MSVC появился с ограничениями; **TSan** в курсе (**том 7**) — **GCC/Clang на Linux**. На Windows для гонок — VS analyzer, или **WSL/Fedora** как эталон **`training`**.

## Связь с томами 6–7

**GDB** — Linux tom 6; MSVC — **Visual Studio debugger**. Sanitizer workflow из тома 7 предполагает **`g++`** и отдельный **`build-tsan`**.

## К собесу

Эквивалент **`-std=c++17`**. Почему **`-pthread`** не на MSVC. **`/WX`**. Почему нельзя линковать GCC **`.o`** с **`cl`**. Куда смотреть **TSan** после тома 5.

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

`/std:c++17`, `/W4`, `/EHsc`. Нет `-pthread`. Нельзя mix GCC `.o` и `cl`.

## Мифы

«Код из Kindle соберётся в VS без изменений» — чистый **C++17 STL** часто да; platform-specific — нет. «MSVC = плохой C++» — устарело; смотри версию. «Флаги один в один» — таблица выше.


## Таблица перевода для review перед собесом

Пробеги глазами пары: **`-pthread`** ↔ Windows **`Threads::Threads`**. **`-g -O0`** ↔ **`/Zi`** + Debug config. **`-fsanitize=address`** ↔ MSVC ASan (ограниченно). **`-std=c++17`** ↔ **`/std:c++17`**. **`__cplusplus`** ↔ **`/Zc:__cplusplus`**.

Запиши от руки (или в чат репетитору) **пять** пар — память держится лучше, чем только чтение таблицы на Kindle.

## CMake на Visual Studio generator

**`cmake -G "Visual Studio 17 2022" -A x64`** создаёт **`.sln`**, не Makefile. **`cmake --build . --config Debug`** — multi-config. **`multithread/CMakeLists.txt`** на Fedora использует single-config Ninja/Make; перенос на VS — смена **generator**, не только **`cl`**.

## Когда оставаться на WSL/Fedora для курса

Если цель — **закрыть **`multithread/`** и tom 7 TSan**, Windows-native сборка **не обязательна**. WSL2 Fedora/Ubuntu с **`g++`** и **`pthread`** — допустимый «эталон курса» на одном железе с Windows desktop.

## Резюме серии: toolchain → debug → sanitize

Том 5 заканчивает **флаги и CMake**. Том 6 использует **`-g`**. Том 7 добавляет **`-fsanitize=*`** в **отдельный** build tree. Не смешивай три роли в одном **`build/`** без понимания — иначе «sanitizer не ловит» или «gdb не видит строки».


## ClangCL on Windows

**Clang** front-end + **`cl`** compatibility layer — hybrid; flags mix **`/W4`** and **`-std`**. Not **`multithread`** default.

## Subsystem windows vs console

**`/SUBSYSTEM:CONSOLE`** for **`main`** — link option; wrong subsystem — no console output for **`std::cout`**.

## Итоги тома 5

Ты прошёл pipeline **preprocess → compile → link**, Fedora toolchain, **C++17** как в CMake, warnings, Debug/**`-g`**, **`-O`**, **pthread**, TU/ODR, **`.so`**, полный разбор **`multithread/CMakeLists.txt`**, **clangd**, **`-D`/`-I`**, однострочник **`g++`**, мост к **MSVC**. **Том 6** — GDB и пошаговая отладка **`04_elevator`** и **`06_thread_pool`**.

## Вопросы для самопроверки

1. MSVC-аналог **`-std=c++17`**?
2. Чем заменить **`-pthread`** на Windows в CMake?
3. Зачем **`/Zc:__cplusplus`**?
4. Можно ли слинковать **`.o`** от **`g++`** с MSVC?
5. Куда в серии смотреть **TSan**?

## Обсуждение с репетитором

**Том:** 5 · **Глава:** `ch14` · **Файл:** `ch14-msvc-differences.md`

**Конец тома 5.** Дальше по серии: **том 6** — GDB, cgdb, отладка в IDE (`books/vol-06-debugging`, по плану в `BOOK-CONTEXT.md`).
