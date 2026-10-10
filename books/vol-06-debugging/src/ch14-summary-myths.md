# Глава 14. Резюме и мифы

## Зачем эта тема

Том дал инструменты — важно не смешать их с **доказательством корректности**. Финал: карта команд, мифы, собеседование, мост к санитайзерам.

## Что ты должен уметь после тома 6

Собрать Debug exe для любого target **`multithread/`**, открыть GDB, воспроизвести fail, снять **`bt`** или **`thread apply all bt`**, поставить conditional breakpoint, отличить hang от crash, знать, когда идти в tom 7. Не обязан знать все subcommands GDB — обязан **не паниковать** при зависшем **`06_thread_pool`**.

## Карта сеанса

1. Собрать **Debug** с `-g` (том 5).
2. `gdb --args ./build/target` → `run`.
3. При падении или Ctrl+C при hang → **`thread apply all bt`**.
4. Breakpoints / conditional / `next` vs `step`.
5. `print`, `info locals`, `frame`, `up`.
6. Core: `ulimit -c`, `gdb exe core`.
7. cgdb или IDE для удобства; pretty-printers для vector.

Многопоточность: всегда **все потоки** при зависании. Пул/очередь: mutex + cv в стеке — том 3.

## Мифы

**«GDB найдёт гонку»** — нет, только подсказки и тайминг.

**«printf безопаснее»** — меняет interleaving.

**«Зелёный под gdb = нет багов»** — scheduler-locking и остановки меняют порядок.

**«TSan не нужен, есть gdb»** — наоборот для race.

**«Release не отлаживают»** — RelWithDebInfo / `-g -O2` возможны, тяжелее.

**«IDE отладчик другой»** — чаще GDB MI.

**«core всегда в папке проекта»** — coredumpctl.

## К собесу

- Разница `step` / `next` / `finish`.
- Как отладить deadlock (команды).
- Зачем `-g` и Debug.
- Post-mortem core workflow.
- Почему optimized out на `-O2`.
- Один сценарий: тест висит в thread pool — что смотреть в bt.

## Если осталась одна слабая глава

Вернись не к перечитыванию всего тома, а к **практике**: одна hang-situация, три команды, один абзац в **`THEORY.md`**. Том 6 держится на muscle memory, не на зубрёжке всех subcommands **`help`**.

## Дальше по серии

**Том 7** — AddressSanitizer, UBSan, **ThreadSanitizer**: флаги из тома 5, отчёт с двумя стеками, связка с упражнениями `02`, `05`, advanced. После TSan GDB остаётся для логики и deadlock.

**Том 8** — ООП/SOLID в конце серии.

## Краткое резюме

GDB — останов, инспекция, стеки, потоки, core. Не заменяет sanitizers. Собирай Debug, учи `thread apply all bt`. Конец тома 6.

## Чеклист перед сдачей упражнения

1. **`check.sh`** или target без GDB.
2. Если был hang — убедись, что **`thread apply all bt`** больше не показывает deadlock **после** fix.
3. Убери временные **`display`**, breakpoints из `.gdbinit` не commit'ь.

## Карта команд по ситуации

При **crash** (SIGSEGV, abort) первым делом **`bt`**, затем **`frame 0`** и **`print`** подозрительный указатель в этом frame. При **hang** — **Ctrl+C**, затем **`thread apply all bt`**. При **неверном значении** без падения — conditional **`break`**, **`display`**, цикл **`continue`**. При **post-mortem** — **`gdb exe core`** и снова **`bt`**. Держи эту карту в голове рядом с tom 5 (как собрать `-g`) и tom 7 (когда включать TSan).

## Миф: «отладил — значит корректно»

Корректность многопоточного кода = модель памяти + инварианты, не один удачный GDB run.

## Миф: «TSan после GDB бесполезен»

TSan находит interleaving, который GDB не «видел». Порядок: логика GDB → TSan при race suspicion.

## Миф: «IDE breakpoint другой»

Тот же INT3 / software breakpoint (упрощённо).

## Повтор связи с томами

**5** — flags. **3** — mutex/cv. **4** — future. **7** — sanitizers. **6** — ты здесь, runtime inspection.

## Дальнейшая практика

Возьми зелёный **`06`**, сломай **копию** (не solutions): убери notify, поставь wrong predicate — отработай **`all bt`**, почини. Восстанови зелёный **`check.sh`**.

## § Финал собес

Расскажи одну историю: «тест висел → all bt → join under mutex → fix design» — без кода эталона, только паттерн.

## One-page cheat mentally

run, break, continue, next, step, finish, print, bt, thread, thread apply all bt, core, cgdb, IDE F5.

## Habit stack for multithread graduate

After course: green check.sh, then break one advanced module yearly to keep gdb muscle.

## Interview story structure

Situation, tool (gdb all bt), finding (mutex order), fix type (design), verification (tests + tsan optional).

## humility

GDB did not prove absence of bugs — tests + sanitizers + code review.

## thank tom 3

Concurrency theory + gdb stacks = complete hang story.

## open tom 7 folder

Next reading: **`books/vol-07-sanitizers`** introduction when ready.

## Retention quiz

Close book, write from memory: three hang commands, step vs next, one TSan reason.

## Series order reminder

0 guide, 1 keywords, 2 lambdas, 3 concurrency, 4 async, 5 flags, **6 debug**, 7 sanitize, 8 OOP.

## Feedback to THEORY.md

Add gdb tricks you discovered to **`multithread/THEORY.md`** — living notes per AGENTS.md.

## End tom 6

You can debug C++17 multithread exercises on Fedora with confidence; next deepen **automatic** bug detection.

## Дополнительный сценарий: устный экзамен себе

Закрой ноутбук, проговори три минуты: как отлаживаешь hang в thread pool, как отличаешь GDB от TSan, какие три команды GDB ты использовал чаще всего в **`multithread/`**. Пробелы — отметь номера глав для повтора.

## После тома 6

Поставь напоминание открыть **`vol-07-sanitizers`**: TSan допишет то, что GDB принципиально не доказывает. Том 6 остаётся в закладках для deadlock и post-mortem core.

## Практическое закрепление (5–10 минут)

Открой Debug-сборку любого **зелёного** target из `multithread/build/`. Запусти GDB, поставь **`break main`**, **`run`**, выполни **`next`** пять раз, **`bt`**, **`info locals`**, **`quit`**. Повтори на другом target — muscle memory важнее заучивания глав. Если что-то не совпало с текстом главы, проверь cwd и путь к exe: nine из десяти «GDB сломан» — неверный путь или сборка без **`-g`**.

## Связь с главой тома 3 без спойлера кода

Вернись мысленно к инварианту упражнения, которое отлаживаешь: что должно быть истинно перед выходом из **`wait`**, когда очередь считается пустой, когда **`submit`** должен молчать. GDB показывает **момент**, когда инвариант уже нарушен или ещё не достигнут. Исправление остаётся в голове и в **`task.cpp`**, не в команде **`continue`**.

## § Коротко к собесу (повтор)

Сформулируй вслух одним предложением главную мысль этой главы и назови **одну** команду GDB, без которой тема не работает. Если не можешь — перечитай «Краткое резюме» и вернись сюда завтра; spaced repetition лучше зубрёжки за вечер.

## Вопросы для самопроверки

1. Три команды при зависании?
2. Почему printf в threads опасен для диагностики?
3. Минимальные флаги сборки для GDB?
4. Когда core полезнее живого сеанса?
5. Следующий том серии после отладки?

## Обсуждение с репетитором

**Том:** 6 · **Глава:** `ch14` · **Файл:** `ch14-summary-myths.md`

**Конец тома 6.** Дальше по серии: **том 7** — санитайзеры ASan, UBSan, TSan (`books/vol-07-sanitizers`, по плану в `BOOK-CONTEXT.md`).
