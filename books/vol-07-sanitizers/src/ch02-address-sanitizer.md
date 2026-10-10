# Глава 2. AddressSanitizer

## Зачем эта тема

Выход за границы массива, **use-after-free**, **double-free**, use-after-return — классические краши и дыры безопасности. В C++ они прячутся за `vector`, `string`, умными указателями и «я же delete сделал». **AddressSanitizer (ASan)** ставит **red zones** вокруг heap-объектов, отслеживает lifetime stack-объектов в типичных сценариях и превращает многие invalid access в **детерминированный** `abort` с отчётом.

ASan **не** заменяет здравый **lifetime**-дизайн (том 1–2: ссылки, `[&]`, dangling). Но когда ошибка уже есть, ASan часто показывает **первую** illegal write/read — быстрее, чем неделя «почему иногда segfault».

## Включение на Fedora

Минимальная однострочная сборка (том 5):

```bash
g++ -std=c++17 -g -O1 -fsanitize=address -fno-omit-frame-pointer \
    main.cpp -o app && ./app
```

**`-fno-omit-frame-pointer`** помогает стеку в отчёте — часто добавляют вместе с ASan/TSan. На **x86_64 Fedora** ASan поддерживают **GCC** и **Clang**.

Для нескольких `.cpp` — те же флаги на **каждый** compile и на **link** итогового exe. CMake-модель — глава 9 (`build-asan`).

## Классы ошибок (карта)

**Heap buffer overflow** — запись или чтение за предел блока, выделенного `operator new`, `malloc`, контейнером на куче. `delete[]` не спасает, если уже писали мимо.

**Stack buffer overflow** — локальный массив `char buf[10]` и копирование длиннее 10. ASan ловит на стеке через **stack canary** / shadow — не полагайся на «маленький массив, не заметит».

**Use-after-free (UAF)** — указатель или ссылка на память после `delete` / `free`. Отчёт часто показывает **allocated here** и **freed here** — два якоря для GDB.

**Double-free** — два освобождения одного адреса. Часто следствие копирования «сырого» указателя или exception path без RAII.

**Use-after-return** — ссылка на локальную переменную, возвращённая наружу (том 2). ASan может поймать, если stack slot переиспользовали.

**Leak** — на Linux часто через **LSan** при exit (глава 12), не отдельный «тип ASan» в заголовке overflow.

## Мини-примеры (учебные, не из multithread)

Heap overflow одной строкой (идея, не копируй в прод):

```cpp
int* p = new int[2];
p[2] = 0;  // heap-buffer-overflow
delete[] p;
```

UAF:

```cpp
int* p = new int(1);
delete p;
*p = 2;    // heap-use-after-free
```

На Fedora запусти с `-fsanitize=address` и прочитай отчёт (глава 3).

## Shadow memory одной фразой

ASan резервирует виртуальную память под **теневую карту**: каждый байт приложения имеет метку «можно / poison». Illegal access → trap → отчёт. Отсюда **рост RSS** и иногда **ASLR**-чувствительность — нормально для instrumented build.

## Связь с томом 1–2: lifetime без гонки

**Dangling reference**, return `T&` на локальное, `[&]` на переменную, которая вышла из scope — иногда ASan ловит как UAF или stack UAF. Не все lifetime-баги: если память **не переиспользовали**, симптом может отсутствовать — «ASan молчит» ≠ «код корректен».

**Rule of Zero/Five** (том 8) снижает double-free; ASan остаётся страховкой на переходный период обучения.

## multithread и ASan

Гонка на памяти может **повредить** внутренности `std::deque` или счётчик allocator — ASan покажет **heap corruption** или invalid read, но **не** напишет «data race». Для гонок — **TSan** (глава 5).

ASan полезен в advanced, если подозреваешь **висячий указатель** на job в пуле, **double move** из очереди, или «кто-то delete не там» — без подстановки кода из `solutions/`.

## vector, string, iterator invalidation

`vector::push_back` может reallocate → все iterator/reference **invalid**, если не обновлены. Два потока без sync — TSan; один поток с висячим iterator — ASan. **`at()`** бросает в debug lib; **`operator[]`** out of range — UB; ASan часто ловит как heap/stack overflow в своих сценариях.

## ASan и оптимизация

Не собирай ASan с **`-O3`** «для скорости» (том 5). **`-O1 -g`** — baseline. **`-O0`** иногда меняет порядок обращений — редко помогает найти баг «лучше», чем `-O1`.

## § К собесу

ASan = `-fsanitize=address`, compile+link, red zones, UAF/double-free/overflow. Не ловит data race явно. Нужен `-g`. LSan часто при exit на glibc Linux.

## Размеры и alignment

ASan сообщает **size** access — 1 byte может быть `bool`, 8 — pointer или `size_t` on amd64. **Misaligned** atomic — UBSan/ASan depending. В выровненном коде курса — реже.

## new[], vector::data, raw loop

`for (size_t i = 0; i <= v.size(); ++i)` на `v[i]` — heap if vector on heap storage — overflow report. Prefer algorithms and bounds checks in **source**, not «ASan as crutch».

## Shared_ptr cycles

Leak at exit (LSan), not overflow. **weak_ptr** breaks cycle — tom 8 preview. multithread rarely uses shared_ptr in exercises.

## Stack array in thread

Large `char buf[N]` on stack — overflow hits **stack** report. Thread stack size limit — if crash without ASan message, check `ulimit -s`.

## Interaction with exceptions

If destructor throws during stack unwind — terminate; ASan may show secondary corruption. **noexcept** policy — later tomes.

## Production note

Some servers run **canary** ASan builds in staging only. Not `-O2` Release with sanitize mixed blindly.

## Типичные заблуждения

«ASan только для C» — C++ тоже. «С `vector` ASan не нужен» — нужен при сырых указателях и UB индексов. «ASan в production Release» — отдельная сборка. «Зелёный тест = нет UAF» — тест мог не дойти до ветки.

## Red zones и allocator

Между соседними heap-блоками ASan оставляет ** poisoned** guard bytes. Запись в guard → немедленный отчёт. При **free** блок часто помечают poisoned — отсюда ловится UAF. Это объясняет, почему «чуть-чуть за границу» иногда ловится сразу, а иногда «далеко» — зависит от layout кучи **без** ASan; с ASan layout **другой**, баги **чаще** всплывают.

**operator new[] / delete[]** must match; `new` + `delete[]` — UB и часто ASan. **Placement new** и custom allocator — ASan может работать, но custom pools иногда **отключают** red zones — в `multithread/` такого нет.

## Stack use-after-return

Компилятор с ASan может **искусственно продлевать** lifetime stack slot или poison после return — ловится return ссылки на локальное. Без ASan тот же код «работает» до перезаписи стека. Это мост к том 2: `[&]` в лямбде, ушедшей в `std::thread` без sync — иногда ASan, чаще TSan + lifetime.

## Global buffer overflow

Глобальные и static массивы тоже под shadow. **Off-by-one** в `char name[32]` для строки из config — классика. UBSan может дополнить, если индекс через signed overflow (глава 4).

## Отладка с GDB после ASan

`gdb ./app` → `run` → на stop: `bt`, `frame 0`, `list`. Символы `-g` показывают строку. **Conditional breakpoint** на функцию, из которой идёт write — tom 6. Не путай **ASan report** (уже напечатан) с **watchpoint** на адрес — watchpoint полезен, если адрес известен из отчёта.

## Сравнение с Valgrind Memcheck

Valgrind не требует пересборки; ASan быстрее на больших бинарниках **после** пересборки. Для одного `task.cpp` на Fedora попробуй оба раз в жизни — почувствуешь скорость. Курс опирается на sanitizers; Valgrind — опция.

## ASan и multithread: сценарий «порча deque»

Два потока вызывают `push_back` на одном `std::deque` без mutex — **data race** (TSan). Симптом может быть **heap-buffer-overflow** внутри libstdc++ (ASan). Интерпретация: **сначала** ищи race (TSan), **не** «виноват STL». Исправление — sync, не suppress на libstdc++.

## Custom deleter и unique_ptr

Wrong deleter (`delete` vs `delete[]`) — ASan double-free or corruption. **Rule of Five** — tom 8.

## Container after move

Use-after-move if access moved-from object — ASan sometimes; logic always wrong (tom 10 move chapter in THEORY).

<!-- vol7-pass5-ch02 -->

## Дополнительное закрепление

Вспомни три класса ошибок ASan: overflow, UAF, double-free. Для каждого — одно слово «где в памяти». Связь с **tom 2**: dangling и move не отменяют ASan.

Если ASan молчит на подозрительный lifetime — прогон TSan или review.

Red zones — poison между объектами; запись в poison = немедленный trap.

## Краткое резюме

`-fsanitize=address`, `-g`, `-O1`. Heap/stack UAF, overflow, double-free. Lifetime — частично. Гонки — TSan. Дальше — читать отчёт.

## Практика: три минуты в /tmp

Создай `bug.cpp`: UAF или overflow, одна команда g++ из главы 2, запусти, сохрани отчёт. Удали `/tmp` файл. Цель — **рука** помнит флаги без шпаргалки.

## ASan и std::string SSO

Small string optimization — buffer inside object — overflow **inside** string object if UB via buggy code — ASan may report on stack/heap depending on size. Rare in course; use `string` normally.

## multithread: pointer to job in queue

If job holds pointer to external buffer freed early — UAF in worker — ASan in TSan build won't replace TSan for race; use **both** builds on pointer suspicion.

<!-- vol7-pass6 -->
## Строка для памяти (Kindle)

Перед сном одним взглядом: **compile+link**, **-g -O1**, **TSan+pthread**, **два build dir**, **два стека в отчёте**, **solutions не спойлерить**. Завтра — один TSan-прогон на 02 или на последнем изменённом advanced target.

## Вопросы для самопроверки

1. Минимальные флаги ASan одной командой?
2. Чем UAF отличается от overflow?
3. Поймает ли ASan гонку на `int`?
4. Зачем `-fno-omit-frame-pointer`?
5. Связь ASan с dangling reference?

<!-- vol7-pass4-ch02 -->
## Закрепление ASan

Перед главой 3 **обязательно** один раз **своими руками** получи отчёт ASan. Без этого глава 3 — теория чтения чужого лога. Даже 10 строк `bug.cpp` достаточно.

В `multithread/` ASan вторичен после TSan, но если в advanced ты хранишь `std::function` с захватом указателя на stack — ASan может сказать UAF раньше, чем TSan скажет race. Порядок инструментов: **подозрение на race** → TSan; **подозрение на lifetime** → ASan+UBSan.

Помни: **AddressSanitizer** не чинит **iterator invalidation** — только ловит последствия. Исправление — дисциплина контейнеров (tom 1–2), не отключение проверок.

## Обсуждение с репетитором

**Том:** 7 · **Глава:** `ch02` · **Файл:** `ch02-address-sanitizer.md`

**Дальше по курсу:** [Глава 3. Отчёт ASan](./ch03-asan-report.md)
