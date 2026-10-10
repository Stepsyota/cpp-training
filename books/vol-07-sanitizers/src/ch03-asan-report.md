# Глава 3. Отчёт ASan

## Зачем эта тема

Первый раз ASan выводит **красный простынёй** stderr — паника. На самом деле отчёт структурирован: **тип ошибки**, **адрес**, **стек записи**, иногда **стек выделения** памяти. Умение прочитать за минуту экономит час.

## Типичная структура

1. Строка `ERROR: AddressSanitizer: heap-buffer-overflow` (или другой тип).
2. `READ` / `WRITE` of size N at address ...
3. `#0` функция в **твоём** `.cpp` — **главная** строка для правки.
4. Ниже — цепочка libc, allocator.
5. `allocated by thread T0 here` — где объект создали.
6. `freed by thread T0 here` — для UAF, где освободили.

Ищи **верхний** frame из твоего репозитория, не из `/usr/include`.

## Shadow memory (идея)

ASan резервирует виртуальную память под **теневую карту**: illegal access попадает в poisoned zone → trap. Поэтому ASan **увеличивает** footprint. Детали ABI не нужны для курса — достаточно знать, почему бинарник «толстый».

## Переменные окружения (кратко)

`ASAN_OPTIONS=help=1` — список опций. Пример: `detect_leaks=0` отключить LSan (редко). Для учебника defaults на Fedora обычно OK.

## После отчёта

Исправь код, **пересобери с теми же флагами**, перезапусти. ASan не «чинит» — только диагностирует.

## GDB + ASan

Можно `gdb ./app` и воспроизвести — останов на `__asan_report`. Том 6: `bt` в момент abort.

## Типы сообщений (шпаргалка)

| Строка ERROR | Смысл |
|--------------|--------|
| heap-buffer-overflow | За границу кучи |
| stack-buffer-overflow | За границу стека |
| heap-use-after-free | После delete |
| double-free | Повторный free |
| stack-use-after-return | После return ref |
| global-buffer-overflow | Static/global |

## Thread id в отчёте

`thread T0` — кто alloc/free. Multithreaded UAF: free in worker, use in main — оба T ids помогают.

## Сохранение отчёта

`./app 2> asan.log; echo exit=$?` — attach to tutor chat without re-run flakiness.

## Несколько ошибок

First error halts often — fix one by one. Rebuild each time.

## Отчёт vs core dump

ASan abort may produce core if `ulimit -c unlimited` — tom 6 core chapter. Report already in stderr usually enough.

## Чтение с репетитором

Paste **first own frame** + error type — tutor asks «where lifetime broke» without giving full solution.

## Типичные заблуждения

«Первый frame = виноват» — иногда баг в caller, смотри allocated/freed. «Игнорировать, если тест прошёл» — ASan мог abort до assert теста.

## Первый контакт: не паникуй из-за цвета

ASan печатает в **stderr** блок, начинающийся с `ERROR: AddressSanitizer:` и типа (`heap-buffer-overflow`, `heap-use-after-free`, `stack-buffer-overflow`, `attempting double-free`, …). Ниже — **READ** или **WRITE** of size N at address `0x...`. Размер N подсказывает: byte, int (4), pointer (8) на amd64.

Следующие строки `#0`, `#1`, … — **stack trace**. **#0** — ближайший к сбою frame; для правки ищи **верхний** frame из **твоего** репозитория (`task.cpp`, `thread_pool.cpp`), не из `/usr/include/c++/`.

## Разбор UAF по секциям

Для **use-after-free** ASan часто даёт три истории:

1. **Where the bug occurred** — кто тронул мёртвую память.
2. **freed by thread T0 here** — кто вызвал `delete` / `free`.
3. **previously allocated by thread T0 here** — кто создал объект.

Сценарий: ты видишь `freed` в деструкторе, `write` в другом потоке — lifetime + sync (том 3), не «ASan глючит».

## Heap overflow vs underflow

**Overflow** — индекс ≥ size. **Underflow** — отрицательный индекс или `p[-1]`. Оба — invalid access. Сообщение укажет direction. Off-by-one в цикле `for (i = 0; i <= n; ++i)` — классика.

## Stack traces и inline

С **`-O1`** часть вызовов **inlined** — frame может указывать на call site, не на inner function. **`-fno-omit-frame-pointer`** помогает. Tom 6: `bt full`, `info locals` на frame с твоим кодом.

## ASAN_OPTIONS на Fedora

`ASAN_OPTIONS=help=1 ./app` — список опций runtime. Учебные defaults обычно OK.

- `verbosity=1` — больше деталей.
- `halt_on_error=1` — stop на первой ошибке (default).
- `detect_leaks=0` — временно отключить LSan (глава 12).

Не меняй options, пока не понял первый отчёт «в лоб».

## Symbolization и addr2line

Если видишь только адреса, проверь **`-g`** при сборке. Иногда нужен `llvm-symbolizer` / `addr2line` в PATH — на Fedora с GCC обычно достаточно `-g`.

## После исправления

Fix → **полная** пересборка с теми же `-fsanitize=address`. Запуск снова. Один прогон — не proof; прогони тест несколько раз, если был flaky heap bug.

## GDB: __asan_report

Под GDB sanitizer может остановиться в runtime hook. `bt` → найди frame **до** sanitizer runtime — там твой код. Tom 6: `run`, `c` (continue) только если учишься, не для production debug.

## § К собесу

Верхний свой frame; UAF = alloc + free + bad access; пересборка; shadow map объясняет память; ASan не race detector.

<!-- vol7-pass5-ch03 -->

## Дополнительное закрепление

Checklist: тип ERROR → READ/WRITE → свой frame → alloc/free для UAF → rebuild → rerun. С репетитором — тип ERROR + frame, без paste task.cpp.

**GDB tom 6**: один раз `bt` после ASan abort.

Сохраняй stderr в файл для разбора на Kindle-вечере без повторного run.

## Краткое резюме

Читай тип ошибки и первый свой frame; для UAF — alloc/free stacks. Пересборка с ASan обязательна. Дальше — UBSan.

## Учебный разбор (воображаемый фрагмент)

Строка `ERROR: AddressSanitizer: heap-use-after-free on address 0x603000001234`. `WRITE of size 4`. `#0 process at task.cpp:28`. `freed by thread T1 at task.cpp:22`. `allocated at task.cpp:18`. План: открыть 18–28, нарисовать lifetime на бумаге, **не** править suppress.

## Если frame только в libstdc++

Поднимись по `#1`, `#2` — ищи **твой** вызов `push_back`, `delete`, `memcpy`. Библиотека — симптом; источник — твой контракт.

## Demangle и C++ names

`_ZN...` — demangle in gdb `bt` or readable with `-g` and recent compiler.

<!-- vol7-pass6 -->
## Строка для памяти (Kindle)

Перед сном одним взглядом: **compile+link**, **-g -O1**, **TSan+pthread**, **два build dir**, **два стека в отчёте**, **solutions не спойлерить**. Завтра — один TSan-прогон на 02 или на последнем изменённом advanced target.

## Вопросы для самопроверки

1. Где в отчёте искать свою строку?
2. Что значит heap-buffer-overflow?
3. Зачем смотреть freed by?
4. Нужно ли пересобирать после fix?
5. Что такое shadow memory одной фразой?

<!-- vol7-pass4-ch03 -->
## Навык чтения отчёта

Отработай шаблон: (1) тип ERROR, (2) READ/WRITE size, (3) первый свой frame, (4) для UAF — alloc/free stacks, (5) пересборка, (6) rerun. Шесть шагов — минута на отчёт после практики.

С репетитором принеси **реальный** stderr (можно без всего кода) — разберёте frame, не решение задачи. Так tom 7 поддерживает AGENTS.md: репетитор не подставляет `task.cpp`.

Если отчёт указывает на `test.cpp` — баг может быть в **вызове** из test, но чаще в API, которое test дергает. Поднимись к `task.cpp` логически.

## Обсуждение с репетитором

**Том:** 7 · **Глава:** `ch03` · **Файл:** `ch03-asan-report.md`

**Дальше по курсу:** [Глава 4. UndefinedBehaviorSanitizer](./ch04-ubsan.md)
