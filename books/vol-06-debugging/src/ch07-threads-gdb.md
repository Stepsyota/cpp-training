# Глава 7. Потоки в GDB

## Зачем эта тема

В `multithread/` почти каждое упражнение — несколько потоков. Один breakpoint показывает **один** контекст. Команды **`info threads`**, **`thread N`**, **`thread apply all bt`** — стандарт при «тест висит» или счётчик не сходится.

## Почему «текущий thread» важен

Любая команда **`print`**, **`bt`**, **`next`** без явного **`thread N`** относится к **выбранному** потоку. Забыл переключиться — «опять queue пуста», хотя смотришь не тот worker. После **`info threads`** явно делай **`thread N`**, смотри **`bt`**, записывай, переключайся.

## Список потоков

```
(gdb) info threads
```

Звёздочка `*` — текущий thread. ID — номер для **`thread 2`**.

```
(gdb) thread 2
(gdb) bt
```

Стек **другого** потока. Вернись: **`thread 1`**.

## Все стеки разом

```
(gdb) thread apply all bt
```

Классика для deadlock: поток 1 в `mutex::lock` на `m`, поток 2 в `lock` на том же `m`, третий в `condition_variable::wait`. Сопоставь с томом 3 (join под mutex — deadlock).

## Имена потоков

Linux pthread name через `pthread_setname_np` **(не в учебнике обязательно)**. GDB показывает `Thread 0x... (LWP 12345)`. LWP — light-weight process, id в ОС.

## Останов всех потоков

По умолчанию при событии в одном потоке GDB часто **останавливает все** (зависит от версии и `set non-stop off`). Уточни:

```
(gdb) show non-stop
```

Для курса: считай, что при breakpoint можно снять снимок, но **порядок** съедания очереди мог уже измениться.

## scheduler-locking

```
(gdb) set scheduler-locking step
```

При step/next другие потоки не бегут — проще рассуждать, **непохоже** на production timing. Только для локального разбора.

## Практика: elevator

Один поток в `wait`, другой должен `notify`. `thread apply all bt`: кто в wait, кто забыл notify, кто держит mutex.

## Практика: thread pool

Воркеры в `cv.wait`, main в `wait_idle`. Если `busy` не ноль, но очередь пуста — смотри locals воркера и поля пула в frame submit.

## Типичные заблуждения

«Один breakpoint = один поток» — срабатывает любой, кто дошёл. «all bt покажет гонку» — только подсказка, не доказательство. «thread 1 = main» — не всегда, смотри bt.

## Краткое резюме

`info threads`, `thread N`, `thread apply all bt`. Для зависаний — все стеки. `scheduler-locking` — опционально для step. Дальше — deadlock сценарии.

## info threads расшифровка

Колонки: Id, Target Id, Frame. **`*`** — current. **`thread find`** — по имени, если задано.

## thread apply all command

**`thread apply all bt`** — к каждому. **`thread apply 1 3 bt`** — выборочно. **`thread apply all print x`** — осторожно, может быть много output.

## non-stop mode

**`set non-stop on`** — threads run independently; сложнее для новичка. Default off на типичной Fedora GDB — все stop при event. **`show non-stop`**.

## Практика: pool wait_idle

Main ждёт **`busy==0`**. Воркер в **`job()`** — **`busy`** положительный. Если main в **`wait`** на cv, а **`busy`** уже 0 — lost notify или predicate (теория). Смотри **оба** thread locals.

## Практика: factory pipeline

**`07`**: несколько потоков на stage. **`all bt`** — кто на **`pop`**, кто на **`push`**, кто **`join`**. Зависание часто «stage 2 ждёт, stage 1 не push'ит».

## set libthread-db

Редко ломается на Fedora; если threads «не видны», проверь **`info threads`** после run с **`pthread`**.

## GDB и TSan

TSan report **лучше** для race; GDB threads **лучше** для «все в mutex lock». Не выбирай одно навсегда.

## § К собесу

Команды при deadlock: interrupt, **`info threads`**, **`thread apply all bt`**, identify lock addresses in frame args.

## thread-specific breakpoints

**`break loc thread 3`** — break only thread 3 hits (GDB 7+). Rarely needed; conditional on **`pthread_self()`** alternative.

## scheduler-locking off before continue long

Забыл выключить — unrealistic single-threaded world too long.

## tls variables

**`thread_local`** — different value per thread; **`print`** in each **`thread N`**.

## fork (не курс)

**`set follow-fork-mode`** — child debug; multithread no fork.

## hw thread count vs software threads

12 threads in **`info threads`**, 6 cores — normal.

## elevator notify path

Switch thread: one in wait, verify other reaches notify line in **`list`** or break there.

## set print thread-events

**`set print thread-events off`** — less noise on thread create during **`run`**.

## all thread stop message

When one hits break, others stop — read GDB message on stop which thread.

## compare LWP in htop

Cross-check OS view vs GDB ids — educational, not required.

## worker numbering

Your pool workers anonymous — identify by bt in worker function same line.

## Дополнительный сценарий: «кто первый взял mutex»

**`thread apply all bt`** не показывает порядок захвата во времени — только момент Ctrl+C. Интерпретация: если два потока в **`lock`** на одном адресе mutex (смотри аргумент **`this`** или адрес в frame), это deadlock или heavy contention. Для **обучения** достаточно увидеть «два waiter'а» и вернуться к дизайну lock ordering из тома 3.

## Переключение thread без путаницы номеров

После **`info threads`** записывая на бумаге: «Thread 3 = worker bt shows wait». Номера **не** стабильны между **`run`** — не зубри «thread 2 всегда main».

## Практическое закрепление (5–10 минут)

Открой Debug-сборку любого **зелёного** target из `multithread/build/`. Запусти GDB, поставь **`break main`**, **`run`**, выполни **`next`** пять раз, **`bt`**, **`info locals`**, **`quit`**. Повтори на другом target — muscle memory важнее заучивания глав. Если что-то не совпало с текстом главы, проверь cwd и путь к exe: nine из десяти «GDB сломан» — неверный путь или сборка без **`-g`**.

## Связь с главой тома 3 без спойлера кода

Вернись мысленно к инварианту упражнения, которое отлаживаешь: что должно быть истинно перед выходом из **`wait`**, когда очередь считается пустой, когда **`submit`** должен молчать. GDB показывает **момент**, когда инвариант уже нарушен или ещё не достигнут. Исправление остаётся в голове и в **`task.cpp`**, не в команде **`continue`**.

## § Коротко к собесу (повтор)

Сформулируй вслух одним предложением главную мысль этой главы и назови **одну** команду GDB, без которой тема не работает. Если не можешь — перечитай «Краткое резюме» и вернись сюда завтра; spaced repetition лучше зубрёжки за вечер.

## Вопросы для самопроверки

1. Как увидеть bt всех потоков одной командой?
2. Почему при отладке пула состояние очереди «прыгает»?
3. Что искать в bt при deadlock на mutex?
4. Как переключиться на поток воркера?
5. Заменяет ли GDB TSan?

## Обсуждение с репетитором

**Том:** 6 · **Глава:** `ch07` · **Файл:** `ch07-threads-gdb.md`

**Дальше по курсу:** [Глава 8. Зависания и deadlock](./ch08-deadlocks-hangs.md)
