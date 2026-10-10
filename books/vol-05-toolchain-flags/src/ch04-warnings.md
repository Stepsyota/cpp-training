# Глава 4. Предупреждения

## Зачем эта тема

Ошибки компилятора останавливают сборку — с ними всё ясно. **Предупреждения (warnings)** — диагноз того, что **формально** может собраться, но нарушает правила или здравый смысл: неявное сужение типа, неиспользуемая переменная, порядок инициализации, подозрительный `if (x = 1)`. В многопоточном коде предупреждение про захват лямбды по ссылке **`[&]`** может стоять рядом с будущей гонкой или use-after-free — компилятор не заменит TSan, но иногда подсветит footgun раньше runtime.

Репозиторий `multithread/` включает для GCC и Clang три флага сразу — это **контракт качества** трека, не «опция для перфекционистов»:

```cmake
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()
```

Привыкай к этому набору с первого упражнения `01_rocket_launch`. Однострочник в AGENTS.md показывает только **`-Wall`** — для честности с CMake добавляй **`-Wextra -Wpedantic`** (глава 13).

## -Wall

Название исторически misleading: это **не все** возможные предупреждения, а **разумный набор** частых багов. Сюда входят многие случаи неиспользуемых переменных (с оговорками), подозрительные конструкции control flow, проблемы с return, часть format-related ситуаций.

**`-Wall`** — baseline. Без него ты дольше живёшь с мёртвым кодом и «компилятор молчит — значит OK». На собеседовании «какие флаги включаешь на C++» — начни с Wall и связи с CI.

## -Wextra

**`-Wextra`** добавляет кейсы, которые не вошли в Wall: сравнение signed и unsigned, пустой параметр в macro-variadic, отсутствующие поля в `switch`, иногда более строгие проверки инициализации. Часть сообщений может показаться шумом на legacy-коде; точечное **`Wno-...`** допустимо **с комментарием «зачем»**, не глобальным «выключил всё».

Пример сценария: цикл `for (int i = 0; i < v.size(); ++i)` — если `i` signed, а `size()` unsigned, `-Wextra` может предупредить. В новом коде лучше `std::size_t` или range-for из C++11.

## -Wpedantic

**`-Wpedantic`** строже к **ISO C++**: отклонения от стандарта, некоторые GNU-расширения в режиме **`c++17`**. Полезно, когда целишься в переносимость Fedora → другой Linux → иногда MSVC (глава 14). Иногда конфликтует с системными заголовками на экзотических комбинациях — в учебном репо редко.

Связка **`-std=c++17`** + **`-Wpedantic`** + **`CMAKE_CXX_EXTENSIONS OFF`** — согласованная «мы пишем стандартный C++17».

## -Werror

**`-Werror`** превращает любое warning в **error**. CI любит это, чтобы предупреждения не копились годами. Локально на этапе обучения иногда мешает — можно не включать глобально в личном `build/`, но **не привыкай** откладывать жёлтые строки «на потом».

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic -Werror -c file.cpp
```

Если `-Werror` ломает сборку из-за warning в **чужом** заголовке — проблема архитектуры includes (`-isystem`, глава 12), а не «Werror плохой».

## Чтение длинных сообщений про шаблоны

Длинный простыни-текст с **`candidate template ignored`** — часто **не новый вид warning**, а **failed overload resolution** или SFINAE. Читай **снизу вверх**: что вызвали, какие кандидаты отвалились и почему. Clang часто даёт **note** с «instantiation requested here» — переходи к **первому** месту в **твоём** коде.

Том 1 про dependent names; том 5 про то, что **Wall не укоротит** template error — нужна практика чтения.

## Clang vs GCC

Один исходник может **warn only on Clang** (optional `-Wthread-safety` с annotations) или only on GCC. Серьёзные проекты гоняют оба. Для курса минимум — **GCC как в репо**; если установлен Clang:

```bash
CXX=clang++ cmake -S multithread -B build-clang
```

Сравни diagnostics на том же `task.cpp` — полезное упражнение без смены логики.

## Предупреждения и многопоточность

**`-Wall`** не видит **data race** на обычном `int`. **TSan** (том 7) — другой инструмент. Но:

- unused result `lock()` без сохранения — иногда ловят analyzer/warning siblings;
- lambda `[&]` capturing locals, passed to `std::thread` — здравый смысл + иногда `-Wextra` на lifetime-adjacent patterns.

Не думай «warnings clean ⇒ thread-safe». Думай «warnings clean ⇒ меньше глупых багов **в одном потоке**».

## Подавление: когда можно, когда нет

**`#pragma GCC diagnostic ignored`** или **`[[maybe_unused]]`** — точечно. Глобальное `-w` (отключить **все** warnings) — почти никогда. В репозитории хозяина не меняй CMake на `-w` без запроса.

**`[[nodiscard]]`** на `std::lock_guard` / futures — API-level; если игнорируешь return, `-Wall` может помочь на обёртках.

## Связь с Release и Debug

В `multithread/CMakeLists.txt` **`add_compile_options`** глобален — **Wall на Debug и Release**. Миф «на релизе warnings не нужны» — путь к shipping signed/unsigned багов, которые **`-O2`** проявит иначе, чем `-O0`.

## К собесу

Назвать три флага из репо. Объяснить `-Werror` для CI. Сказать, что warnings **не заменяют** sanitizer. Упомянуть `/W4` на MSVC как аналог «строгого» набора.

## Типичные заблуждения

«Нет errors — код OK» — warnings matter. «Pedantic только для академиков» — для **`c++17`** без gnu extensions. «Отключу Wall на одном файле через `-w` в Makefile» — лучше починить код. «Template error = нужен другой `-std`» — чаще нужен другой **тип** в вызове.


## Примеры предупреждений, полезных в multithread

**Unused variable** после рефакторинга **`cv`** — убери или **`[[maybe_unused]]`**, не **` -w`**. **Sign compare** в цикле по индексу и **`vector::size()`** — исправь тип индекса.

**`-Wshadow`** (иногда в **`-Wextra`**) — inner scope shadow outer; в nested lambda для thread pool легко запутаться в именах **`job`**, **`stopped`**. Переименование дешевле, чем отключить warning.

Предупреждение о **deprecated** **`std::result_of`** (если когда-нибудь встретишь в старом коде) — в C++17 есть **`std::invoke_result`**; репо на 17, но книги по async (том 4) могут упоминать миграцию.

## Wall на CI и локально

Если когда-нибудь **`cicd/`** добавит **`-Werror`**, локальная привычка **не игнорировать warnings** спасёт от красного pipeline. Пока в **`CMakeLists.txt`** репо **нет** **`-Werror`** — это сознательно мягче для обучения; ты всё равно **читай** вывод **`cmake --build`**.

## Диалог с репетитором по warning

Хороший формат: «вот полный текст warning, вот строка, вот моя гипотеза». Плохой: «компилятор ругается, что делать» без текста. Том 5 учит **читать toolchain output** — переноси навык в чат **`ch04-warnings.md`**.


## -Wno-* точечно

```bash
g++ -Wno-unused-parameter -c legacy.cpp
```

Только если parameter required by API. В **`task.cpp`** multithread — **лучше исправить код**, не отключать warning глобально в CMake.

## Format warnings and iostream

**`-Wformat`** family — больше про **`printf`**. На **`iostream`** реже; но **`std::cout << ptr`** без **`void*`** cast — старые warnings на некоторых версиях.

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


## -Wall and third-party headers

If you add external lib later, **`‑isystem`** for its include — chapter 12. **`multithread`** has no third-party headers beyond STL.

## Treat warnings as debt

Each ignored warning — ticket for future you. Repetitor chat: paste warning text + file line — faster than «compiler broken».

## К собесу (дополнение)

Три флага warnings в `CMakeLists.txt`. Зачем `-Werror` в CI. Wall не заменяет TSan.

## Краткое резюме

**`-Wall -Wextra -Wpedantic`** — база репо в `CMakeLists.txt`. **`-Werror`** — для CI и дисциплины. Читай текст предупреждения; не глуши без причины. Дальше — **`-g`**, Debug и подготовка к GDB (том 6).

## Вопросы для самопроверки

1. Где в `multithread/CMakeLists.txt` заданы предупреждения и для каких компиляторов?
2. Чем **`-Wextra`** дополняет **`-Wall`** — приведи класс примера?
3. Когда уместен **`-Werror`** локально vs в CI?
4. Заменяет ли **`-Wall`** Thread Sanitizer?
5. Как пересобрать трек тем же CMake с **`clang++`**?

## Обсуждение с репетитором

**Том:** 5 · **Глава:** `ch04` · **Файл:** `ch04-warnings.md`

**Дальше по курсу:** [Глава 5. Отладочная сборка](./ch05-debug-symbols.md)
