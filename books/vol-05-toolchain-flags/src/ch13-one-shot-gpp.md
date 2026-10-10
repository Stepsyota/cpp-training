# Глава 13. Одна команда g++

## Зачем эта тема

CMake — для всего трека и **`check.sh`**. Иногда нужно **быстро** проверить snippet из тома 1, воспроизвести упражнение **без** **`build/`**, или отладить **`-g -O0`** в папке задачи. **`AGENTS.md`** даёт шаблон; эта глава разбирает **порядок аргументов**, типичные опечатки и **соответствие** тому, что делает **`multithread/CMakeLists.txt`**.

Цель — не заменить CMake, а **не бояться** одной строки в терминале и **не расходиться** с флагами репо (**C++17**, **Wall+extra+pedantic**, **pthread**).

## Шаблон beginner из AGENTS.md

Из **папки задачи** (не из корня **`multithread`**):

```bash
g++ -std=c++17 -pthread -Wall -o name task.cpp test.cpp && ./name
```

**`-o name`** — имя **бинарника**, не исходника. Частая ошибка: **`-o task.cpp`**. Запуск **`./name`**.

CMake для **`02_ticket_office`** добавляет **`-Wextra -Wpedantic`** и **`-I`** текущей папки (через **`target_include_directories`**). Честная ручная строка:

```bash
g++ -std=c++17 -pthread -Wall -Wextra -Wpedantic \
  -o ticket task.cpp test.cpp && ./ticket
```

## Порядок: файлы и main

**`test.cpp`** содержит **`main`** и вызывает API из **`task.cpp`**. Оба в **одной** link-команде. Третий **`.cpp`** — перечисли все.

**`-c`** только compile — без link, без **`main`** если проверяешь snippet:

```bash
g++ -std=c++17 -Wall -c snippet.cpp
```

## Include без CMake

**`#include "foo.hpp"`** из той же папки — OK. Подкаталог — **`-I.`** или **`-I..`**.

Advanced **`06`** вручную:

```bash
cd multithread
g++ -std=c++17 -pthread -Wall -Wextra -Wpedantic -g -O0 \
  -Iadvanced/01_thread_pool \
  advanced/01_thread_pool/thread_pool.cpp \
  advanced/01_thread_pool/smithy.cpp \
  advanced/01_thread_pool/test.cpp \
  -o /tmp/06_test && /tmp/06_test
```

Для сдачи курса — **CMake**; меньше расхождений с **`check.sh`**.

## Отладка

```bash
g++ -std=c++17 -pthread -g -O0 -Wall -Wextra -Wpedantic \
  -o name task.cpp test.cpp
gdb ./name
```

Том 6 — команды внутри gdb.

## Sanitizer одной строкой (мост к тому 7)

```bash
g++ -std=c++17 -pthread -g -O1 -fsanitize=thread -fno-omit-frame-pointer \
  -Wall -Wextra task.cpp test.cpp -o name && ./name
```

**Compile и link** — оба с **`-fsanitize=thread`**. Clang: **`clang++`** с теми же флагами часто работает на Fedora.

## Очистка

**`rm -f name`** — не коммить exe. **`build/`** в git не нужен.

## Типичные опечатки

| Симптом | Причина |
|---------|---------|
| **`undefined reference to main`** | Забыл **`test.cpp`** |
| **`pthread_create`** | Забыл **`-pthread`** |
| **`no matching function`** | Нет **`-std=c++17`** |
| Перезаписал исходник | **`-o task.cpp`** |

## Связь с CMakeLists.txt

Каждый флаг ручной строки имеет аналог: **`-std`** → **`CMAKE_CXX_STANDARD`**, **pthread** → **`Threads::Threads`**, **`-I`** → **`target_include_directories`**, список **`.cpp`** → **`add_executable`**.

## К собесу

Минимальная строка для beginner. Зачем **`-o`**. Когда **`-c`**.


## Карта: beginner 01–05

Для каждой папки **`beginner/0N_*/`**: **`cd`** туда, **`g++ ... task.cpp test.cpp`**. Имена бинарника **`‑o`** — любые; **`check.sh`** использует **`build/0N_*`** из CMake, не локальный **`name`**.

## Проверка после правки только task.cpp

```bash
g++ -std=c++17 -pthread -Wall -Wextra -Wpedantic -o /tmp/t task.cpp test.cpp && /tmp/t
```

**`/tmp/t`** — не мусорить в git tracked dir.

## Когда одна строка ломает pedagogical rule

**`AGENTS.md`**: репетитор не подменяет **`task.cpp`**. Одна строка **`g++`** — для **проверки** и **отладки**, не чтобы обойти CMake навсегда перед сдачей **`check.sh`**.


## time and /usr/bin/time

**`/usr/bin/time -v ./name`** — max RSS, page faults; pairs with Release **`08`** timing discussion.

## ASan one-liner reminder

Sanitizer flags must be on **link** too — duplicate on **`g++`** one line is automatic; separate compile/link hand steps easy to get wrong.

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

Шаблон AGENTS.md. `-o` бинарник. Когда `-c`. Advanced multi-file command.

## Краткое резюме

Beginner: из папки задачи, **`task.cpp test.cpp`**, **`-std=c++17 -pthread -Wall`**, **`-o`** на exe. Advanced — несколько **`.cpp`** и **`-I`**. Sanitizer — отдельные флаги. Дальше — **MSVC**.

## Вопросы для самопроверки

1. Что означает **`-o name`**?
2. Где **`main`** в beginner?
3. Как syntax-check без link?
4. Минимальные флаги AGENTS vs полные как CMake?
5. Почему для **`06`** удобнее CMake?

## Обсуждение с репетитором

**Том:** 5 · **Глава:** `ch13` · **Файл:** `ch13-one-shot-gpp.md`

**Дальше по курсу:** [Глава 14. Отличия MSVC](./ch14-msvc-differences.md)
