# Глава 12. LeakSanitizer

## Зачем эта тема

Утечка памяти — не data race, но баг. При **ASan** на Linux **LeakSanitizer (LSan)** часто включён по умолчанию: при exit печатает **leaked** блоки и стек выделения.

## Как увидеть

Собери с ASan, запусти до нормального exit:

```bash
g++ -std=c++17 -g -O1 -fsanitize=address leak.cpp -o leak && ./leak
```

В конце:

```
SUMMARY: AddressSanitizer: 123 byte(s) leaked in 1 allocation(s)
```

Со стеком `malloc` / `operator new`.

## Отключить leaks

`ASAN_OPTIONS=detect_leaks=0` — если отлаживаешь только overflow и leaks мешают (временно).

## Утечки и потоки

Память, не освобождённая до exit процесса, LSan видит. **Потерянный** указатель в работающем сервере без exit — LSan не поможет до shutdown.

## multithread

Учебные exe короткоживущие; утечка в `new` без `delete` в тесте — LSan. Пул с корректным drain обычно чист — если нет, ищи забытый `shared_ptr` цикл (редко в курсе).

## Отличие от Valgrind leak check

Похожая идея; ASan+LSan быстрее на больших программах при пересборке.

## still reachable

LSan may classify **still reachable** at exit — global singletons — different severity.

## fork without exec

Leak check tricky — not exercises.

## shared_ptr leak

Refcount never zero — leak report with alloc stack.

## pool on shutdown

Workers joined, jobs done — expect clean LSan; if not, job heap lifetime wrong.

## disable in CI

Some ASan CI disable leak for speed — explicit choice.

## new in test main only

Leak in test harness — fix test or accept with comment — rare.

## compare TSan run

Same binary TSan — leak section may differ — focus ASan build for leaks.

## Типичные заблуждения

«TSan найдёт leak» — не его задача. «Leak не важен» — в долгоживущих процессах важен. «LSan без ASan» — отдельный режим, на курсе через ASan.

## LSan bundled with ASan on glibc Linux

`-fsanitize=address` → at **process exit**, leak scan often runs. Message:

`SUMMARY: AddressSanitizer: N byte(s) leaked in M allocation(s)`.

## Stack of allocation

Shows **where** `new`/`malloc` without matching free. Fix: RAII, smart pointers (tom 8 preview).

## detect_leaks=0

`ASAN_OPTIONS=detect_leaks=0` — focus on overflow only **temporarily**.

## Leaks vs race

**Leak** — resource bug. **Race** — TSan. Orthogonal.

## Long-running server

No exit → no LSan summary until shutdown. Different tools (heap profiling).

## multithread exercises

Short-lived test binaries — leak = forgot delete in error path. Pool drain — if leak report, check job ownership.

## Valgrind leak check

Similar goal; ASan+LSan faster **after** rebuild.

## intentional leak in test

Some tests leak on purpose — rare in training repo.

## § К собесу

LSan with ASan on Linux; exit-time; detect_leaks=0; not TSan job.

<!-- vol7-pass5-ch12 -->

## Дополнительное закрепление

LSan at exit on short tests. detect_leaks=0 временно.

Leaks — build-asan; races — build-tsan.

RAII tom 8 снижает leaks.

## Краткое резюме

ASan на Linux → часто leak report на exit. Стек allocation. `detect_leaks=0` отключить. Дальше — ограничения.

## RAII preview

Tom 8 Rule of Zero — fewer leaks by design. LSan still catches forgotten `new` in transitional code.

## leak sanitizer standalone

`-fsanitize=leak` exists without full ASan on some setups — course uses ASan bundle on Linux.

## multithread global singletons

Rare static — still reachable at exit — know category exists.

<!-- vol7-pass6 -->
## Строка для памяти (Kindle)

Перед сном одним взглядом: **compile+link**, **-g -O1**, **TSan+pthread**, **два build dir**, **два стека в отчёте**, **solutions не спойлерить**. Завтра — один TSan-прогон на 02 или на последнем изменённом advanced target.

## Вопросы для самопроверки

1. Какой sanitizer включает LSan по умолчанию?
2. Когда виден отчёт leak?
3. Как отключить leaks временно?
4. Ловит ли LSan leak в бесконечном сервере без exit?
5. Связь leak с `new` в task?

<!-- vol7-pass4-ch12 -->
## LSan как мягкое напоминание

Leak на exit учебного exe — **не** «мелочь»: приучает к RAII до tom 8. `detect_leaks=0` — только когда overflow отлаживаешь и leak report мешает **временно**.

TSan build и leak report — не главный путь; **build-asan** для leaks + memory bugs together.

В pool theory (tom 3): jobs должны **завершаться**; leak может mean job heap не освобождён — logic + LSan, не TSan.

## Обсуждение с репетитором

**Том:** 7 · **Глава:** `ch12` · **Файл:** `ch12-leaks-lsan.md`

**Дальше по курсу:** [Глава 13. Ограничения](./ch13-limits.md)
