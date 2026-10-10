# Глава 6. Стек вызовов

## Зачем эта тема

Исключение, assert или breakpoint в глубине `job()` — вопрос «**кто вызвал**» решает **`backtrace`**. Без стека ты правишь симптом в воркере, а ошибка в `submit` или в тесте.

## Backtrace как «история звонков»

Представь стек как стопку тарелок: **frame 0** — верхняя, где ты **сейчас**; чем больше номер, тем глубже «кто вызвал кого». При чтении **`bt`** иди **снизу вверх** мысленно: **`main`** → … → текущая строка. В многопоточности у **каждого** thread своя стопка — один **`bt`** без **`thread`** показывает только выбранный поток.

## bt

```
(gdb) backtrace
(gdb) bt
```

Список frame от текущего к `main` (или к `start_thread`). Номер frame **`#0`** — где стоим.

```
(gdb) backtrace full
```

С locals в каждом frame — длинно, но полезно.

## frame, up, down

```
(gdb) frame 2
(gdb) up
(gdb) down
```

Перейти на другой уровень стека и смотреть **`info locals`** там. В frame caller увидишь аргументы `submit`.

## Где упали

При SIGSEGV GDB останавливается на faulting instruction:

```
(gdb) bt
```

Часто видно nullptr dereference. **`info registers`**, **`x/10gx addr`** — низкий уровень; для курса достаточно bt + строка.

## assert и abort

`assert` → `abort` → SIGABRT. bt покажет цепочку до `assert`. В release с `NDEBUG` assert нет — другой сценарий.

## Исключения C++

При `catch throw` bt в момент throw. Если программа завершилась с `terminate` из `std::thread` (забыли join) — bt в момент abort укажет на ~thread destructor path — связь с томом 3.

## Типичные заблуждения

«bt только после crash» — нет, на breakpoint тоже. «frame 0 = main» — нет, 0 = текущее. «полный стек всегда точен» — при `-O2` inline может скрыть frames.

## Краткое резюме

`bt`, `frame`, `up`/`down`, `backtrace full` при необходимости. Первый шаг при segfault и при «откуда вызов». Дальше — потоки.

## Полный стек и глубина

**`bt 20`** — первые 20 frame. **`bt full`** — locals everywhere — verbose. Фильтруй глазами frame с твоими путями `multithread/`.

## select frame и info args

**`info args`** — параметры функции frame. **`info locals`**. **`info frame`** — язык, адрес, sig.

## Inline frames

На **`-Og`/`-O2`** **`bt`** может показывать **`inline`** frames — нормально. **`set print inline on`**.

## SIGSEGV и nullptr

Frame #0 — fault. Ищи первый **твой** frame выше libc. **`print ptr`** в том frame часто **`0x0`**.

## terminate и thread

bt на abort после **`~thread()`** — ищи **`std::terminate`**, **`~thread`**, твой **`main`** без join. Том 3 объяснял контракт; GDB **показывает** факт.

## Исключения

Если **`catch throw`**, bt в throw point. Unhandled — **`terminate`** path. В worker пула try/catch — break на **`catch`**.

## Запись bt для репетитора

Скопируй **`thread apply all bt`** в чат репетитору (без solutions) — достаточно для «где deadlock», не нужен весь locals.

## core vs live bt

Post-mortem (глава 9) — те же команды, но **нет** `print` changing state — только чтение.

## § К собесу

«Как найти, кто вызвал функцию?» — **`bt`**, **`up`**, **`frame`**.

## where vs bt

**`where`** alias **`bt`**. **`backtrace`** same.

## filter frames

**`bt -qualified`** — demangled C++ names readable.

## main thread identification

Ищи frame с **`main`** и **`test.cpp`** — who drives test harness.

## async stack

**`03_oracle`**: frames через **`std::thread`** / future internals — не пугайся libstdc++ depth.

## saved bt in issue template

«Steps: run pool, Ctrl+C, bt attached» — professional habit.

## correlation with assert message

Assert text + bt frame = fast fix path.

## thread apply all bt full

Huge output on 8 threads — pipe to file via **`set logging`**.

## newest frame first

#0 current — walk **up** to main mentally.

## corrupted stack

Stack smash — bt nonsense; ASan first (tom 7).

## no symbols frame

**`??`** — rebuild with **`-g`**.

## Дополнительный сценарий: assert в test.cpp

Тестовый harness иногда вызывает **`assert`** при неверном результате. В Debug bt покажет assert macro frame и твой код выше. Исправляй **логику** в task, не assert в test — test задаёт контракт. Если assert срабатывает «иногда», подозревай гонку, а не «сломанный assert».

## Цепочка от worker к main

В пуле bt от **`job()`** вверх часто проходит через trampoline потока. Ищи **первый** frame с именем из твоих файлов — там аргументы и locals имеют смысл для репетитора.

## Практическое закрепление (5–10 минут)

Открой Debug-сборку любого **зелёного** target из `multithread/build/`. Запусти GDB, поставь **`break main`**, **`run`**, выполни **`next`** пять раз, **`bt`**, **`info locals`**, **`quit`**. Повтори на другом target — muscle memory важнее заучивания глав. Если что-то не совпало с текстом главы, проверь cwd и путь к exe: nine из десяти «GDB сломан» — неверный путь или сборка без **`-g`**.

## Связь с главой тома 3 без спойлера кода

Вернись мысленно к инварианту упражнения, которое отлаживаешь: что должно быть истинно перед выходом из **`wait`**, когда очередь считается пустой, когда **`submit`** должен молчать. GDB показывает **момент**, когда инвариант уже нарушен или ещё не достигнут. Исправление остаётся в голове и в **`task.cpp`**, не в команде **`continue`**.

## § Коротко к собесу (повтор)

Сформулируй вслух одним предложением главную мысль этой главы и назови **одну** команду GDB, без которой тема не работает. Если не можешь — перечитай «Краткое резюме» и вернись сюда завтра; spaced repetition лучше зубрёжки за вечер.

## Вопросы для самопроверки

1. Что такое frame #0?
2. Как посмотреть locals в caller?
3. Что даст bt при SIGSEGV?
4. Как связан bt с забытым join?
5. Почему стек короче в Release?

## Обсуждение с репетитором

**Том:** 6 · **Глава:** `ch06` · **Файл:** `ch06-backtrace.md`

**Дальше по курсу:** [Глава 7. Потоки в GDB](./ch07-threads-gdb.md)
