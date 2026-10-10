# Глава 12. Флаги препроцессора

## Зачем эта тема

**`-DNAME=value`**, **`-UFOO`**, **`-Ipath`** обрабатываются **до** разбора C++ в TU. Они включают ветки **`#ifdef`**, подставляют версии сборки, добавляют каталоги заголовков. CMake пишет те же смыслы в **`compile_commands.json`** как **` -D`** и **`-I`**. Если собралось в cmake, но **не** собралось одной строкой **`g++`** — часто расходятся **`-D`/`-I`**.

Том 1 отделял **keywords** от **макросов**; **`-D`** создаёт **макрос**, не keyword. **`#define class struct`** — зло вне учебных примеров.

## -D и -U

```bash
g++ -std=c++17 -DDEBUG=1 -DPROJECT_VERSION=\"0.1\" -c file.cpp
```

В коде:

```cpp
#if DEBUG
// ...
#endif
```

**`-UFOO`** снимает определение. **`NDEBUG`** в Release — **`-DNDEBUG`** → **`assert`** исчезает (глава 5).

CMake:

```cmake
target_compile_definitions(03_oracle PRIVATE MY_FLAG=1)
```

**PRIVATE** — define только для этого target, не для зависимых (exe ни от кого не depend).

## -I include path

```bash
g++ -I./advanced/01_thread_pool -c smithy.cpp
```

Порядок **`-I`**: **первый** найденный заголовок побеждает. Системные пути компилятор добавляет сам. Конфlict **`version.h`** в проекте vs **`/usr/include`** — классика.

**`target_include_directories(06_thread_pool PRIVATE advanced/01_thread_pool)`** — тот же **`-I`** в json для clangd.

## -isystem

**`-isystem dir`** — «системный» include; warnings в чужих headers приглушены. Для **своего** кода multithread — обычный **`-I`**, не прячь баги.

## -include

**`-include header.h`** — принудительный include в начало **каждого** TU. Legacy precompiled headers; в репо не используется.

## Макросы компилятора

**`__GNUC__`**, **`__clang__`**, **`__x86_64__`**, **`__cplusplus`** — для условной компиляции. Feature-test из **`<version>`** **(C++20)** предпочтительнее для фич, чем одна цифра **`__cplusplus`** (глава 3, 14).

## Сценарий: ручная vs CMake

Ты в **`advanced/01_thread_pool`**, **`g++ smithy.cpp`** без **`-I`** — **`order.hpp` not found**. CMake target добавляет **`-I`**. Fix: скопировать **`-I`** из **`compile_commands.json`** или вызывать g++ из корня multithread с путями как в главе 13.

## Связь с -E

**`g++ -E`** показывает результат **`-D`** и **`#include`** (глава 1). Отладка «откуда **`#if 0`**» — смотри defines в compile command.

## К собесу

Как передать **`DEBUG`**. **`PRIVATE` definitions**. **`-DNDEBUG`**. **`-I` order**.

## Типичные заблуждения

«**`-D`** только Debug» — любой build. «**`-I`** заменяет CMake» — дублируй при ручной сборке. «Макрос = const variable» — нет type checking.


## Defines из CMake для build type

**`NDEBUG`** часто появляется в Release через **`CMAKE_CXX_FLAGS_RELEASE`** — не через **`CMakeLists.txt`** multithread явно, но через **defaults**. Поэтому **`assert`** в test может исчезнуть на Release — помни из главы 5.

## -I и quoted vs bracket includes

**`#include "parallel_map.hpp"`** — ищет relative **`-I`** first. **`#include <vector>`** — system paths. **`target_include_directories`** влияет только на quoted и на some project-specific brackets.

## Feature test macros

**`#include <version>`** и **`__cpp_lib_jthread`** **(C++20)** — предпочтительнее самодельных **`‑D`** для фич. Репо на 17; macros — на будущее чтение tom 1/4.


## -MMD dependency files

**`-MMD`** generates **`.d`** for Make — CMake/Ninja generate deps itself. Seen in raw Make tutorials.

## -macros list

**`g++ -std=c++17 -dM -E -x c++ /dev/null | grep __cplusplus`** — quick macro check without source file.

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


## CMAKE_CXX_FLAGS from environment

**`export CXXFLAGS="-Wall"`** before cmake — merges with project flags; can surprise. Prefer explicit **`CMakeLists.txt`** as source of truth for **`multithread`**.

## Response file @file.rsp

**`cl`** and **`g++ @rsp`** — long command lines; Ninja uses rsp internally. You see **`@`** in verbose build logs.

## К собесу (дополнение)

`-D`, `-I`, `-U`. `target_compile_definitions`. `-isystem`.

## Краткое резюме

**`-D`/`-U`** для **`#if`**; **`-I`** для путей; CMake — **`compile_definitions`**, **`include_directories`**. Согласуй ручную **`g++`** с target. Дальше — **одна команда** AGENTS.

## Вопросы для самопроверки

1. Как передать **`DEBUG=1`** через g++?
2. Зачем **`PRIVATE`** у definitions?
3. Что **` -DNDEBUG`** делает с **`assert`**?
4. Почему порядок двух **`-I`** меняет сборку?
5. Чем **`-isystem`** от **`-I`**?

## Обсуждение с репетитором

**Том:** 5 · **Глава:** `ch12` · **Файл:** `ch12-preprocessor-flags.md`

**Дальше по курсу:** [Глава 13. Одна команда g++](./ch13-one-shot-gpp.md)
