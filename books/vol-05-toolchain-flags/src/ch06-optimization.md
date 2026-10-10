# Глава 6. Оптимизация

## Зачем эта тема

**`-O2`** в Release меняет порядок инструкций, убирает «лишние» чтения памяти, встраивает функции (**inline**), vectorize циклы. Многопоточный код при этом остаётся корректным **только если** нет **undefined behavior** и **data race** — компилятор **не** обязан сохранять «как ты нарисовал в голове» при гонке на обычном `int`. Понимание уровней **`-O`** объясняет «на Debug всё ок, на Release падает» и почему TSan советуют с **`-O1`**, не с **`-O0`**.

Ты уже знаешь из тома 3: **mutex** и **atomics** с правильным memory order — барьеры, которые оптимизатор **обязан** уважать. Это не повод отключать **`-O2`** в production; повод — **исправить синхронизацию**. Отключение оптимизаций «чтобы гонка исчезла» — маскировка UB.

## Уровни -O

**`-O0`** — без оптимизаций, быстрая сборка, отладка (глава 5). **`-O1`** — лёгкие оптимизации; часто baseline для sanitizers. **`-O2`** — рабочий Release во многих проектах; разумный баланс. **`-O3`** — агрессивнее (vectorization, inline expansion); иногда **больше** кода и регрессии скорости из-за cache. **`-Os`** — уменьшение размера; embedded.

CMake **`Release`** на GCC часто **`-O3`** или **`-O2`** — смотри **`CMAKE_CXX_FLAGS_RELEASE`**:

```bash
cmake -LA build | grep CXX_FLAGS_RELEASE
```

**`-Og`** (GCC) — «оптимизации для отладки»; компромисс между `-O0` и `-O2`.

## Компилятор и память в legal multithreaded code

При **легальных** **`std::mutex`** и **`std::atomic`** с корректным ordering оптимизатор не может «выкинуть» lock или «прочитать int один раз в регистр навсегда» там, где стандарт запрещает. При **гонке** на non-atomic — **anything goes** (UB). Отсюда миф «`-O2` сломал мой счётчик без mutex» — сломал **UB**, Release просто проявил.

## Link Time Optimization (LTO)

**`-flto`** (GCC/Clang): оптимизация **на этапе линковки** через все `.o` — cross-TU inline, dead code. Дольше сборка, иногда быстрее бинарник. Требует LTO на **всех** единицах и совместимого linker (**lld**, **gold**). В учебном репо не обязательно; термин для **R+P+I** и собеса.

```bash
g++ -std=c++17 -O2 -flto f1.cpp f2.cpp -o app
```

Если один `.o` без LTO, а остальные с — боль и странные ошибки.

## Profile Guided Optimization (PGO)

Сначала сборка с **instrumentation**, прогон типичных входов, пересборка с профилем. Мощно для серверов и hot paths. Для **`08_parallel_map`** в тренажёре — избыточно; упоминание для полноты картины «как делают в индустрии».

## -ffast-math и соседи

Флаги, ломающие семантику IEEE для **`float`/`double`**, — **не** включай случайно. К multithread слабо относятся, но «результат только на Release» иногда из **reassociation** с `-ffast-math`, не из потоков.

## Измерение parallel_map

`std::chrono` вокруг **`parallel_map`** vs последовательный вызов на **Release** осмысленнее спора «`-O3` vs `-O2`». Ryzen 5500: 6 ядер / 12 потоков SMT — hyperthreading и размер данных влияют сильнее одного флага. Тесты **`08`** делят размер нацело — зелёный тест не доказывает правильность нарезки кусков (см. AGENTS.md).

## Debug performance vs correctness

**`-O0`** может **скрыть** race из-за другого timing. TSan + **`-O1`** — другой режим. Не путай «медленно на Debug» с «корректно на Debug».

## К собесу

Почему баг «только на Release» при UB. Что даёт **`-flto`**. Влияет ли **`-O2`** на **корректный** mutex — нет, на **гонку** — да (UB). Какой `-O` у sanitizer.

## Типичные заблуждения

«`-O0` = как в production» — нет. «LTO всегда быстрее» — не всегда, CI дороже. «Оптимизатор чинит гонки» — нет. «Release = `-O3` всегда» — смотри CMake flags.


## CMAKE_BUILD_TYPE и 08_parallel_map

Прогон **`08_parallel_map`** на **Release** имеет смысл для **времени**, не для **корректности** — тесты функциональные. Если Release падает, а Debug нет — ищи UB/race, не «‑O2 виноват сам по себе».

```bash
cmake -S multithread -B build-rel -DCMAKE_BUILD_TYPE=Release
cmake --build build-rel --target 08_parallel_map
```

Сравни wall time с Debug — порядок величин важнее точных процентов на домашнем Ryzen.

## -march=native и переносимость

**`-march=native`** (не в репо) — оптимизация под **текущий** CPU. Exe может не запуститься на старом CPU в CI. Учебный **`multithread`** не использует — знай термин, не включай без причины.

## Inline и hot path в thread pool

Компилятор может inline маленькие функции в **`submit`** на **‑O2**. Это **не** меняет требования к mutex. Если профилируешь — **RelWithDebInfo** + **`perf record`** — за пределами тома, но логичное продолжение после **`‑g`**.


## -fvisibility=hidden

На **`.so`** скрывает symbols по умолчанию; на exe **`multithread`** не актуально. Знай имя для чтения CMake сторонних libs.

## Benchmark hygiene

Warmup iterations, fixed **`n_threads`**, не сравнивай Debug sec с Release sec в одном графике без подписи **build type**.

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


## Разбор флагов в CMakeCache для Release

Выполни после configure **`build-rel`**:

```bash
cmake -LA build-rel 2>/dev/null | egrep 'CMAKE_CXX_FLAGS|BUILD_TYPE'
```

Сравни **`CMAKE_CXX_FLAGS_DEBUG`** и **`CMAKE_CXX_FLAGS_RELEASE`**. Увидишь **`-g`** только в Debug (обычно) и **`-O0`** vs **`-O3`**. Это объясняет, почему **`check.sh`** на default **`build/`** без **`BUILD_TYPE`** может быть neither pure Debug nor Release — зависит от первого configure. Явно задай **`Debug`** или **`Release`** при создании каталога.

## Atomics и compiler fences

**`std::atomic`** operations compile to specific instructions + fences on x86_64. **`-O2`** may elide redundant **loads** only where memory model allows. Том 3 — happens-before; том 5 — не blame **`-O2`** for missing **mutex**.

## К собесу (дополнение)

Уровни `-O`. LTO в двух словах. Почему Release exposes UB races.

## Закрепление на 05_highscore и 08_parallel_map

**`05_highscore`** с **`std::atomic`** и CAS — корректность не зависит от **`-O2`**, если нет UB и используешь atomics по назначению. Собери **`05_highscore`** в Debug и Release; результат теста должен совпасть. Если diverge — ищи data race или неatomic путь, не «отключи оптимизацию навсегда».

**`08_parallel_map`** — измеряй wall time только на **Release** с **`CMAKE_BUILD_TYPE=Release`**, иначе сравниваешь в основном overhead **`‑O0`**. Число потоков **`min(n_threads, size)`** из твоей реализации влияет на speedup сильнее, чем спор **`-O2` vs `-O3`**. Запиши одну таблицу «Debug sec / Release sec / threads» в блокнот — пригодится репетитору без доступа к твоей машине.

## Краткое резюме

Debug: **`-O0`**; Release: **`-O2`/`-O3`**. Sanitizer: часто **`-O1`**. LTO/PGO — продвинутый уровень. Гонки не лечатся флагом. Дальше — **`-pthread`** и линковка потоков.

## Вопросы для самопроверки

1. Почему баг «только на Release» часто связан с UB, а не с «злым `-O2`»?
2. Что даёт **`-flto`** и где он выполняется в pipeline?
3. Как узнать, какой **`-O`** у **`CMAKE_BUILD_TYPE=Release`** в твоём `build/`?
4. Зачем мерить **`parallel_map`** на Release, а не на Debug?
5. Может ли корректный **`lock_guard`** стать «невидимым» для другого потока из-за `-O2`?

## Обсуждение с репетитором

**Том:** 5 · **Глава:** `ch06` · **Файл:** `ch06-optimization.md`

**Дальше по курсу:** [Глава 7. pthread и линковка](./ch07-pthread-linking.md)
