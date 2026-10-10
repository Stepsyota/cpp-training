# Глава 13. Оптимизированный код

## Зачем эта тема

Компилятор на **`-O2`** и **`-O3`** не «ломает» корректную программу в смысле стандарта C++: если исходник без UB и гонок, результат Debug и Release **должен** совпадать по observable behavior. На практике учебный код иногда **содержит** гонку или UB — тогда Release «ведёт себя иначе» чаще, потому что timing и layout памяти другие, а не потому что «оптимизатор злой». GDB на Release добавляет вторую боль: **optimized out**, inline, странные номера строк.

Эта глава учит не бояться сообщения GDB и не объявлять Release «неотлаживаемым». Есть **RelWithDebInfo**, **`-Og`**, временный **Debug** для расследования — и отдельно sanitizer-сборка из tom 7, когда подозрение на UB.

Тест зелёный в Debug, падает или врёт в Release — или отладчик в Release говорит **«variable optimized out»** и прыгает по странным строкам. Компилятор с **`-O2`** убирает лишние переменные, встраивает функции, переставляет инструкции **в рамках правил стандарта**. При UB (гонка) Release «ломается» чаще.

## Ментальная модель: переменная «живёт в регистре»

На **`-O2`** компилятор держит значение в регистре и не создаёт stack slot для имени из исходника. GDB честно говорит **optimized out** — не «переменная исчезла из программы», а «имени нет в debug info в этой точке». Иногда **`print` другого** выражения, зависящего от неё, всё ещё работает — компилятор сохранил промежуточный результат под другим dbg именем.

## optimized out

```
(gdb) print i
$i = <optimized out>
```

Переменной нет в DWARF — используй **`print` других** живых vars, **`info registers`**, или собери **`-Og`** (GCC: optimize с debuggability) для промежуточного режима:

```bash
g++ -std=c++17 -g -Og -pthread ...
```

CMake: кастомный `RelWithDebInfo` или флаг на один target.

## Inline и step

`step` в вызов `submit` может **не зайти** в тело — функция inline в caller. Смотри disasm или временно `-fno-inline` **только для отладки** (не для сдачи).

## Нестабильный баг только на -O2

Подозрение на **UB**: data race, uninitialized, signed overflow. GDB в Release не докажет — TSan с `-O1` (том 7). Логика: сначала Debug + sanitizer, потом Release.

## Debug как «источник истины» для инвариантов

Пока не доказал, что баг UB-only, веди расследование на Debug: инварианты очереди (**`empty`**, **`closed`**, **`cap`**) читаются **`print`** без optimized out. Когда инвариант нарушен на Debug — это **логика**, не `-O2`. Когда только Release — sanitizer и внимательный memory model (tom 3, tom 7).

## parallel_map и оптимизация

Корректный код без гонки ведёт себя одинаково по **результату**; timing другой. Неверный разрез диапазона — ошибка логики, не `-O2`.

## Отладка «как в production»

Иногда нужен Release с `-g` (**RelWithDebInfo**): символы есть, код быстрый. Breakpoints работают, locals — частично. Прими ограничения.

## Типичные заблуждения

«-g отключает оптимизацию» — нет, только если не задан `-O`. «Release баг = компилятор виноват» — чаще UB. «Og медленный как O0» — обычно между.

## Краткое резюме

Release усложняет GDB; `-Og` или Debug для расследования. optimized out — норма на `-O2`. Нестабильность → sanitizer. Дальше — итог.

## -Og на Fedora GCC

```bash
g++ -std=c++17 -g -Og -pthread ...
```

Компромисс для «почти Release» с отладкой. Не путай с **`-O0`**.

## -fno-optimize-sibling-calls

Редкий флаг для bt; курс не требует.

## RelWithDebInfo в CMake

**`-O2 -g -DNDEBUG`** типично — assert off, optimized locals partial.

## parallel_map и O2

Неверный результат на **O0 и O2** — логика. Только O2 — подозрение UB/race.

## disassemble / stepi

**`disassemble /m`** — mixed source/asm. Когда **`list`** врёт.

## Почему «компилятор сломал» — red flag

Чаще UB. TSan/UBSan. GDB на Debug воспроизводит логику.

## § К собесу

optimized out → build Debug or -Og; inline → use disassemble or disable inline for debug session only.

## LTO + debug

Link-time optimize further scrambles debug — tom 5 mentioned; avoid LTO when learning gdb.

## volatile and debug

**`volatile`** variables harder to optimize away — not license for sync.

## sanitizer builds debug-ish

**`-O1 -g`** TSan build debuggable enough for combined session (tom 7).

## breakpoint skew

Line 42 break hits line 38 — optimization; use **`break *addr`** advanced or Debug.

## teaching point

Compiler is not enemy; contract of language + UB is.

## compare Debug Release same input

Same test data, two builds — isolate logic vs UB.

## __attribute__((noinline))

Debug hack in copy branch only — never in submit.

## volatile optimized

Still not sync primitive.

## Debug default policy

Course: Debug until logic proven, then Release **`check.sh`**.

## profile guided

Out of scope — tom 5 mentioned lightly.

## Дополнительный сценарий: RelWithDebInfo

Сконфигурируй отдельный **`build-relwithdebinfo`** (том 5), собери **`08`**, поставь breakpoint на hot loop. Сравни **`info locals`** с Debug. Ощути **`optimized out`** на живом примере — запоминается лучше, чем абзац.

## Release check.sh после Debug fix

Правило курса: fix проверен Debug + обычный run + **`check.sh`** на том же build type, что CI хозяина. Если CI Release — финально Release.

## Практическое закрепление (5–10 минут)

Открой Debug-сборку любого **зелёного** target из `multithread/build/`. Запусти GDB, поставь **`break main`**, **`run`**, выполни **`next`** пять раз, **`bt`**, **`info locals`**, **`quit`**. Повтори на другом target — muscle memory важнее заучивания глав. Если что-то не совпало с текстом главы, проверь cwd и путь к exe: nine из десяти «GDB сломан» — неверный путь или сборка без **`-g`**.

## Связь с главой тома 3 без спойлера кода

Вернись мысленно к инварианту упражнения, которое отлаживаешь: что должно быть истинно перед выходом из **`wait`**, когда очередь считается пустой, когда **`submit`** должен молчать. GDB показывает **момент**, когда инвариант уже нарушен или ещё не достигнут. Исправление остаётся в голове и в **`task.cpp`**, не в команде **`continue`**.

## § Коротко к собесу (повтор)

Сформулируй вслух одним предложением главную мысль этой главы и назови **одну** команду GDB, без которой тема не работает. Если не можешь — перечитай «Краткое резюме» и вернись сюда завтра; spaced repetition лучше зубрёжки за вечер.

## Вопросы для самопроверки

1. Что значит optimized out?
2. Зачем `-Og`?
3. Почему step не входит в inline?
4. Первый шаг при баге только в Release?
5. Что даёт RelWithDebInfo?

## Обсуждение с репетитором

**Том:** 6 · **Глава:** `ch13` · **Файл:** `ch13-optimized-code.md`

**Дальше по курсу:** [Глава 14. Резюме и мифы](./ch14-summary-myths.md)
