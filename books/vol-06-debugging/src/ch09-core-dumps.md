# Глава 9. Core dump

## Зачем эта тема

Программа упала на CI или у тестировщика без GDB под рукой — **core file** сохраняет память и стеки на момент crash. Ты открываешь **`gdb ./exe core`** post-mortem и смотришь **`bt`** как при живом сеансе.

## Core vs живой GDB

Живой сеанс позволяет **`print`**, менять flow, **`continue`**. Core — **замороженный** снимок: идеален, когда процесс уже мёртв или машина перезагрузилась. Нельзя «продолжить» core как **`run`** — только анализ. Для учебного segfault часто хватает живого **`run`** до SIGSEGV; core учат на случай «упало у коллеги».

## Включить core на Fedora

```bash
ulimit -c unlimited
```

Проверка:

```bash
ulimit -c
```

`unlimited` или число килобайт. В systemd/cgroup лимиты могут обрезать — для локальной отладки обычно OK.

Запуск:

```bash
./build/05_highscore
# segfault → core dumped
```

Имя файла: часто **`core`** или `core.PID` (зависит от `/proc/sys/kernel/core_pattern`). На Fedora может быть `coredumpctl` (systemd):

```bash
coredumpctl list
coredumpctl gdb
```

## Открыть в GDB

```bash
gdb ./build/05_highscore core
```

или

```
(gdb) core-file core
```

```
(gdb) bt
(gdb) thread apply all bt
```

Символы нужны те же **`-g`**; исходники на тех же путях.

## Когда core не создаётся

`ulimit -c 0`, setuid бинарник, некоторые контейнеры, abort без core policy. Проверь `dmesg` и `coredumpctl`.

## Не путать с sanitizer

ASan/TSan при ошибке печатают отчёт и **exit**, core может быть отключён или бесполезен. Том 7 — отдельный workflow.

## Безопасность

Core содержит **всю память процесса** — секреты, пароли. Не коммить core в git; не шарить публично.

## Типичные заблуждения

«core = всегда файл core в cwd» — смотри core_pattern. «post-mortem без -g» — только адреса. «core на каждый assert» — зависит от настроек.

## Краткое резюме

`ulimit -c unlimited`, воспроизвести crash, `gdb exe core`, `bt`. `coredumpctl` на systemd. Нужны символы. Дальше — cgdb.

## core_pattern на Fedora

```bash
cat /proc/sys/kernel/core_pattern
```

Может быть **`|/usr/lib/systemd/systemd-coredump`** — файл не в cwd, а в journal. **`coredumpctl list`**, **`coredumpctl info PID`**, **`coredumpctl debug`**.

## ulimit в скрипте

**`ulimit -c unlimited`** действует на shell. CI без core — норма. Локально добавь в **`~/.bashrc`** только если понимаешь размер диска.

## gdb exe core

Символы и **те же** build paths. Если exe пересобрали — core от **старого** exe **несовместим** с новым — misleading bt.

## thread apply all на core

Post-mortem — все threads застыли на crash. Идеально для «кто писал в nullptr».

## assert и core

Зависит от **`abort()`** и ulimit. **`NDEBUG`** в Release — assert нет, другой путь бага.

## docker / flatpak

Core может быть отключён политикой. Локальный **`multithread/build`** на хосте — наш кейс.

## Практика без намеренного crash

Не ломай exe ради core в сданной задаче; для эксперимента — копия с **`*(int*)0=0`** в отдельной ветке.

## § К собесу

Workflow: enable core → reproduce → **`gdb prog core`** → **`bt`**.

## core ulimit in systemd user session

**`ulimit -c`** in IDE terminal may differ from ssh — check both if core missing.

## gdb -c vs core-file

Equivalent paths to load core.

## missing debuginfo packages

Fedora **`debuginfo-install`** for libc — rarely for own `-g` exe.

## ASan abort core

Often disabled; read report text first (tom 7).

## retention

**`coredumpctl`** vacuum old cores — disk hygiene.

## reproduce minimal

Smallest input that crashes — same for core and live gdb.

## zstd compressed cores

Modern Fedora may compress — **`coredumpctl`** hides complexity.

## partial core

**`ulimit -c 1000`** tiny — truncated useless bt.

## kill -ABRT

Force core from running process — lab trick only.

## match build-id

**`readelf -n ./exe | grep Build`** — match core note when advanced.

## Практическое закрепление (5–10 минут)

Открой Debug-сборку любого **зелёного** target из `multithread/build/`. Запусти GDB, поставь **`break main`**, **`run`**, выполни **`next`** пять раз, **`bt`**, **`info locals`**, **`quit`**. Повтори на другом target — muscle memory важнее заучивания глав. Если что-то не совпало с текстом главы, проверь cwd и путь к exe: nine из десяти «GDB сломан» — неверный путь или сборка без **`-g`**.

## Связь с главой тома 3 без спойлера кода

Вернись мысленно к инварианту упражнения, которое отлаживаешь: что должно быть истинно перед выходом из **`wait`**, когда очередь считается пустой, когда **`submit`** должен молчать. GDB показывает **момент**, когда инвариант уже нарушен или ещё не достигнут. Исправление остаётся в голове и в **`task.cpp`**, не в команде **`continue`**.

## § Коротко к собесу (повтор)

Сформулируй вслух одним предложением главную мысль этой главы и назови **одну** команду GDB, без которой тема не работает. Если не можешь — перечитай «Краткое резюме» и вернись сюда завтра; spaced repetition лучше зубрёжки за вечер.

## Вопросы для самопроверки

1. Как включить core в текущей shell?
2. Чем `coredumpctl gdb` удобен?
3. Зачем те же пути к исходникам?
4. Почему TSan-report не заменяет core?
5. Можно ли коммитить core?

## Обсуждение с репетитором

**Том:** 6 · **Глава:** `ch09` · **Файл:** `ch09-core-dumps.md`

**Дальше по курсу:** [Глава 10. cgdb](./ch10-cgdb.md)
