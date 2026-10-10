# Глава 9. False sharing и кэш-линии

## Зачем эта тема

Два потока могут **не делить** один mutex и **не писать** в одну переменную — и всё равно мешать друг другу. Если их atomics или счётчики лежат в **одной кэш-линии**, каждое обновление заставляет другие ядра инвалидировать кэш. Программа выглядит «lock-free и чистая», а масштабируется как будто под одним замком. Это **false sharing**: конфликт не из-за логики данных, а из-за физической упаковки в памяти.

После [lock-free обзора](./ch08-lock-free.md) и atomics полезно понять **железный** слой: cache line, alignment, padding — иначе оптимизация «разнесём счётчики по потокам» не сработает.

## Кэш-линия в двух словах

CPU читает память **блоками** (часто 64 байта на x86-64; на других платформах может быть иначе — смотрите документацию, но порядок величины тот же). Когда поток пишет один байт, когерентность протокола (MESI и родственники) помечает **всю линию** изменённой для других ядер. Соседний поток, который трогает **другое** поле в той же линии, получает miss и повторную загрузку — даже если «логически» переменные независимы.

False sharing **не** data race в смысле C++: два `atomic<int>` на разных логических счётчиках — формально разные объекты. Это **performance bug**, не UB — но для пула и parallel map это может съесть весь выигрыш от потоков.

## Типичный сценарий

Массив «счётчик на поток»:

```cpp
struct Stats {
    std::atomic<int> hits;
    std::atomic<int> misses;
};
Stats table[8];  // 8 воркеров, каждый пишет в свой index
```

Если `sizeof(Stats)` меньше 64 и объекты идут подряд, несколько `Stats` попадают в одну линию. Воркер 0 бьёт в `table[0].hits`, воркер 1 в `table[1].hits` — линия скачет между ядрами, throughput падает.

Похожая картина: глобальные `atomic` рядом в `.bss`, поля структуры «плотно упакованные» компилятором, или `std::vector<atomic<int>>` без padding между элементами.

## Padding и alignas

Идея: **разнести** горячие записи так, чтобы каждая жила на **своей** линии. Приёмы:

- `alignas(64)` на структуре счётчика (64 — типичная линия x86; для переносимости иногда берут 128 или читают `std::hardware_destructive_interference_size` **(C++17)**, если реализация задаёт ненулевое значение).
- Внутри struct: `char pad[64 - sizeof(atomic<int>)]` — грубо, но наглядно в учебнике; в production предпочтительнее interference size.
- Отдельный vector, где каждый элемент — `alignas(64) Counter`.

```cpp
struct alignas(64) PaddedCounter {
    std::atomic<std::uint64_t> value{0};
};
// хвост struct добивает до размера линии — не трогайте pad без нужды
```

**Over-padding** увеличивает RAM: для восьми счётчиков это килобайты — обычно OK; для миллиона «микро-счётчиков» — уже архитектурное решение.

## Atomic и обычные поля на одной линии

Mutex + `int busy` рядом с `atomic stopped` в одной cache line — mutex редко hot, но `busy` могут часто `--`/`++` воркеры. Если `busy` и очередь метаданных сидят рядом с полем, которое другой поток constantly load/store, получите скрытый contention. На этапе **дизайна** полей пула (глава 10) полезно мысленно разнести «часто пишут разные потоки» на разные линии.

Read-mostly данные рядом с write-heavy atomic — тоже страдают: читатели промахиваются из-за чужих записей.

Hot counters лучше выносить в отдельные line-aligned структуры; cold metadata можно держать рядом — компилятор сам не разнесёт поля «для скорости» без `alignas` или ручного pad.

## Как заметить в профиле

На Fedora `perf stat -e cache-misses,cycles` на stress-тесте до/после padding — грубая проверка. `perf c2c` (если доступен) специально ищет false sharing. Время растёт с числом потоков, а lock contention в профиле низкий — подозрение на линии.

Микробенчмарк: два потока, два atomic в одной struct vs две padded struct — разница в разы на большом числе итераций.

## NUMA и следующий уровень (упоминание)

На машине с несколькими сокетами память «ближе» к одному NUMA-node. Даже с padding счётчик на «чужом» node дороже. `numactl --hardware` на Fedora server-class; для домашнего Ryzen обычно один node — false sharing всё равно актуален. Том не углубляется в NUMA policy; достаточно знать, что padding — necessary, not sufficient на huge systems.

## True sharing vs false sharing

**True sharing** — потоки **логически** делят одно поле (счётчик под mutex или один atomic) — contention неизбежен, нужен алгоритм (шarding, локальные аккумуляторы + редкая сводка). **False sharing** — поля разные, линия общая — лечится layout. Сначала отличите в профиле, потом лечите.

Локальные аккумуляторы в thread-local или per-thread padded counter с редким `fetch_add` в глобальный — компромисс для метрик.

## Связь с parallel map и пулами

Parallel map без lock на read-only input не страдает от sharing на данных, если каждый поток пишет в **свой** диапазон **out**. Если бы все писали в один `atomic` индекс выхода — true sharing. False sharing всплывает, когда «каждый пишет в свой slot», но slots близко в памяти без padding.

Пул: `busy` нескольких воркеров — не должны сидеть в одной линии с head/tail очереди, если head/tail hot.

## std::hardware_destructive_interference_size **(C++17)**

Стандарт даёт макрос размера «минимального расстояния, при котором два адреса не мешают друг другу в кэше». Если значение 0 — реализация не знает — fallback на 64 или эмпирику. На Fedora с GCC/Clang часто ненулевое; проверьте `std::cout << std::hardware_destructive_interference_size` в tiny программе.

Использование:

```cpp
struct alignas(std::hardware_destructive_interference_size) Counter {
    std::atomic<std::uint64_t> n{0};
};
```

Не путать с `hardware_constructive_interference_size` — «сколько байт имеет смысл класть рядом, чтобы **делить** prefetch»; для false sharing нужен **destructive** (разнести).

## Пример «до и после» в мысленном эксперименте

Восемь потоков, каждый делает миллион `fetch_add(1, relaxed)` в **свой** счётчик. Вариант A: `atomic<uint64_t> c[8]` плотно. Вариант B: массив `PaddedCounter`. На 6 ядрах / 12 SMT разница в wall time может быть кратной — не из-за atomic операции, а из-за линий. Это хороший домашний microbench без привязки к коду упражнений.

## Parallel map и разнесение записей

В `08_parallel_map` каждый поток пишет в **свой** диапазон `out` по индексам — логически нет true sharing на одном элементе. Если бы все писали в `out[0]` или в один `atomic` индекс, масштабирование исчезло бы. False sharing всё ещё возможен, если соседние `out[i]` и `out[i+1]` попадают в одну cache line и обновляются с разных ядер — редкий edge для dense output, но проверяемый microbench.

Чтение **read-only** `input` без lock безопасно **после** того, как данные не меняются concurrent — типичный контракт parallel map в репо.

## Padded counters в метриках пула

Счётчик `busy` и длина очереди могут обновляться часто с разных потоков. Если они сидят в одной struct без padding, профиль «странно плохой mutex» иногда на самом деле **линия**. На первом проходе не оптимизируй; при втором — `alignas` или `hardware_destructive_interference_size` **(C++17)**.

## § К собесу (углубление)

- Объясни **MESI** на уровне «одна линия — один владелец modified».
- Почему false sharing **не UB**, но **performance bug**?
- Когда **локальные** accumulators + редкий global merge лучше padded atomics?
- Чем **constructive** interference size отличается от **destructive**?

## Prefetch и ложная надежда

Компилятор и `_mm_prefetch` не отменяют MESI: если другой поток пишет в линию, prefetch не спасает от invalidation. Padding решает layout; prefetch — другая ось.

## Типичные заблуждения

«Atomic же без mutex — значит, масштабируется» — false sharing может серialize cache line. «64 байта alignas всегда переносимо» — на некоторых CPU линия 128; interference size **(C++17)** помогает, но может быть 0 (неизвестно). «Padding в `std::atomic` внутри `std::vector<std::atomic<int>>`» — vector не добавит pad между элементами сам.

При чтении disassembly hot loop обращай внимание на `lock` prefix у atomic на x86 — это не mutex, но линия всё равно участвует в протоколе когерентности. Padding уменьшает **межъядерные** invalidation, не убирает сам atomic RMW.


## Глубже: структура пула и layout полей

При проектировании `ThreadPool` или `BoundedQueue` мысленно разнеси: hot atomics/int (`busy`, counters), указатели на редко меняющиеся данные, cold config. Компилятор **уплотнит** struct без `alignas` — профилируй перед микроправками.

## Hyperthreading и false sharing

Два logical CPU на **одном** core делят кэш; false sharing между потоками, закреплёнными на sibling SMT, особенно болезненна. Affinity редко нужна в учебнике; знать — полезно при tuning.


## Мост к пулу и parallel map

Пул: поля `busy`, queue metadata. Map: output slots. Layout — часть performance story **после** корректности mutex/indices.

## Измерение на Fedora (идея эксперимента)

Создай два бинарника: (A) массив плотных atomics, (B) массив `alignas(64)` counters; одинаковое число потоков и итераций `fetch_add(relaxed)`. Сравни wall time и `perf stat cache-misses`. Разница без изменения логики — сигнал false sharing.

Не путай с «atomic медленнее int»: здесь оба atomic; меняется только layout.

## Практика на бумаге

Сколько байт в типичной cache line x86-64?

Зачем alignas вокруг одного atomic на поток?

## Краткое резюме

Кэш-линия — единица когерентности; независимые atomics в одной линии создают false sharing и портят масштабирование. Лечение — alignas, padding, разнесение hot полей, per-thread локальные счётчики. Отличайте от true sharing, где одно поле — узкое место алгоритма. Профилируйте, не угадывайте layout.

## Вопросы для самопроверки

1. Почему false sharing не является data race в C++?
2. Зачем `alignas(64)` вокруг одного atomic-счётчика на поток?
3. Чем true sharing отличается от false на примере «глобальный hits++»?
4. Почему `std::vector` из `std::atomic<int>` подряд может масштабироваться хуже массива padded counters?
5. Какой симптом в perf намекает на линии, а не на mutex?
6. Когда `hardware_destructive_interference_size == 0`, что делать с alignas?

## Обсуждение с репетитором

**Том:** 3 · **Глава:** `ch09` · **Файл:** `ch09-false-sharing.md`

**Дальше по курсу:** [Глава 10. Паттерн пула потоков (теория)](./ch10-thread-pool.md)
