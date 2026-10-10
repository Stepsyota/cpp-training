# Глава 5. Отладочная сборка

## Зачем эта тема

Отладка зависания на `condition_variable` или «странного» порядка в thread pool без символов — угадайка по адресам `0x55...`. Флаг **`-g`** встраивает **отладочную информацию** (на Linux обычно **DWARF**): имена функций, соответствие строк исходника, локальные переменные, иногда inline frames. **Том 6** разберёт команды GDB; здесь — **как собрать**, чтобы отладчик был полезен, и чем Debug отличается от Release в CMake.

Ты уже прошёл гонки и deadlock в теории (том 3). На практике первый шаг — воспроизвести на **`-O0 -g`**, поставить breakpoint на предикат `cv.wait`, посмотреть `busy` и очередь. Без `-g` GDB покажет только mangled symbols из dynamic symbol table — частично, но не то же самое.

## -g и уровни

Типичная ручная строка для упражнения beginner:

```bash
g++ -std=c++17 -g -O0 -pthread -Wall task.cpp test.cpp -o exe
```

**`-g`** — стандартный уровень для GDB. **`-ggdb3`** — чуть богаче для gdb (макросы). **`-g1`** — минимум, реже нужен. **`-gsplit-dwarf`** — отладочные секции отдельно; для курса не обязательно.

**`-O0`** отключает агрессивные оптимизации: строки в gdb совпадают с исходником, переменные не «исчезают» из-за inlining и reorder. На **`-O2`** breakpoint на строке может **не сработать** или переменная **optimized out** — для сложной отладки временно понижай оптимизацию или используй **RelWithDebInfo**.

## Debug vs Release в CMake

```bash
cd multithread
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --target 04_elevator
```

**`Debug`** для GCC/Clang обычно добавляет **`-g`** и **`-O0`**. **`Release`** — **`-O3`** или **`-O2`** и часто **без** `-g` (или символы strip’ят при установке).

Компромисс:

```bash
cmake -S . -B build-rel -DCMAKE_BUILD_TYPE=RelWithDebInfo
```

Быстрее код + символы — профилирование, частичная отладка production-like поведения.

Смотреть флаги в cache:

```bash
cmake -LA build | grep CMAKE_CXX_FLAGS
```

## Не путать с NDEBUG

Макрос **`NDEBUG`** часто определяют в Release через **`-DNDEBUG`**. Он **отключает `assert`**. Программа меняет поведение: assert’ы исчезают — классика «баг только в Release» иногда из-за **логики внутри assert side effect** (антипаттерн) или из-за того, что в Debug ты не доходил до гонки из-за timing.

**`-g`** и **`NDEBUG`** независимы: можно Release с символами (RelWithDebInfo) и с NDEBUG.

## Optimized-out и том 6

На `-O2` GDB говорит **«value optimized out»**. Решения: собрать Debug; **`gcc -Og`** — оптимизации полегче, частично сохраняет отладку; disassemble / examine registers — том 6. Для курса multithread — **Debug** достаточно.

## Санитайзеры и -g

**ASan/TSan/UBSan** (том 7) **требуют** **`-g`** и обычно **`-O1`**, не **`-O0`** и не агрессивный **`-O3`**. Том 5 фиксирует: **отдельная** конфигурация CMake под sanitizer, не смешивай с обычным Release в одном каталоге `build/`.

Пример заготовки (не включена в репо по умолчанию):

```cmake
add_compile_options(-fsanitize=thread -g -O1 -fno-omit-frame-pointer)
add_link_options(-fsanitize=thread)
```

Флаги и на compile, и на link — см. главу 1 про pipeline.

## Размер бинарника и strip

**`-g`** увеличивает размер на диске; на **скорость runtime** почти не влияет (замедляет **`-O0`**, не `-g`). Для дистрибуции **`strip exe`** убирает символы; для учебных бинарников в `build/` не критично.

**`split debug`** — хранить `.debug` отдельно; enterprise packaging, не multithread.

## Связь с multithread и check.sh

`check.sh` собирает через CMake; тип **`CMAKE_BUILD_TYPE`** зависит от того, как ты configure’ил `build/`. Для отладки одной задачи удобна **ручная строка** из AGENTS.md с **`-g -O0`** в папке упражнения — глава 13.

Advanced `06` с несколькими `.cpp` — тот же принцип: все TU с `-g`, один link.

## Сценарий: «gdb не знает переменную»

1. Проверь `file ./exe` — есть ли **with debug_info**?
2. Если собрал Release — пересобери Debug.
3. Если `-O2` — попробуй `-O0` или `-Og`.
4. Убедись, что gdb смотрит **тот же** binary, что только что собрал (не старый `./name` в папке задачи).

## К собесу

Объяснить **`-g` vs `-O0`**. Назвать **`RelWithDebInfo`**. Сказать, почему sanitizer не с **`-O3`**. Упомянуть **`NDEBUG`** и assert.

## Типичные заблуждения

«Release нельзя отлаживать» — можно с sym, но тяжелее. «-g тормозит программу» — почти нет; тормозит **-O0** vs **-O2**. «gdb без -g покажет нормальные C++ имена» — частично из symbols export, не полноценно.


## Debug multithread: что ставить breakpoint

Для **`04_elevator`**: breakpoint на **`wait`** **после** проверки предиката в исходнике (том 3 — lost wakeup). На **Debug** **`‑O0`** условие в gdb совпадает со строкой.

Для **`06_thread_pool`**: смотри **`busy`**, размер очереди, **`stopped`** под mutex — но **не держи** mutex, когда будешь **`join`** в gdb (deadlock с воркером). Том 6 разберёт **process vs threads**; здесь — собери **Debug** правильно.

## file, readelf

```bash
file multithread/build/04_elevator
readelf -S multithread/build/04_elevator | grep debug
```

**`with debug_info`** в **`file`** — быстрый sanity check перед часом gdb.

## Два build directory

Держи **`build-debug`** для отладки и **`build`** для быстрых прогонов — меньше путаницы «почему оптимизировано». Оба должны использовать **тот же** **`CMakeLists.txt`** и **Threads**.


## Separate debug info on Fedora

Packaging иногда ставит **`‑debuginfo` rpm** с **`*.debug`**. Локально **`‑g`** в **`build/`** достаточно. **`debuginfo-install`** — если отлаживаешь **system** libraries.

## core dumps (preview)

**`ulimit -c unlimited`** — core при crash; tom 6. Debug symbols нужны **`gdb ./exe core`**. Release без **`-g`** — core бесполезнее.

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

Пара `-g` и `-O0`. RelWithDebInfo. NDEBUG и assert. Sanitizer + `-O1`.

## Краткое резюме

Отладка multithread: **`-g -O0`** или CMake **Debug**. **RelWithDebInfo** — средний путь. Sanitizer — свои **`-O`/флаги** (том 7). **NDEBUG** меняет assert. Дальше — **`-O2`/Release** и почему баг «только на Release».

## Вопросы для самопроверки

1. Зачем **`-O0`** вместе с gdb на старте?
2. Что даёт **`CMAKE_BUILD_TYPE=Debug`** в типичном GCC toolchain?
3. Как **`NDEBUG`** влияет на **`assert`**?
4. Почему TSan обычно не с **`-O3`**?
5. Что делает **`strip`** с точки зрения GDB?

## Обсуждение с репетитором

**Том:** 5 · **Глава:** `ch05` · **Файл:** `ch05-debug-symbols.md`

**Дальше по курсу:** [Глава 6. Оптимизация](./ch06-optimization.md)
