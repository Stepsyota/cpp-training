# Глава 14. Резюме и мифы

## Зачем эта тема

Том 7 замыкает «инструментальный» блок серии (5–7): флаги → GDB → sanitizers. Финал — карта, мифы, чеклист с томом 3, мост к **тому 8** (ООП/SOLID).

## Карта тома

| Инструмент | Флаг (идея) | Ловит |
|------------|-------------|--------|
| ASan | `-fsanitize=address` | overflow, UAF, double-free |
| UBSan | `-fsanitize=undefined` | shift, overflow, null, часть casts |
| TSan | `-fsanitize=thread` + pthread | data race |
| LSan | с ASan на Linux | leaks at exit |

Всегда: **`-g`**, чаще **`-O1`**, флаги на **link**. TSan **отдельно** от ASan. Два build dir. Читать **два стека** в TSan. Прогон **02, 05, 06–08** на своём коде.

## Мифы

**«Зелёный тест = нет гонки»** — TSan или stress.

**«volatile для sync»** — tom 3; TSan на plain bool между потоками.

**«GDB заменяет TSan»** — нет.

**«ASan ловит race»** — не специализация.

**«Один sanitizer на всё»** — TSan отдельно.

**«Suppress в task»** — обман.

**«Sanitizer в Release»** — отдельная сборка.

**«Прошёл TSan раз — навсегда»** — недетерминизм.

## Чеклист (с томом 3)

Перед «готово» на multithread diff:

- Прогон TSan на затронутых targets (`build-tsan`).
- При работе с сырыми указателями / lifetime — ASan+UBSan (`build-asan`).
- Deadlock — `thread apply all bt` (том 6).
- Логика очереди/пула — review по чеклисту tom 3 ch14.

## К собесу

- Зачем sanitizer на compile **и** link?
- Разница ASan / TSan / UBSan одной фразой каждый.
- Как читать TSan report?
- Почему `-O1`?
- Можно ли ASan+TSan вместе?
- Что sanitizers **не** ловят?

## Дальше по серии

**Том 8** — ООП, SOLID, проектирование классов (финал серии 0–8). Инструменты готовы; дизайн — следующий слой.

Практика `multithread/` остаётся с репетитором; книга — перечитывание на Kindle.

## Карта тома (развёрнуто)

**ASan** — memory invalidity, often leaks at exit via LSan. **UBSan** — executed UB subset. **TSan** — data race. Always **-g**, usually **-O1**, flags on **link**. **TSan separate** from ASan. Read **two stacks**. Run **02, 05, 06–08** on **your** code.

## Мифы (развёрнуто)

**Green test ≠ no race** — TSan + stress.

**volatile for sync** — tom 3; TSan on plain bool.

**GDB replaces TSan** — complementary.

**ASan catches race** — not specialized.

**One sanitizer for all** — TSan isolated.

**Suppress in task** — cheating learning.

**Sanitizer in Release** — separate config.

**One TSan run forever** — nondeterminism.

**TSan proves full correctness** — false.

## Чеклист с tom 3

Diff review → check.sh → TSan affected targets → ASan if pointers → GDB if hang → queue/pool logic checklist.

## Toolchain block 5–6–7

Tom 5 flags & CMake → Tom 6 GDB → Tom 7 sanitize. Three **build roles** don't mix blindly.

## CI tie-in (tom 5)

**-Werror** compile job ≠ sanitizer run job. Nightly TSan optional. Owner controls `.gitlab-ci.yml`.

## Дальше tom 8

OOP, SOLID — design. Sanitizers stay in toolbox for lifetime bugs in classes.

## Практика с репетитором

Book on Kindle; exercises with tutor; no solutions paste.

## § К собесу (повтор)

Compile+link; three sanitizers one phrase each; two TSan stacks; no ASan+TSan; limits list.

## Закрытие серии инструментов

After tom 8 — series 0–8 read. Return to tom 7 when setup new machine or forget flag combo.

<!-- vol7-pass5-ch14 -->

## Дополнительное закрепление

Чеклист tom 3 + 7. Дальше **tom 8**.

Мифы — вычеркнуть из речи.

Instrument block 5–6–7 завершён.

## Краткое резюме

Sanitizers — обязательная привычка после tom 3–5–6. Два build, правильные флаги, честные ограничения. Конец тома 7.

## P2 pass: финальный чеклист перед tom 8

Перечитай таблицу карты тома. Прогони мысленно 02 и 06 на TSan. Вспомни два build dir. Закрой tom 7 — открой tom 8 introduction когда готов к SOLID.

## Повтор мифов коротко

Тест≠race. volatile≠sync. GDB≠TSan. ASan≠race detector. TSan≠logic. Suppress≠fix. Release≠sanitize build.

## Благодарность процессу

Ты прошёл multithread **сам**; sanitizer — способ **увидеть** то, что репетитор описывал словами. Книга на Kindle — чтобы не забыть флаги через полгода.

<!-- vol7-pass6 -->
## Строка для памяти (Kindle)

Перед сном одним взглядом: **compile+link**, **-g -O1**, **TSan+pthread**, **два build dir**, **два стека в отчёте**, **solutions не спойлерить**. Завтра — один TSan-прогон на 02 или на последнем изменённом advanced target.

## Вопросы для самопроверки

1. Три флага для TSan на g++?
2. Почему два каталога build?
3. Первый шаг при подозрении на гонку?
4. Кто ловит use-after-free?
5. Следующий том серии?

<!-- vol7-pass4-ch14 -->
## Конец инструментального блока 5–6–7

Tom 5 — **как собрать**. Tom 6 — **как остановить**. Tom 7 — **как поймать** UB/race классов. Tom 8 — **как спроектировать** классы (SOLID). Не пропускай tom 8, думая «я уже силён в threads» — ООП в серии **финал** по плану BOOK-CONTEXT.

Перечитай этой главой **мифы** перед любым «volunteer multithread help» другу: без TSan совет опасен.

Kindle: tom 7 удобно держать рядом с tom 3 при повторении memory order — разные тома, одна мотивация **корректности**.

## Обсуждение с репетитором

**Том:** 7 · **Глава:** `ch14` · **Файл:** `ch14-summary-myths.md`

**Конец тома 7.** Дальше по серии: **том 8** — ООП и SOLID (`books/vol-08-oop`, по плану в `BOOK-CONTEXT.md`).
