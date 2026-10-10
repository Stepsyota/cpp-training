# Глава 9. Статика и динамика

## Зачем эта тема

После компиляции **`.o`** линковщик подтягивает **библиотеки**: **`libc`**, **`libstdc++`**, **`libm`**, **`libpthread`**. Они бывают **статическими** (архив **`.a`** — нужные `.o` копируются в exe) и **динамическими** ( **`.so`** — код загружается при старте или через **`dlopen`**). Для учебного репо достаточно **динамической libstdc++** по умолчанию; знать разницу нужно для «почему на другой машине не запускается» и для чтения **`ldd`**.

Ты не обязан собирать static exe в multithread, но на собеседовании «static vs dynamic linking» спрашивают рядом с **`-pthread`** и **`-l`**.

## Динамическая линковка по умолчанию

**`g++`** тянет **`libstdc++.so.6`**. Проверка:

```bash
ldd multithread/build/03_oracle
```

Увидишь **`libstdc++.so.6`**, **`libgcc_s.so`**, **`libc.so.6`**, **`libpthread`**. Если на другой Fedora **нет** совместимой libstdc++ — ошибка **при запуске**, compile был OK.

## Статическая линковка runtime (частичная)

```bash
g++ -static-libstdc++ -static-libgcc app.o -o app
```

Exe **крупнее**, меньше зависимость от версии **`.so`** на целевой системе. Полная **`-static`** (включая glibc) — редко, лицензии и обновления безопасности.

## -l и -L

**`-lfoo`** — линковать **libfoo** (ищет **`libfoo.so`** / **`libfoo.a`**). **`-L/path`** — каталог поиска. **Порядок важен**: объектники, потом библиотеки; циклические зависимости иногда требуют **`-lfoo` дважды**.

**`multithread`** не тянет сторонние **`-l`** кроме pthread через **Threads**. Сторонние libs — будущие проекты / cicd.

## .a как архив .o

**`ar`** собирает **`.a`**. Линковщик вытаскивает **только нужные** member `.o` (обычно). Создание своих static libs — за рамками трека; понимание «`.a` = zip of `.o`» помогает читать ошибки.

## rpath и RUNPATH

Своя **`.so`** не в системном пути:

```bash
g++ ... -Wl,-rpath,'$ORIGIN/lib' -L./lib -lmylib
```

**`$ORIGIN`** — каталог exe. CMake: **`INSTALL_RPATH`**, **`BUILD_RPATH`**. Для курса — awareness.

## nm, readelf (лёгкое знакомство)

**`nm -C exe | c++filt`** — символы в бинарнике. **`readelf -d exe`** — dynamic section, rpath. Том 6 использует символы; том 5 — «они живут после link».

## Связь с undefined reference

**`.so`** на link time — **import stubs**; реализация в `.so` на диске. **`.a`** — код **внутри** exe после link. **undefined reference to `foo`** — ни `.o`, ни `.a`, ни `.so` не дали **`foo`**.

## К собесу

**`ldd`**. **`-static-libstdc++`**. **`.a` vs `.so`**. Порядок **`-l`**. Нужен ли **`-lstdc++`** после **`g++`**.

## Типичные заблуждения

«exe переносится без `.so`» — только static или те же libs. «**`-lstdc++`** всегда явно» — **`g++`** добавляет. «`.so` на link = dlopen» — обычно dynamic linker at startup.


## ldd после обновления Fedora

После **`dnf upgrade`** **`ldd ./build/06_thread_pool`** может показать «not found» для **`.so`** — пересобери exe, не только pull git.

## NEEDED и SONAME

**`readelf -d exe | grep NEEDED`** — какие **`.so`** нужны at startup. Понимание помогает читать ошибки loader'а на другой машине.

## Статический pthread

**`-static`** + threads — special case; на Linux часто **не** нужен для **`training`**. Не усложняй **`CMakeLists.txt`** без запроса.


## LD_LIBRARY_PATH

Dev hack to run exe with local **`.so`** — **`export LD_LIBRARY_PATH=./lib`**. Prefer **rpath** for reproducibility. Don't rely on **`LD_*`** in production.

## dlopen dynamic loading

**`dlopen("libfoo.so")`** — after link of main exe without **`-lfoo`** at build time. Plugin systems — beyond course.

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


## copy vs run on another machine

Copy **`06_thread_pool`** binary to другой Fedora same major — usually works. Copy to Alpine/musl — **not** — different libc. Static linking tradeoffs from chapter 9 apply.

## libstdc++ SONAME

**`libstdc++.so.6`** — SONAME stable across many releases; minor incompatibilities rare but documented in distro release notes.

## К собесу (дополнение)

`ldd`. `.a` vs `.so`. `-static-libstdc++` когда. Порядок `-l`.

## Все восемь targets и одна libstdc++

Каждый exe в **`build/`** — **`01_rocket_launch`** … **`08_parallel_map`** — **динамически** зависит от **`libstdc++.so.6`**. Ты не линкуешь **`‑lboost`** или **`‑lfmt`**; граф зависимостей простой: **libc**, **libm**, **libpthread**, **libstdc++**. Выполни **`ldd build/06_thread_pool`** и **`ldd build/08_parallel_map`** — списки NEEDED почти одинаковы; размер exe разный из-за кода, не из-за другого runtime.

Если когда-нибудь перенесёшь **только** бинарник на USB без исходников — нужна **совместимая** Fedora/glibc/libstdc++ на целевой машине. Для учебы всегда переноси **git clone + cmake**, не «голый exe». Это сознательное отличие от shipping static binaries в embedded.

Пара **`readelf -Ws build/03_oracle | head`** показывает **глобальные** символы после link — mangled **`main`**, **`std::`**, твои функции из **`task.cpp`**. Связь с главой 8: символы из **`test.o`** и **`task.o`** merge в один address space.

## Краткое резюме

По умолчанию dynamic **libstdc++**. **`-l`/`-L`** для сторонних lib. **`ldd`** — диагностика. **rpath** — свои **`.so`**. Дальше — **полный CMake** репо.

## Вопросы для самопроверки

1. Что покажет **`ldd`** на **`03_oracle`**?
2. Зачем **`-static-libstdc++`**?
3. Чем **`.a`** отличается от **`.so`** при link?
4. Почему порядок **`-l`** иногда важен?
5. Нужен ли **`-lstdc++`** после **`g++`**?

## Обсуждение с репетитором

**Том:** 5 · **Глава:** `ch09` · **Файл:** `ch09-static-dynamic.md`

**Дальше по курсу:** [Глава 10. CMake трека multithread](./ch10-cmake-multithread.md)
