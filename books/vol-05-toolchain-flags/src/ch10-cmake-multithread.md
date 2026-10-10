# Глава 10. CMake трека multithread

## Зачем эта тема

`AGENTS.md` даёт **`cmake -S . -B build`** и **`cmake --build build --target 06_thread_pool`** — без понимания **targets**, **`add_executable`** и функции **`add_exercise`** легко собрать не то упражнение, забыть **Threads** или wonder, почему **`08_parallel_map`** не видит заголовок. Эта глава — **построчный разбор** реального **`multithread/CMakeLists.txt`**: тот же файл, что лежит в репозитории `training`, без выдуманных сокращений.

CMake **не компилирует** сам: он генерирует Ninja/Makefile и вызывает **`g++`** с флагами, которые мы разобрали в главах 3–7. После этой главы строки **`set(CMAKE_CXX_STANDARD 17)`** и **`target_link_libraries(... Threads::Threads)`** должны быть такими же знакомыми, как **`-std=c++17 -pthread`** в однострочнике.

## Файл целиком — карта

Ниже — структура **`multithread/CMakeLists.txt`** (48 строк в репозитории на момент написания тома). Читай сверху вниз: версия CMake → project → стандарт → warnings → Threads → beginner function → пять вызовов → три advanced targets.

```cmake
cmake_minimum_required(VERSION 3.16)
project(multithread_training LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(Threads REQUIRED)
```

Блок **`cmake_minimum_required(VERSION 3.16)`** — CMake **3.16+**, как в AGENTS.md. Старее — configure может упасть на синтаксисе или поведении **`target_*`**.

**`project(multithread_training LANGUAGES CXX)`** — имя проекта и **только C++** (без C). Задаёт переменные **`CMAKE_CXX_COMPILER`**, default flags.

Три **`set`** для **C++17** — дублируют **`-std=c++17`** и **`gnu++` off** (глава 3).

**`if(GNU|Clang)`** + **`add_compile_options`** — глобальные **Wall** (глава 4). MSVC здесь не ветвится — репо Linux-first.

**`find_package(Threads REQUIRED)`** — один раз на проект; дальше **`Threads::Threads`** (глава 7).

## Функция add_exercise

```cmake
function(add_exercise name)
    add_executable(${name}
        beginner/${name}/task.cpp
        beginner/${name}/test.cpp
    )
    target_include_directories(${name} PRIVATE beginner/${name})
    target_link_libraries(${name} PRIVATE Threads::Threads)
endfunction()
```

**`function(add_exercise name)`** — макрос-подобная функция: параметр **`name`** подставляется в пути. **`add_executable(${name} ...)`** создаёт **target** с именем папки, например **`01_rocket_launch`**.

Источники:

- **`beginner/${name}/task.cpp`** — код **ученика** (README «ты пишешь»);
- **`beginner/${name}/test.cpp`** — тесты и **`main`**, **не трогать** без README.

**`target_include_directories(... PRIVATE beginner/${name})`** — **`#include "..."`** из той же папки без **`-I`** вручную. **PRIVATE** — includes **не** транзитивны потребителям (exe никого не consume).

**`target_link_libraries(... PRIVATE Threads::Threads)`** — pthread на link **этого** exe.

Вызовы:

```cmake
add_exercise(01_rocket_launch)
add_exercise(02_ticket_office)
add_exercise(03_oracle)
add_exercise(04_elevator)
add_exercise(05_highscore)
```

Пять targets — пятёрка beginner. Добавить упражнение = новая папка + одна строка **`add_exercise`** — паттерн расширения без копипасты четырёх строк.

## Target 06_thread_pool

```cmake
add_executable(06_thread_pool
    advanced/01_thread_pool/thread_pool.cpp
    advanced/01_thread_pool/smithy.cpp
    advanced/01_thread_pool/test.cpp
)
target_include_directories(06_thread_pool PRIVATE advanced/01_thread_pool)
target_link_libraries(06_thread_pool PRIVATE Threads::Threads)
```

Три **`.cpp`** → три **`.o`** (глава 8). Имя target **`06_thread_pool`**, папка **`advanced/01_thread_pool/`** — номер **06** в имени бинарника для **`check.sh`**.

Ты писал **`thread_pool.hpp`** + **`thread_pool.cpp`**. **`smithy.cpp`** — готовый «цех». **`test.cpp`** — спецификация.

Забыть **`thread_pool.cpp`** в списке — **undefined reference** на методы пула при «зелёном» синтаксисе в test.

## Target 07_pipeline

```cmake
add_executable(07_pipeline
    advanced/02_pipeline/factory.cpp
    advanced/02_pipeline/test.cpp
)
target_include_directories(07_pipeline PRIVATE advanced/02_pipeline)
target_link_libraries(07_pipeline PRIVATE Threads::Threads)
```

**`bounded_queue.hpp`** — header-only; **`factory.cpp`** include’ит и использует шаблон. Два TU.

## Target 08_parallel_map

```cmake
add_executable(08_parallel_map
    advanced/03_parallel_map/test.cpp
)
target_include_directories(08_parallel_map PRIVATE advanced/03_parallel_map)
target_link_libraries(08_parallel_map PRIVATE Threads::Threads)
```

Один **`.cpp`**, но шаблон **`parallel_map.hpp`** генерирует код в этом TU. **`target_include_directories`** обязателен для **`#include "parallel_map.hpp"`**.

## Сборка — команды из AGENTS.md

```bash
cd multithread
cmake -S . -B build
cmake --build build --target 06_thread_pool
./build/06_thread_pool
```

**`-S .`** — source с **`CMakeLists.txt`**. **`-B build`** — out-of-tree build (не мусорить объектниками в beginner).

**`--target 06_thread_pool`** — только одна цель; без него — все 8 exe. **`check.sh`** гоняет тесты — **не меняй** без запроса хозяина.

Полная пересборка после смены заголовка, включённого в много TU:

```bash
cmake --build build --clean-first
```

## CMAKE_BUILD_TYPE и флаги

Первый configure:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
```

Single-config generators (Make, Ninja) используют **`CMAKE_BUILD_TYPE`**. Multi-config (Visual Studio) — другая модель; репо на Fedora — **Debug/Release** через отдельные каталоги:

```bash
cmake -S . -B build-debug -DCMAKE_BUILD_TYPE=Debug
cmake -S . -B build-rel -DCMAKE_BUILD_TYPE=Release
```

Не смешивай Release objects и Debug в **одном** `build/` без clean.

## EXPORT_COMPILE_COMMANDS и clangd

```bash
cmake -S . -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
```

Появится **`build/compile_commands.json`** — по одной записи на **`task.cpp`**, **`thread_pool.cpp`**, … с **`-std=c++17`**, **`-I`**, defines (глава 11). Cursor/clangd: **`--compile-commands-dir=build`**.

После правки **`CMakeLists.txt`** — **re-configure**, иначе json устареет.

## Что CMake не делает в этом файле

Нет **`enable_testing()`**, **`add_subdirectory`**, sanitizer options, **`FetchContent`**. Минимализм тренажёра. Том 7 покажет, куда **аккуратно** добавить **`option(ENABLE_TSAN ...)`** без ломания default build:

```cmake
option(ENABLE_TSAN "Thread sanitizer" OFF)
if(ENABLE_TSAN)
  add_compile_options(-fsanitize=thread -g -O1 -fno-omit-frame-pointer)
  add_link_options(-fsanitize=thread)
endif()
```

В репо по умолчанию **OFF** — обычная сборка остаётся быстрой и без TSan runtime.

## Сравнение с одной строкой g++

CMake target **`02_ticket_office`** эквивалентен по смыслу (упрощённо):

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic -pthread \
  -Ibeginner/02_ticket_office \
  beginner/02_ticket_office/task.cpp \
  beginner/02_ticket_office/test.cpp \
  -o 02_ticket_office
```

Разница: CMake кэширует **`.o`**, фиксирует flags для всех, пишет json для IDE. **`check.sh`** и репетитор ожидают **CMake** path.

## Типичные ошибки configure/build

**«CMake 3.16 not found»** — **`dnf install cmake`**. **«Could NOT find Threads»** — редко на desktop; проверь toolchain. **«No rule to make target 09_...»** — такого target **нет** (только 01–08). **Compiler changed** — удали **`build/`** и configure заново.

## К собесу

Объяснить **`add_executable`**, **`PRIVATE`**, зачем **`find_package(Threads)`**. Сколько **`add_exercise`** в репо. Как собрать **только** **`04_elevator`**.


## Построчный walkthrough: порядок выполнения CMake

При **`cmake -S . -B build`** интерпретатор читает **`CMakeLists.txt`** сверху вниз: **`cmake_minimum_required`** → **`project`** (инициализация компилятора) → **`set` standard** → **`add_compile_options`** → **`find_package(Threads)`** → **`function(add_exercise)`** (только **определение**, вызовов ещё нет) → **`add_exercise(...)`** ×5 (создаются targets) → **`add_executable(06...)`** и т.д.

Каждый **`add_executable`** регистрирует target в build graph. **`cmake --build`** потом вызывает правила: для каждого **`*.cpp`** правило compile, затем link executable.

## generator expressions (не в репо, но рядом)

В больших проектах пишут **`target_compile_options(... $<$<CONFIG:Debug>:-O0>)`**. В **`multithread/CMakeLists.txt`** конфигурация через **`CMAKE_BUILD_TYPE`** — проще. Если увидишь **`$<...>`** в чужом коде — это «флаг зависит от Debug/Release», не магия preprocessor.

## Если добавить ninth exercise (гипотетически)

Потребовался бы новый каталог **`beginner/09_.../`**, строка **`add_exercise(09_...)`**, и правка **`check.sh`** — **не делай** без запроса хозяина (см. **`AGENTS.md`**). CMake-паттерн уже готов: одна функция, один вызов.

## Связь полей CMakeLists с главами 1–9

| Строки CMake | Глава |
|--------------|-------|
| **`cmake_minimum_required`** | 10, AGENTS |
| **`CMAKE_CXX_STANDARD*`** | 3 |
| **`add_compile_options -Wall`** | 4 |
| **`find_package(Threads)`** | 7 |
| **`add_executable` / several cpp** | 1, 8 |
| **`target_link_libraries Threads`** | 7, 9 (dynamic lib) |

Перечитывая **`multithread/CMakeLists.txt`** на Kindle, открывай файл в репо параллельно — номера строк могут сдвинуться, **структура** должна совпадать.

## ninja -t commands

С **`ninja`** backend: **`ninja -C build -t commands 06_thread_pool`** показывает compile/link lines — удобнее спорить «какие флаги реально», чем гадать по cache.


## install() и export — не в репо

**`install(TARGETS ...)`** — packaging; **`multithread`** runs from **`build/`**. Если добавишь **`install`**, **`CMAKE_INSTALL_PREFIX`** — отдельная тема DevOps.

## ccache и sccache

Compiler cache ускоряет rebuild CI; локально **`export CXX="ccache g++"`** — optional. Не меняет flags из **`CMakeLists.txt`**.

## Полный текст CMakeLists для сверки

Открой **`/home/stepsyota/training/multithread/CMakeLists.txt`** (или относительный **`multithread/CMakeLists.txt`** от корня репо). Сверь: **48 строк**, **8 targets**, **`add_exercise`** ×5. Любое расхождение в форке — обнови Kindle-заметку или **`META.md`** тома.

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


## Реплика CMakeLists.txt (справочный фрагмент)

Ниже — содержимое **`multithread/CMakeLists.txt`** репозитория `training` для offline-чтения. Если файл в git изменился, верный источник — git, не EPUB.

```cmake
cmake_minimum_required(VERSION 3.16)
project(multithread_training LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(Threads REQUIRED)

function(add_exercise name)
    add_executable(${name}
        beginner/${name}/task.cpp
        beginner/${name}/test.cpp
    )
    target_include_directories(${name} PRIVATE beginner/${name})
    target_link_libraries(${name} PRIVATE Threads::Threads)
endfunction()

add_exercise(01_rocket_launch)
add_exercise(02_ticket_office)
add_exercise(03_oracle)
add_exercise(04_elevator)
add_exercise(05_highscore)

add_executable(06_thread_pool
    advanced/01_thread_pool/thread_pool.cpp
    advanced/01_thread_pool/smithy.cpp
    advanced/01_thread_pool/test.cpp
)
target_include_directories(06_thread_pool PRIVATE advanced/01_thread_pool)
target_link_libraries(06_thread_pool PRIVATE Threads::Threads)

add_executable(07_pipeline
    advanced/02_pipeline/factory.cpp
    advanced/02_pipeline/test.cpp
)
target_include_directories(07_pipeline PRIVATE advanced/02_pipeline)
target_link_libraries(07_pipeline PRIVATE Threads::Threads)

add_executable(08_parallel_map
    advanced/03_parallel_map/test.cpp
)
target_include_directories(08_parallel_map PRIVATE advanced/03_parallel_map)
target_link_libraries(08_parallel_map PRIVATE Threads::Threads)
```

Перечитай **`function(add_exercise)`**: параметр **`name`** использован **четыре** раза — в имени target, в двух путях **`task`/`test`**, в **`include`**. Это DRY для beginner. Advanced не вписывается в функцию, потому что **разные** наборы исходников — копировать **`target_link_libraries(... Threads::Threads)`** три раза осознанно.

## target_sources и OBJECT libraries (за horizon)

CMake позволяет **`target_sources(06_thread_pool PRIVATE file.cpp)`** добавлять файлы после **`add_executable`**. **`OBJECT` libraries** — промежуточные **`$<TARGET_OBJECTS:...>`**. Репо проще — все sources в **`add_executable`**. Узнаешь паттерн в enterprise CMake, но **`multithread/CMakeLists.txt`** — учебный минимализм.

## Мини-кейс для самостоятельной проверки toolchain

Возьми **`multithread/beginner/02_ticket_office/`** (или любую сданную задачу). Собери через CMake target **`02_ticket_office`**. Затем собери одной строкой **`g++`** из главы 13 с **`-Wextra -Wpedantic`**. Оба run должны быть зелёными. Если расходятся — выпиши diff флагов из **`compile_commands.json`** и ручной команды. Повтори для **`06_thread_pool`** только через CMake (ручная строка длиннее). Запиши один вывод **`ldd`** на любой binary. Это занимает 15 минут и закрепляет том лучше, чем перечитывание без terminal.

## К собесу (дополнение)

Разбор `add_exercise`. Сколько targets. Зачем `PRIVATE`. Полный список sources `06`.

## Краткое резюме

**`multithread/CMakeLists.txt`**: **C++17**, **Wall**, **Threads**, **`add_exercise`** для 01–05, ручные targets для 06–08. Out-of-tree **`build/`**. **`EXPORT_COMPILE_COMMANDS`** для clangd. Sanitizer — опционально, не в базовом файле. Дальше — **IDE** и json.

## Вопросы для самопроверки

1. Какие файлы пишет ученик в **`01_rocket_launch`** vs что в **`test.cpp`**?
2. Зачем **`PRIVATE`** у **`target_include_directories`**?
3. Команда собрать только **`04_elevator`**?
4. Где в CMake **pthread** для **`07_pipeline`**?
5. Зачем два каталога **`build-debug`** и **`build-rel`**?

## Обсуждение с репетитором

**Том:** 5 · **Глава:** `ch10` · **Файл:** `ch10-cmake-multithread.md`

**Дальше по курсу:** [Глава 11. clangd и compile_commands](./ch11-clangd.md)
