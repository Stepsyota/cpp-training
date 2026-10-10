# Глава 7. pthread и линковка

## Зачем эта тема

`#include <thread>` **компилируется**, а линковка падает с **`undefined reference to pthread_create`** — классика первой недели multithread. На Linux реализация **`std::thread`** в **libstdc++** опирается на **POSIX threads (pthread)**. Флаг **`-pthread`** (или CMake **`Threads::Threads`**) нужен на **линковке** — и частично на compile — не «для красоты в README».

Ошибка выглядит как «C++ сломан», но compile прошёл: front-end видел объявления из заголовков, а **определения** системных функций подтянулись бы только при правильном link. Эта глава связывает **том 3** (потоки) с **том 5** (флаги).

## Флаг -pthread

```bash
g++ -std=c++17 -Wall -pthread task.cpp test.cpp -o exe
```

**`-pthread`** для GCC/Clang на Linux: макросы для компиляции **и** линковка **libpthread**. Предпочтительнее старого **`-lpthread`** отдельно — порядок библиотек на link иногда ломал legacy Makefile’ы.

Без `-pthread` простой single-file с **`thread`** может **случайно** собраться на некоторых версиях из-за неявных зависимостей — **не рассчитывай**. AGENTS.md и репо явно требуют **`-pthread`**.

## CMake: find_package(Threads)

Фрагмент **`multithread/CMakeLists.txt`**:

```cmake
find_package(Threads REQUIRED)

function(add_exercise name)
    ...
    target_link_libraries(${name} PRIVATE Threads::Threads)
endfunction()
```

И для advanced:

```cmake
target_link_libraries(06_thread_pool PRIVATE Threads::Threads)
```

**`Threads::Threads`** — imported target: CMake подставляет правильные флаги compile/link **для платформы**. На Linux это `-pthread`; на Windows — другой backend (глава 14).

**`REQUIRED`** — configure упадёт, если threads не найдены (редко на Fedora desktop).

## Компиляция vs линковка

Иногда кажется, что pthread нужен «только в конце». Драйвер **`g++`** с **`-pthread`** передаёт флаги **обоим** этапам. Если пишешь Makefile руками — **`-pthread`** в финальной команде link **и** желательно в compile для consistency.

**`.o`** файлы от `task.cpp` не «несут» pthread сами — link executable их соединяет с libpthread.

## async, future, condition_variable

**`std::async`**, **`std::future`**, **`std::condition_variable`** — всё в libstdc++, но создание потоков под капотом те же требования. Отдельного **`-lfuture`** нет.

**`mutex`** в однопоточной программе формально может линковаться без создания потоков; смысл появляется с **`thread`**. Для всего трека **`multithread/`** — **pthread на каждом** executable target.

## Сценарий: только -lpthread

Старые туториалы: **`g++ ... -lpthread`**. На современном GCC **` -pthread`** включает **`-D_REENTRANT`** и согласованный link. Если видишь `-lpthread` в чужом коде — понимай как «нужны потоки», но в новом коде Fedora предпочитай **`-pthread`**.

## Связь с check.sh

Скрипт собирает через CMake — pthread уже на targets. Если ты собираешь **вручную** beginner и забываешь pthread — расхождение «CMake зелёный, g++ одной строкой — нет».

## К собесу

Типичная ошибка без pthread. Что делает **`Threads::Threads`**. Нужен ли pthread для **`03_oracle`**. Почему ошибка на **link**, не compile.

## Типичные заблуждения

«`<thread>` в заголовке — pthread не нужен» — нужен на link. «`-lpthread` и `-pthread` всегда идентичны» — на GCC предпочитай `-pthread`. «CMake сам угадает потоки» — только с **`find_package(Threads)`**.


## Проверка, что pthread в link line

```bash
cmake --build multithread/build --target 03_oracle --verbose 2>&1 | tail -20
```

(Ninja/Make verbose flags vary.) Ищи **`-pthread`** в финальной link command. Если нет — **`Threads::Threads`** не на target.

## Windows и кросс-компиляция (awareness)

**MinGW** на Windows может использовать **`-pthread`** в POSIX-слое; **MSVC** — нет (глава 14). **`multithread/CMakeLists.txt`** не настраивает MinGW — Fedora first.

## std::thread и exceptions

Если **`std::thread`** constructor throws, link всё равно нужен pthread. **`std::terminate`** от **joinable destructor** (том 3) — runtime, не link error.


## pthread and std::async thread pool

**`async`** may use thread pool implementation in libstdc++ — всё равно **Threads** на link for **`03_oracle`**.

## -pthread on compile only mistake

Rare hand Makefile: **`-pthread`** only on compile line for **`.cpp`**, missing on link — link fail. **`g++` one-liner** avoids split.

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


## Объект Threads::Threads в CMake 3.16+

**`Threads::Threads`** — imported target с **`INTERFACE_COMPILE_OPTIONS`** и **`INTERFACE_LINK_LIBRARIES`**. **`nm`** на exe покажет **`pthread_*`** symbols unresolved before link, resolved after.

## std::this_thread и pthread_self

Runtime identity thread — не меняет флаги link. **`-pthread`** всё равно нужен при **`std::thread`** creation from scratch.

## К собесу (дополнение)

`undefined reference to pthread_create`. `Threads::Threads`. `-pthread` vs `-lpthread`.

## Краткое резюме

Репо и AGENTS.md: **`-std=c++17 -pthread`**. CMake: **`find_package(Threads)`** + **`Threads::Threads`** на каждый exe. Ошибка **`pthread_create`** — линковка. Дальше — **TU**, **ODR**, `.o`.

## Вопросы для самопроверки

1. Типичное сообщение линковщика без pthread?
2. Что делает **`Threads::Threads`** в CMake на Linux?
3. Нужен ли pthread для **`03_oracle`**?
4. **`-pthread`** затрагивает compile, link или оба?
5. Где в **`CMakeLists.txt`** Threads подключён к **`05_highscore`**?

## Обсуждение с репетитором

**Том:** 5 · **Глава:** `ch07` · **Файл:** `ch07-pthread-linking.md`

**Дальше по курсу:** [Глава 8. Раздельная компиляция](./ch08-separate-compile.md)
