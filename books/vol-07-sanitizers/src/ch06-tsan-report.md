# Глава 6. Отчёт TSan

## Зачем эта тема

Отчёт TSan построен вокруг **двух (или более) стеков**: кто **писал**, кто **читал/писал** без синхронизации. Это карта для исправления — добавить mutex, atomic, или перенести доступ в один поток.

## Типичный фрагмент

```
WARNING: ThreadSanitizer: data race (pid=...)
  Write of size 4 at 0x... by thread T2:
    #0 increment at task.cpp:12
    #1 ...
  Previous read of size 4 at 0x... by thread T1:
    #0 increment at task.cpp:12
```

Один и тот же файл:строка — оба потока в **одной** операции `++` на shared `int`. Исправление — `std::mutex` + `lock_guard` или `atomic`.

## Location и variable

TSan печатает **адрес** и иногда **имя** глобальной. Локальные на стеке друг друга не «гонят» — только если два потока видят **один** объект (reference, pointer, heap).

## Happens-before в отчёте

Иногда TSan пишет про **mutex** или **atomic** — «synchronized». Если ты **думаешь**, что sync есть, а TSan нет — проверь, тот ли объект, не два разных счётчика, не race на **соседнем** поле (false sharing не всегда race в смысле TSan на разных адресах).

## Suppress vs fix

Первый инстинкт — **исправить код**, не глушить (глава 11).

## После fix

Пересобери **полностью** с TSan (`ninja clean` или новый build dir). Старый `.o` без sanitizer в смешанном build — ложное чувство безопасности.

## GDB

При `abort` от TSan — `gdb ./t`, `run`, на падении `bt`. Или воспроизвести под gdb с TSan (медленно).

## Location line

`Location is global 'counter'` vs heap block — tells you **which** symbol.

## Merged reports

Sometimes one race reported once per interleaving — rerunning may differ.

## Compare to tom 3 definition

Say aloud: «two accesses, one write, no hb» — match report to definition for interview.

## Races in constructors

Two threads touch object mid-construct — UB + TSan — rare in exercises.

## const race

`const int&` to shared mutable through cast — still race on underlying — don't cheat const.

## Report to code change map

Write → need mutex or atomic write side. Read+write → sync both paths.

## Timing

TSan slows — races **more** likely to show — ironically helpful.

## Copy report for ch07 discussion

Tutor: «which exercise pattern is this?» without opening solutions.

## Типичные заблуждения

«Один стек = один виновник» — нужны оба. «Race в libc» — часто вызвано твоим вызовом выше. «Игнорировать warning» — UB остаётся.

## Анатомия WARNING

Строка `WARNING: ThreadSanitizer: data race (pid=...)`. Затем **Write** / **Read** of size at address. **Thread T2** vs **T1** — два стека.

Оба на **одной** строке `task.cpp` — оба в `operator++` на shared ref — fix sync.

## Mutex в отчёте

Иногда TSan пишет, что доступ **synchronized via mutex M**. Если ты **ожидал** sync, но race есть — wrong mutex, wrong object, или lock **не** покрывает поле.

## Global vs heap vs stack

**Global** `int g` — оба потока — race. **Stack**: два потока не race **разные** locals; race если **pointer** к одному heap object. **Reference** to shared outer variable — same address.

## Happens-before словами

**Thread A** `unlock(mutex)` **happens-before** **Thread B** `lock(same)` — accesses under lock ordered. **Release/acquire** on atomic — tom 3; TSan понимает pthread and C++ sync lib.

## False sharing

Two atomics on same cache line — **different addresses** — **not** data race if different objects. TSan reports **same address**. **False sharing** — perf (tom 3 ch9), not TSan.

## Suppress temptation

First fix: **mutex/atomic**. Suppress — ch11, last resort.

## Clean rebuild

`ninja clean` in build-tsan or rm build-tsan. Mixed `.o` — silent wrong.

## GDB under TSan

Painfully slow. Use report stacks first; GDB for hang/deadlock.

## § К собесу

Two stacks; same address; fix sync; clean rebuild; not one stack only.

<!-- vol7-pass5-ch06 -->

## Дополнительное закрепление

Два стека = два участника timeline. Clean rebuild после fix.

Сравни с **tom 3 ch14** preview.

Race на одной строке — оба потока в одной операции на shared.

## Краткое резюме

Два стека, один адрес, conflicting access. Правь синхронизацию. Clean rebuild. Дальше — упражнения.

## Упражнение: прочитать вслух

Взять гипотетический report из главы 6, прочитать: «Thread T2 write, Thread T1 read, same address, data race». Связать с определением tom 3.

## Main vs worker stacks

Report labels T1, T2 — map to `main` and worker lambda in pool **conceptually** without opening solutions.

## No race report but wrong count

Logic or test insufficient — not TSan scope — tom 3 stress.

<!-- vol7-pass6 -->
## Строка для памяти (Kindle)

Перед сном одним взглядом: **compile+link**, **-g -O1**, **TSan+pthread**, **два build dir**, **два стека в отчёте**, **solutions не спойлерить**. Завтра — один TSan-прогон на 02 или на последнем изменённом advanced target.

## Вопросы для самопроверки

1. Сколько стеков смотреть в типичном race?
2. Почему оба потока на одной строке?
3. Что делать после исправления task.cpp?
4. TSan говорит про mutex — что это значит?
5. Как связать отчёт с томом 3?

<!-- vol7-pass4-ch06 -->
## Два стека — две истории

Первый стек отвечает «кто второй участник конфликта сейчас»; второй — «кто был раньше без sync». В голове рисуй **timeline**, не «один виноват».

После fix **не** коммить, пока clean rebuild TSan не зелёный на **нескольких** прогонах для flaky race.

С tom 6: если TSan abort в незнакомом месте — `bt` в GDB; но **первичный** текст всё равно из TSan report — GDB дополняет.

## Обсуждение с репетитором

**Том:** 7 · **Глава:** `ch06` · **Файл:** `ch06-tsan-report.md`

**Дальше по курсу:** [Глава 7. Упражнения multithread](./ch07-multithread-exercises.md)
