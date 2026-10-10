# Глава 3. future и shared_future

## Зачем эта тема

`std::future<T>` — ручка читателя на одно значение типа `T` (или void, или исключение). Пока ты не понял `wait`, `get` и правило «один раз забрал — future пуст», легко словить блокировку навечно, двойной `get` или гонку при попытке «раздать результат нескольким потребителям». `std::shared_future` решает последний случай ценой другой семантики.

## Состояния future

У `std::future` нет публичного «готов / не готов» в C++17 как простого `bool` без ожидания — обычно проверяют через `wait_for` с нулевым таймаутом или просто ждут. Концептуально future проходит путь: **invalid** (по умолчанию или после move) → **shared state привязан** → после `get()` снова **нет значения** (future consumed).

Move-only семантика: `std::future` нельзя копировать, только перемещать. Это гарантирует **единственного** читателя, который заберёт значение. Если нужно раздать — смотри `shared_future`.

## get, wait, wait_for, wait_until

```cpp
std::future<int> fut = std::async(std::launch::async, compute);
fut.wait();           // блок до готовности
int x = fut.get();    // блок + забрать значение; повторный get — ошибка
```

`get()` для `T` возвращает копию или move из shared state. Для `void` просто синхронизирует завершение. После успешного `get()` future **не** содержит значение — второй `get()` бросает `std::future_error`.

`wait_for` и `wait_until` возвращают `std::future_status`: `ready`, `timeout`, `deferred` (если задача отложенная). Удобно для UI или цикла опроса без вечного блока:

```cpp
if (fut.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
    process(fut.get());
}
```

Не путай «таймаут wait_for» с отменой работы: в C++17 работа может всё равно продолжаться в фоне, если она уже запущена.

## shared_future

```cpp
std::promise<int> p;
std::shared_future<int> sf = p.get_future().share();
```

`share()` переводит future в shared режим (или создаёт `shared_future` из promise). Несколько потоков могут вызвать `get()` на **копиях** одного `shared_future` и получить **одно и то же** значение. Копирование `shared_future` — как `shared_ptr` на общее состояние.

Когда это нужно: один producer положил ответ, несколько worker’ов ждут один и тот же конфиг или флаг старта. Когда **не** нужно: каждый потребитель должен забрать уникальный элемент — тогда очередь (том 3), а не shared_future.

## valid()

`fut.valid()` проверяет, привязан ли future к shared state. После `get()` обычно `false`. На invalid future вызов `get`/`wait` — `future_error`. Перед использованием после move проверяй, что ты не оставил пустую оболочку.

## Синхронизация и happens-before

Успешный `get()` или `wait()` **синхронизируется с** завершением задачи, которая записала результат. Это не отменяет гонок на данных, которые задача и main **одновременно** пишут без mutex. Future публикует **один слот**; остальное — твоя ответственность.

## Отличие от condition_variable

`condition_variable` + mutex описывает **произвольный инвариант** («очередь не пуста», «stopped»). Future — **узкий контракт** «одно значение T один раз». Для bounded queue future на каждый элемент — избыточно и дорого; для «ответ oracle» — идеально.

## Типичные заблуждения

«`wait` и `get` делают одно и то же» — `get` забирает значение и опустошает future. «`shared_future` быстрее нескольких future» — это про семантику чтения, не про производительность. «Можно копировать `future`» — нет.

<!-- expand:ch03-future-shared.md -->
## Развёрнутый разбор

`std::future<T>` — move-only handle читателя. Копирование запрещено, чтобы **один** поток забрал значение. Это отличает future от многих других типов STL и от `shared_future`.

После move `std::move(fut)` старый future пуст: `valid()` false. В контейнерах `vector<future<int>>` при emplace_back и последующем move следи, какой элемент уже опустошён — use-after-move в цикле типичная ошибка новичка.

### wait и get для void

Для `future<void>` `wait()` ждёт завершения без «значения». `get()` тоже ждёт и **потребляет** future; при исключении в задаче — rethrow. Паттерн «только дождаться» — `wait` или `get` для void; разница в том, что `get` для void всё равно помечает future использованным.

### wait_for и polling

Цикл с `wait_for(0ms)` или коротким timeout — способ не блокировать UI или worker навечно. Это **не отмена** задачи: фоновый `async` с `launch::async` продолжает крутиться. Отмена в C++17 — отдельный протокол (флаг, закрытие сокета, **(C++20)** stop_token в других API).

`future_status::deferred` напоминает: задача может ещё не стартовать. Не трактуй timeout как «ошибку сети» без контекста политики запуска.

### shared_future семантика

`share()` или конструирование `shared_future` из `future` переводит handle в режим нескольких читателей. Каждый `get()` возвращает **копию** `T` (для `T` copyable) или move для rvalue — все видят одно и то же опубликованное значение. Исключение при `get()` на shared_future получают все читатели одинаково.

Use case: один раз загрузили конфиг, несколько worker’ов вызывают `sf.get()` и стартуют с одной версии. Anti-case: очередь задач — каждому нужен **свой** элемент; это `BoundedQueue`, не shared_future.

### Связь с condition_variable

CV + mutex: «жди, пока pred()». Future: «жди, пока положат **одно** T». Узкий контракт future проще рассуждать, но не заменяет очередь. В пуле (том 3) cv будит worker’ов при появлении job; future на каждую мелкую job — overhead.

### happens-before

Стандарт гарантирует синхронизацию между публикацией результата в shared state и успешным `get`/`wait`. Это **не** распространяется на произвольные данные, которые worker писал в глобальные без mutex параллельно с публикацией. Oracle обычно не делит состояние между потоками кроме канала результата — идеальный учебный случай.

### К собесу

Почему future move-only. Когда shared_future. Что после второго get на обычном future.

<!-- expand2 -->
## Углубление: future

Пункт **1** (future). Стандарт C++17 в репозитории `training` — база упражнений; C++20 coroutines помечены отдельно, чтобы не смешивать «как будто всё было в 17».
Пункт **2** (future). Конспект `multithread/THEORY.md`, §6, разбирает `async`, `future`, `promise` и сценарии, когда async не подходит. Перечитывай §6 после глав 2–4.
Пункт **3** (future). Упражнение `multithread/beginner/03_oracle/` требует другой поток и `.get()`; README рекомендует `std::launch::async`. Книга не подставляет код из `multithread/solutions/`.
Пункт **4** (future). Том 3 даёт mutex, condition_variable, atomics и идеи пула/очереди; том 4 не отменяет синхронизацию общих данных — future защищает только канал одного результата.
Пункт **5** (future). Том 2 про лямбды: capture `[&]` в фоне опасен, init-capture переносит копию; это критично для `async` и `packaged_task`.
Пункт **6** (future). На Fedora: `g++ -std=c++17 -pthread`, mdBook для EPUB — `books/vol-04-async`, команда `mdbook build` (том 0).
Пункт **7** (future). Деструктор `std::future` от `async` может блокировать — не fire-and-forget без плана (глава 7).
Пункт **8** (future). `std::launch::deferred` выполняет work на потоке, который вызвал `get` — не путать с фоновым параллелизмом.
Пункт **9** (future). `broken_promise` — promise уничтожен без set; `future_error` на второй `get`.
Пункт **10** (future). Исключение из worker в `async` всплывает в `get()` вызывающего — ловить там (глава 6).
Пункт **11** (future). Пул ограничивает потоки; лес `async` в цикле — red flag на собесе и в профиле.
Пункт **12** (future). Packaged_task + пул — когда нужен future и контроль момента запуска; oracle — когда хватает async.
Пункт **13** (future). Promise связывает void-колбэк библиотеки с blocking `get()` в main.
Пункт **14** (future). Executor — «куда submit»; ручной пул из tom 3 — executor без std::execution в C++17.
Пункт **15** (future). Asio `io_context` + strand — модель потоков для I/O handlers (глава 10, 13).
Пункт **16** (future). Coroutines: `promise_type` coroutine ≠ `std::promise`; имена совпадают, сущности разные (глава 11).
Пункт **17** (future). `co_await` приостанавливает coroutine, не обязательно освобождая поток — зависит от awaitable.
Пункт **18** (future). Блокирующий read в thread + future прост для клиента; websocket-сервер на чистом async — нет.
Пункт **19** (future). TSan (том 7) ловит гонки, которые future не предотвращает на ваших структурах.
Пункт **20** (future). Gdb (том 6): все потоки, backtrace worker, breakpoint на `get`.
Пункт **21** (future). К собесу: shared state, wait vs get, shared_future, когда promise вместо async.
Пункт **22** (future). Миф: «future thread-safe для всего приложения» — только протокол shared state future.
Пункт **23** (future). Миф: «coroutine = thread» — many coroutines on one thread возможны.
Пункт **24** (future). Дальше: том 5 toolchain, 6 gdb, 7 sanitizers, 8 OOP/SOLID.

<!-- expand3 -->
### valid() и пустой future

Default-constructed future не привязан — `valid()` false. После move-from — тоже. Проверка перед `get` в generic коде спасает от `future_error`.

### shared_future и const

Несколько потоков могут держать копии `shared_future` и звать `get` параллельно — это поддержано. Но если после get вы модифицируете **локальную копию** T без синхронизации между потоками — это уже ваши данные, не future.

### timed wait и UI

Цикл: `while (running) { if (f.wait_for(16ms) == ready) update(f.get()); draw(); }` — типичная схема без coroutines. Не путать с 60 FPS гарантией: `get` может быть тяжёлым.

### Связь с oracle

Oracle блокирует до результата — `get` без wait_for. Для pipeline с несколькими oracle-подобными вызовами помни порядок launch/get (глава 2).

<!-- expand4 -->
## Практика и собес: future

1. wait vs get для int.
2. wait vs get для void.
3. Второй get — какое исключение.
4. shared_future: два потока, один конфиг.
5. Когда очередь лучше shared_future.
6. wait_for(0) сценарий polling.
7. valid() после move.
8. deferred и future_status.
9. Oracle: один get на один результат.
10. happens-before не для произвольных globals.

<!-- expand5 -->
## Стандарт, ревью, серия (ch03 future shared)

- Стандарт описывает **поведение** API, не конкретный пул потоков libstdc++. На собесе различай «что гарантирует стандарт» и «что делает GCC 14 на Fedora».
- Документация cppreference для `std::async`, `std::future`, `std::promise` — справочник; этот том — narrative поверх §6 `THEORY.md` и oracle.
- При ревью кода ищи: сохранён ли future от async; явная ли политика; не `[&]` на stack; promise set ровно один раз; join thread после ручного worker.
- Упражнение **03_oracle** проверяет контракт потоков; не путай с **02_ticket_office** (mutex) — разные навыки, один трек multithread.
- Если в проекте C++17-only, главы 11–13 помечай как forward-looking; coroutines не обязаны быть в production коде репозитория training.
- Связка **future + UI** часто через message queue или `post` на executor, не прямой touch виджетов из worker — глава 9.
- **Packaged_task** в industrial коде часто спрятан внутри thread pool library; ты можешь пользоваться пулом, не видя тип напрямую.
- **Parallel STL** `par` не возвращает future на элемент; tom 3 — не смешивай ментально с oracle.
- Ошибки concurrency отлаживай слоями: воспроизведи, gdb threads, TSan, потом читай стандарт — tom 6–7.
- Книжный стиль tom 1: абзацы короткие для Kindle, но глава плотная по смыслу — не outline из одной строки на §.
- Идентификаторы в коде английские; prose русский — как в `BOOK-CONTEXT.md`.
- Не используй `vector<int>` в prose вне code block — mdBook HTML; в коде — норм.
- META.md тома 4 фиксирует оглавление; после раздувания обновим строки и EXPANSION-STATUS.
- Репетитор: строка «Том 4 · Глава ch0K» в конце каждой главы для чата.
- Дальше по курсу — ссылка на следующую главу; tom 5 после ch14.
- Broken promise и double get — типичные вопросы на junior/middle собесе по C++.
- Launch deferred на собесе: «выполнится на том, кто ждёт» — ключевая фраза.
- Shared future — «один результат, много читателей»; не путать с broadcast сообщений разного содержания.
- Executor слово в резюме без контекста — уточни: ручной пул, Asio, или P2300.
- Coroutine interview: suspend не equals async I/O unless awaitable says so.

## Краткое резюме

`future` — один читатель, move-only, `get()` один раз забирает `T`. `wait*` — блок без изъятия. `shared_future` — много читателей одного значения. Таймауты не отменяют фоновую работу в C++17. Дальше — ручной `promise`.

## Вопросы для самопроверки

1. Что произойдёт при втором `get()` на том же `future`?
2. Когда нужен `shared_future`, а не очередь?
3. Чем `wait()` отличается от `get()` для `future<void>`?
4. Может ли `wait_for(0)` вернуть `ready` для `deferred` задачи до первого `get`?
5. Остаётся ли future valid после `get()`?

## Обсуждение с репетитором

**Том:** 4 · **Глава:** `ch03` · **Файл:** `ch03-future-shared.md`

**Дальше по курсу:** [Глава 4. promise вручную](./ch04-promise.md)
