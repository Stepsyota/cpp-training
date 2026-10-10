# Глава 13. Ограничения

## Зачем эта тема

Санитайзеры сильные, но **не всевидящие**. Переоценка ведёт к ложной уверенности после одного зелёного TSan-прогона. Честный список границ — часть инженерной зрелости.

## Не ловят или слабо

**Deadlock, livelock** — GDB, design.

**Логические ошибки** — неверный предикат cv, потеря задач при `close`, off-by-one в parallel map без гонки.

**Гонка не проигралась** на данном входе.

**Производительность и fair scheduling** — не sanitizer.

**Спецификация «должно быть 42»** — assert и тесты.

**Все UB** — UBSan subset; неформальное UB вне списка.

**Правильный mutex, неверный инвариант** под lock.

## False sense после green

Один прогон `check.sh` с TSan на Debug — **хорошая** привычка, не **доказательство**. Stress, разные `n_threads`, другой компилятор — усиление.

## Производительность кода

TSan-build нельзя сравнивать по wall time с Release для оптимизации пула.

## Security

ASan ловит многие memory bugs, не заменяет аудит, fuzzing, permissions.

## Связь с томом 8

ООП и дизайн (SOLID) — про структуру, не runtime sanitizer. Том 8 закрывает другой слой.

## Spurious wakeup

Not TSan — cv logic tom 3.

## Memory order bug example

Producer release, consumer acquire wrong — may pass tests — tom 3 ch7.

## TSan and lock-free

Course defers lock-free — TSan still applies if try CAS on shared wrong.

## Formal methods

Out of scope — sanitizer ≠ proof.

## Code coverage

Coverage + sanitizer complementary — neither complete.

## assert in Release

NDEBUG — assert off — sanitizer still runs.

## Heisenbugs

TSan makes some **more** reproducible — good.

## When to stop sanitizing

Before release benchmark — switch build tree — tom 5 Release.

## Типичные заблуждения

«TSan прошёл — mutex везде» — может не быть гонки, но логика сломана. «Sanitizer в prod binary» — обычно нет.

## Deadlock и livelock

Threads blocked forever — **no** TSan «DEADLOCK» banner. **GDB** all threads blocked on mutex — tom 6.

## Логика без race

**closed** queue discard jobs — wrong predicate — tests may fail, TSan silent.

**08** wrong partition — wrong answer, maybe no race.

## Недетерминизм гонок

One green TSan run **≠** proof. **Stress**, more threads, more iterations.

## Performance claims

Never benchmark TSan binary vs Release **-O2**.

## Security scope

ASan catches many memory bugs; not sandbox, not fuzz replacement.

## UBSan subset

Not all standard UB instrumented.

## Mutex + wrong invariant

All accesses locked — TSan clean — still wrong pop logic.

## Production

Sanitizer builds **not** shipped to users typically.

## Fuzz + sanitizer

Common combo in industry — out of course scope, mention only.

## tom 8

SOLID/design — separate layer from runtime tools.

## § К собесу

Limits list; stress; GDB deadlock; logic vs race.

<!-- vol7-pass5-ch13 -->

## Дополнительное закрепление

Пять вещей sanitizers не ловят — перечисли. Deadlock → GDB.

Stress **08** tom 3.

TSan binary не для bench.

## Краткое резюме

Sanitizers — memory, часть UB, data race. Не logic, не deadlock, не полнота. Тесты + review + GDB остаются. Дальше — итог.

## Checklist «что ещё проверить»

Tests, review, TSan, ASan if pointers, GDB if hang, logic checklist tom 3 ch14, stress 08, second compiler.

## When sanitizer blocks merge

Fix real bug; don't disable sanitizer job to green-wash.

## Technical debt

Old suppress files — pay down or inherit pain.

<!-- vol7-pass6 -->
## Строка для памяти (Kindle)

Перед сном одним взглядом: **compile+link**, **-g -O1**, **TSan+pthread**, **два build dir**, **два стека в отчёте**, **solutions не спойлерить**. Завтра — один TSan-прогон на 02 или на последнем изменённом advanced target.

## Вопросы для самопроверки

1. Кто ловит deadlock?
2. Пример логической ошибки без TSan?
3. Почему stress для 08?
4. Заменяют ли sanitizers тесты?
5. Можно ли бенчмаркить TSan binary?

<!-- vol7-pass4-ch13 -->
## Зрелость инженера

Знать **границы** sanitizers важнее, чем знать все `ASAN_OPTIONS`. Deadlock, logic, spec — **тесты + review + GDB**. TSan — один столп, не храм.

Stress для **08** (tom 3): sanitizer + странный результат = hint на partition bug **без** race — tom 7 не противоречит tom 3.

После tom 7 не говори «я доказал корректность» — говори «я прогнал TSan/ASan на risk, tests green, review done».

## Обсуждение с репетитором

**Том:** 7 · **Глава:** `ch13` · **Файл:** `ch13-limits.md`

**Дальше по курсу:** [Глава 14. Резюме и мифы](./ch14-summary-myths.md)
