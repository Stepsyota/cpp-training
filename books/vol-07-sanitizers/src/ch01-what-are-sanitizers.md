# Глава 1. Что такое санитайзеры

## Зачем эта тема

Undefined behavior и data race **не обязаны** проявляться при каждом запуске. Классический сценарий: на Debug с `-O0` счётчик «вроде сходится», на Release с `-O2` результат другой — компилятор законно перестроил код вокруг гонки или UB. Ещё хуже: программа **годами** проходит regression-тесты, пока не изменится timing или размер кучи. Санитайзер не заменяет доказательство корректности, но **встраивает проверки в бинарник**: при многих классах ошибок процесс падает с **понятным отчётом** и стеком в **твоём** `.cpp`.

Если ты прошёл `multithread/` с репетитором, ты уже знаешь **mutex** и **atomic** «на бумаге». Санитайзер — способ **проверить**, что в коде не осталось plain `int`, на который два потока делают `++` без синхронизации. Это другой слой, чем `-Wall`: предупреждение компилятора — статика; sanitizer — **runtime** на **этом** входе.

## Instrumentation: что меняется в бинарнике

Слово **instrumentation** (инструментация) значит: компилятор вставляет дополнительные проверки и вызовы runtime **AddressSanitizer**, **UBSan** или **ThreadSanitizer**. Твой исходник выглядит почти так же; меняется **object file** и финальный **link**. Поэтому нельзя «включить ASan только на одном `.cpp`» в смешанном executable: часть кода без shadow map, часть с — **ложное чувство** или link error.

Runtime sanitizer — это **библиотека**, линкуемая вместе с `-fsanitize=...`. При старте процесса она регистрирует handlers; при нарушении печатает отчёт в **stderr** и часто вызывает `abort()`. Тестовый harness может не дойти до `assert`, если ASan упал раньше — для обучения это плюс: баг виден до «зелёного» assert.

## Compile-time и link-time

Флаги **`-fsanitize=address`**, **`-fsanitize=undefined`**, **`-fsanitize=thread`** должны быть согласованы на **компиляции** каждой единицы трансляции и на **финальной линковке** executable. Типичная ошибка на Fedora:

```bash
g++ -std=c++17 -c -fsanitize=address file.cpp -o file.o
g++ -std=c++17 file.o main.o -o app
```

Второй шаг без `-fsanitize=address` собирает бинарник **без** полного ASan runtime. Правильная однострочная модель (том 5, глава 13):

```bash
g++ -std=c++17 -g -O1 -fsanitize=address file.cpp -o app
```

Для **TSan** добавь **`-pthread`** на compile **и** link. Драйвер `g++` передаёт часть флагов линковщику, но явная пара **compile options + link options** в CMake (глава 9) снимает сомнения.

## -g и -O1: зачем не «просто Debug»

**`-g`** не меняет семантику; он кладёт таблицы строк для GDB и для имён в отчёте sanitizer. Без `-g` останутся адреса и `??` вместо `task.cpp:42`.

**`-O0`** меняет timing и форму кода; для TSan/ASan часто берут **`-O1`**: лёгкие оптимизации, код ещё читаем. **`-O3`** может скрыть UB (том 5, глава 6). Sanitizer-сборка — **отдельная** конфигурация, не «Release для скорости».

Baseline курса: **`-g -O1 -fsanitize=... -fno-omit-frame-pointer`**.

## Сценарий: первый instrumented build

Представь, что ты собрал `02_ticket_office` обычным `cmake --build build` и тест зелёный. Ты **намеренно** убираешь `lock_guard` на минуту и собираешь:

```bash
g++ -std=c++17 -pthread -g -O1 -fsanitize=thread \
    -fno-omit-frame-pointer -Wall -o t task.cpp test.cpp && ./t
```

Если TSan печатает **WARNING: ThreadSanitizer: data race** — toolchain работает. Возвращаешь mutex и пересобираешь **полностью** — отчёт должен исчезнуть. Это калибровка: ты доверяешь инструменту, когда он ловит **известный** баг.

Если отчёта нет на сломанном коде — проверь link line: был ли `-fsanitize=thread` на последнем `g++`, не смешал ли `.o` из `build/` без sanitizer.

## Exit code, stderr и CI

Sanitizer часто завершает процесс с **ненулевым exit code** после `abort`. Shell-скрипт вроде `check.sh` трактует это как **fail** — правильно. В CI (том 5: **`-Werror`** на compile) sanitizer обычно в **отдельном job**: он медленный и не обязан блокировать каждый push, но nightly TSan на `multithread` targets — разумная модель (глава 8).

**stderr** с отчётом не смешивай с «просто логом» теста: первые строки с `ERROR: AddressSanitizer` или `WARNING: ThreadSanitizer` — главный артеfact.

## Runtime overhead и когда гонять

**ASan** — shadow memory и red zones: память ↑, CPU часто **2–3×**. **TSan** тяжелее (**5–15×**, больше RSS). Не бенчмаркуй пул на TSan-бинарнике.

Привычка: TSan перед «сдал advanced»; ASan+UBSan при сырой памяти; не каждый save.

## Valgrind, static analysis, fuzzing

**Valgrind** — без пересборки, очень медленно. **Sanitizers** — быстрее на больших exe, нужна пересборка.

**`-Wall`**, **clang-tidy** — без запуска. **Fuzzing** (AFL, libFuzzer) — другой класс; sanitizer часто **внутри** fuzz-сборки. Слои дополняют друг друга.

## Три семейства

| Семейство | Флаг | Вопрос |
|-----------|------|--------|
| ASan | `-fsanitize=address` | Доступ к памяти законен? |
| UBSan | `-fsanitize=undefined` | Операция — UB? |
| TSan | `-fsanitize=thread` | Data race на адресе? |

ASan может показать corruption **из-за** гонки, но не «data race» — нужен TSan.

## Связь с томами 5–6 и multithread

Том 5: **link**, **pthread**, отдельный `build-tsan`. Том 6: **GDB** после `abort`. Том 3: модель гонки. **`multithread/solutions/`** в книге не используем — только свой код.

## § К собесу

Sanitizer на compile **и** link; **`-g -O1`**; ASan+UBSan вместе; **TSan отдельно**; `-Wall` не ловит race; зелёный тест ≠ безопасность; не production Release.

## Словарь на одной странице

**Sanitizer** — семейство runtime-проверок при специальной сборке. **Instrumentation** — вставленный compiler код. **False positive** — отчёт без реального бага (редко в учебниках). **False negative** — баг есть, отчёта нет (чаще: гонка не сыграла). **Recover** — продолжить после UBSan (обычно off в обучении). **Shadow memory** — карта ASan. **Happens-before** — порядок для TSan (том 3).

## Fedora: что установить

`g++`, `clang++`, `cmake`, `gdb` — tom 5. Sanitizers **внутри** compiler, отдельный dnf пакет «asan» не нужен. `mdbook` — для EPUB тома, не для sanitizer.

## Репозиторий training: границы

Книга **не** правит `multithread/check.sh`, `.gitlab-ci.yml`, `CMakeLists.txt` без запроса. Ты **можешь** локально добавить `ENABLE_TSAN` — tom 5 и глава 9 показывают **как**, не **commit** без хозяина.

## Педагогика: сломай и почини

Перед «я всё понял» — **controlled bug**: убери mutex, запусти TSan, верни mutex. То же с UAF на маленьком `.cpp` в `/tmp`. Мышечная память на **флагах** важнее заучивания типов отчётов.

## Kindle и код

На узком экране переноси длинные `g++` строки mentally: **std**, **g**, **O1**, **sanitize**, **pthread**, **output**. В EPUB код остаётся monospace — tom 0 про чтение.

## Типичные заблуждения

«Sanitizer в Release» — отдельная сборка. «Один `-fsanitize` на всё» — разные семейства. «Зелёный без sanitizer» — мало значит. «TSan = review пула» — логика очереди без гонки TSan не видит.

## Практический workflow на одной машине

Заведи привычку **двух деревьев сборки**, как в томе 5: `multithread/build` — быстрый цикл с `./check.sh`; `multithread/build-tsan` — только когда трогал shared state. Переключение без `rm -rf` допустимо, если **не** переиспользуешь `.o` между деревьями: после смены `-fsanitize` — clean или новый `-B`.

Перед первым TSan-прогоном убедись, что **версия** компилятора свежая (`g++ --version`, `clang++ --version`, глава 10). Старый toolchain на LTS иногда не собирает TSan — это не «sanitizer сломан», а сигнал обновить Fedora или взять `clang++` из `dnf`.

Когда отчёт sanitizer длинный, **не скролль в панике**: сохрани stderr в файл (`./t 2> report.txt`), открой в редакторе, найди первый frame с `task.cpp` или твоим заголовком. Том 6: тот же бинарник под GDB — `run`, дождись `abort`, `bt`.

## Связь с warnings и CI (том 5)

**`-Werror`** в CI превращает warning в fail **на этапе compile**. Sanitizer fail — **на run**. Имеет смысл держать оба слоя: compile строго, run с TSan на nightly. Репозиторий `training` по умолчанию **не** включает sanitizer в `.gitlab-ci.yml` — книга описывает модель, **не** меняет CI без запроса хозяина.

Если когда-нибудь добавишь job: отдельный образ, `cmake -B build-tsan` с флагами из главы 9, `./build-tsan/02_ticket_office` — timeout больше обычного (TSan медленный).

## Когда sanitizer «молчит»

Instrumented build **не** доказывает отсутствие всех багов: ветка не вызвана, гонка не сыграла, UB в другом translation unit без флагов. **Stress** (больше итераций, больше потоков) и **второй компилятор** (GCC vs Clang, глава 10) — усиление, не парanoia.

Sanitizer **не** ловит неверный инвариант под mutex: очередь закрыли и выбросили jobs — логика (том 3, 7). **Deadlock** — GDB (том 6), не TSan report.

## Историческая заметка

Sanitizers выросли из **LLVM** ecosystem (Google, Apple); GCC интегрировал совместимые runtime. На курсе важен **интерфейс** (`-fsanitize=`, формат отчёта), а не внутренний ABI shadow map — unless ты читаешь исходники compiler-rt.

## Переменные окружения (обзор)

`TSAN_OPTIONS`, `ASAN_OPTIONS`, `UBSAN_OPTIONS` — runtime tuning. Start with defaults; `help=1` when debugging sanitizer itself (rare).

## Обучающий минимум на неделю

День 1: ASan one-file overflow. День 2: read report ch3. День 3: UBSan shift. День 4–5: TSan 02. День 6: CMake build-tsan. День 7: ch14 checklist on real diff.

<!-- vol7-pass5-ch01 -->

## Дополнительное закрепление

Перечитай таблицу трёх семейств вслух. Сформулируй одним предложением, почему sanitizer не заменяет `-Wall`. Запиши две команды g++ (ASan+UBSan и TSan) в личные notes — без commit в repo.

Связь с **tom 0**: mdBook EPUB собирается `mdbook build`; sanitizer нужен для **кода** `multithread/`, не для сборки книги.

Перед ASan (глава 2) убедись, что tom 5 главы 5–7 про `-g`, link и pthread — свежи в памяти.

## Краткое резюме

**`-fsanitize=...`**, compile+link, **`-g -O1`**, TSan + **`-pthread`**. Медленнее, ловит UB/race. Дальше — ASan.

<!-- vol7-pass3-ch01 -->
## Разговор с репетитором: типичные вопросы

«Почему не добавить sanitizer в основной CMake?» — Потому что каждый прогон станет в разы медленнее; тренажёр рассчитан на быстрый цикл `./check.sh`. Sanitizer — **ворота качества** перед сдачей, не замена dev build.

«Можно ли TSan только на test.cpp?» — Нет смысла: race в **task.cpp** не instrumented, если флаг только на test TU. Весь exe одинаково.

«Sanitizer и coroutines/future» — tom 4; shared state в continuation без sync — TSan как обычно.

## Мини-глоссарий runtime

**Abort** после отчёта — ненулевой exit. **Interceptors** — ASan wraps malloc. **Vector clock** — TSan internal hb tracking (не путать с SIMD). Знать internals не нужно — нужно читать **свой** frame.

## Связь с THEORY.md

`multithread/THEORY.md` живой конспект; tom 7 не дублирует mutex/cv теорию, а **инструмент** проверки. После главы 5 перечитай THEORY § про гонку — слова совпадут с отчётом TSan.

<!-- vol7-pass6 -->
## Строка для памяти (Kindle)

Перед сном одним взглядом: **compile+link**, **-g -O1**, **TSan+pthread**, **два build dir**, **два стека в отчёте**, **solutions не спойлерить**. Завтра — один TSan-прогон на 02 или на последнем изменённом advanced target.

## Вопросы для самопроверки

1. Почему sanitizer на **link**?
2. Зачем **`-O1`** с TSan?
3. Sanitizer vs **`-Wall`**?
4. **`-pthread`** с TSan?
5. Заменяет ли sanitizer тесты?

<!-- vol7-pass4-ch01 -->
## Итоговая картина главы 1

Sanitizer — не «ещё один флаг», а **договор** с toolchain: ты собираешь **другой** artifact, запускаешь **те же** тесты или сценарии, читаешь stderr. Этот artifact не уходит пользователю и не участвует в сравнении «мой пул быстрее твоего» — tom 5 Release для perf, tom 7 build-tsan для **корректности** concurrency и памяти.

На собеседовании часто спрашивают «чем TSan отличается от mutex». Mutex — **механизм** в коде; TSan — **детector**, что механизм забыли. Оба нужны в голове: mutex в production, TSan в dev/CI.

Если после tom 7 остаётся один страх — «я не запомню строку g++» — сохрани **две** команды в личный `notes.md`: ASan+UBSan one-file, TSan multithread one-target. BOOK-CONTEXT не требует заучивания — требует **понимания**, зачем compile=link.

## Обсуждение с репетитором

**Том:** 7 · **Глава:** `ch01` · **Файл:** `ch01-what-are-sanitizers.md`

**Дальше по курсу:** [Глава 2. AddressSanitizer](./ch02-address-sanitizer.md)
