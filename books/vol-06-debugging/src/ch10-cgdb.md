# Глава 10. cgdb

## Зачем эта тема

Терминал — главный инструмент на Fedora и в SSH. Но читать **`list`** по десять строк за раз утомляет, когда отлаживаешь длинный **`factory.cpp`** или свой **`thread_pool.cpp`**. **cgdb** не меняет семантику GDB: breakpoint, thread, **`print`** — те же. Меняется **layout**: исходник сверху, командная строка снизу, подсветка синтаксиса (если терминал не монохромный). Для чтения книги на Kindle и практики на ноуте cgdb — компромисс между «голым» GDB и тяжёлой IDE.

Если ты уже настроил **`launch.json`** (глава 11), cgdb всё равно полезен на сервере без GUI и когда IDE не открыта. Одна и та же muscle memory **`:run`**, **`:bt`** переносится.

Чистый GDB в терминале — только текст. **cgdb** — обёртка с панелью исходника и командной строкой GDB внизу: удобнее на Kindle-ноуте без полноценной IDE, чем встроенный TUI GDB.

## Соседство с tmux и SSH

На удалённой машине часто **`tmux new -s dbg`**, внутри — cgdb. Отсоединился — сеанс жив; вернулся — те же breakpoints (пока процесс gdb жив). Размер pane не менее четверти экрана по высоте — иначе панель исходника бесполезна. **`echo $TERM`** должен быть нормальным (`xterm-256color`, `screen-256color` в tmux).

## Установка

```bash
sudo dnf install cgdb
```

Запуск как gdb:

```bash
cgdb ./build/04_elevator
```

Внутри те же команды: **`:break main`**, **`:run`**, **`:next`** — двоеточие переводит в режим команд GDB (или Esc в зависимости от настроек; см. help в cgdb).

## Навигация

Стрелки по исходнику, **`o`** — перейти к текущей строки выполнения. Breakpoint часто ставится на текущей строке клавишей (см. `?` в cgdb — keymap может отличаться по версии).

## Когда cgdb, когда IDE

cgdb — лёгкий, SSH-friendly, не требует launch.json. IDE — точки мышкой, интеграция с CMake. Оба вызывают GDB.

## Сравнение с IDE одной фразой

IDE хранит дерево проекта; cgdb показывает **один** текущий файл навигации. Для **`multithread/advanced`** с несколькими `.cpp` переключай **` :set`** / open file по имени (см. **`?`**) или держи второй терминал с **`less`** на нужном header. cgdb — не замена IDE для больших рефакторингов, только для **сессии отладки**.

## Ограничения

Большие шаблонные заголовки в view тормозят. Многопоточность — те же `info threads` в command line. Не все версии cgdb одинаково дружелюбны к UTF-8 в путях.

## Связь с томом 5

Собери Debug до запуска cgdb. Путь к exe — из `multithread/build`.

## Типичные заблуждения

«cgdb = другой отладчик» — это GDB. «не нужен gdb пакет» — cgdb зависит от gdb.

## Краткое резюме

`dnf install cgdb`, запуск на exe, команды GDB с `:`. Альтернатива TUI для терминала. Дальше — IDE.

## cgdb vs gdb TUI

**cgdb** — отдельный UI, подсветка синтаксиса, scroll исходника. Команды с **`:`** prefix в command window. **`ESC`** — режим навигации по исходнику (проверь **`?`** на версии).

## Запуск с аргументами

```bash
cgdb --args ./build/06_thread_pool
```

## Когда не cgdb

Нужен copy-paste bt в чат — plain gdb иногда проще. IDE — breakpoints на всём проекте.

## UTF-8 пути

Репозиторий с ASCII путями — OK. Проблемы кириллицы в пути к exe — редкость на Fedora home.

## Многопоточность в cgdb

**`:info threads`** как в gdb. Нет «magic thread UI» — command line.

## Сессия на SSH

cgdb через SSH при **`TERM=xterm-256color`** обычно работает; **`tmux`** + cgdb — удобно.

## § К собесу

«cgdb — frontend к GDB» — достаточно.

## colors and themes

cgdb syntax colors help scan; not required for learning.

## split layout resize

Terminal font size affects source pane; Kindle reading = desktop practice.

## map gdb init

cgdb reads gdbinit — same **`set print pretty on`**.

## multiple files

**`:set`** commands — see cgdb docs for tab between sources.

## when plain gdb wins

Copy-paste bt to tutor chat — fewer UI glitches.

## pairing with tmux

Pane 1 build, pane 2 cgdb — workflow from tom 5 shell habits.

## switch to gdb command mode

Practice **`:`** prefix 10 times until muscle memory.

## rebuild after edit

Same as gdb — cgdb does not build.

## mouse support

Terminal dependent; often off over SSH.

## status bar

Shows file:line current — mirror of gdb TUI info.

## Дополнительный сценарий: первая сессия cgdb

Собери **`04_elevator`**, запусти **`cgdb --args ./build/04_elevator`**, **` :break main`**, **` :run`**, **` :next`** несколько раз. Если UI не виден — проверь размер терминала (минимум ~80×24). Вернись к plain gdb, если SSH client ломает TUI — не все среды равны.

## cgdb и copy-paste bt

Command window позволя выделить **`thread apply all bt`** output мышью — удобнее для репетитора, чем scrollback plain gdb в некоторых терминалах.

## Практическое закрепление (5–10 минут)

Открой Debug-сборку любого **зелёного** target из `multithread/build/`. Запусти GDB, поставь **`break main`**, **`run`**, выполни **`next`** пять раз, **`bt`**, **`info locals`**, **`quit`**. Повтори на другом target — muscle memory важнее заучивания глав. Если что-то не совпало с текстом главы, проверь cwd и путь к exe: nine из десяти «GDB сломан» — неверный путь или сборка без **`-g`**.

## Связь с главой тома 3 без спойлера кода

Вернись мысленно к инварианту упражнения, которое отлаживаешь: что должно быть истинно перед выходом из **`wait`**, когда очередь считается пустой, когда **`submit`** должен молчать. GDB показывает **момент**, когда инвариант уже нарушен или ещё не достигнут. Исправление остаётся в голове и в **`task.cpp`**, не в команде **`continue`**.

## § Коротко к собесу (повтор)

Сформулируй вслух одним предложением главную мысль этой главы и назови **одну** команду GDB, без которой тема не работает. Если не можешь — перечитай «Краткое резюме» и вернись сюда завтра; spaced repetition лучше зубрёжки за вечер.

## Вопросы для самопроверки

1. Чем cgdb отличается от gdb?
2. Зачем cgdb в SSH-сессии?
3. Нужен ли отдельно пакет gdb?
4. Где смотреть горячие клавиши?
5. Решает ли cgdb гонки?

## Обсуждение с репетитором

**Том:** 6 · **Глава:** `ch10` · **Файл:** `ch10-cgdb.md`

**Дальше по курсу:** [Глава 11. Отладчик в IDE](./ch11-ide-debugger.md)
