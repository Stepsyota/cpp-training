# Глава 7. Ловушки async и future

## Зачем эта тема

API `async`/`future` выглядит коротким, но именно здесь часто теряют часы: программа «иногда висит», тесты flaky, процесс создаёт сотни потоков. Большинство ловушек — не магия компилятора, а **контракты**, которые легко нарушить незаметно.

## Деструктор std::future

Если на `std::future`, возвращённом из `std::async`, **не вызвать** `get()` или `wait()`, деструктор future в конце полного выражения или области видимости **блокируется**, пока задача не завершится — для async-запуска это по сути скрытый join. «Fire-and-forget» через голый `async` без сохранения future **не** освобождает от ожидания: временный future всё равно уничтожится и может подождать.

Для отложенной политики `deferred` деструктор может **не** выполнять работу — поведение зависит от того, был ли вызван wait/get. Путаница deferred и async — классический источник «почему потоков не видно».

Практика: сохраняй future, если запускал работу; явно `get()` или отменяй дизайн (в C++17 отмены нет — не запускай лишнее).

## Политика по умолчанию

```cpp
auto fut = std::async([] { return work(); }); // политика не указана
```

Реализация может выбрать `async`, `deferred` или комбинацию. Код, который **требует** параллельного выполнения, обязан передать `std::launch::async`. Код, который **требует** отложенного — `deferred`. Для переносимости не оставляй default в критичных местах.

## Взрыв числа потоков

Каждый `async` с `launch::async` может породить новый поток. Цикл из тысячи итераций с `async` внутри — тысяча потоков или давление на runtime. Планировщик ОС захлебнётся; overhead съест выигрыш. Для многих мелких однотипных job — **пул** (том 3), не лес `async`.

Правило thumb: `async` для **редких** тяжёлых задач с чётким future на результат; пул для **потока** мелких callables.

## Блокировка на get в цепочке

```cpp
auto a = std::async(std::launch::async, step_a);
auto b = std::async(std::launch::async, step_b);
auto x = a.get(); // ждём a
auto y = b.get(); // b мог уже закончиться
```

Если `step_b` не зависит от `a`, оба запускаются параллельно; если ты сначала `get()` на `a`, а `b` ещё не запущен из-за порядка строк — теряешь параллелизм. Планируй: сначала **все** launch, потом **все** get (или используй `when_all` из библиотек — не в C++17 STL).

## Lifetime и ссылки

Лямбда в `async` с `[&]` на локальные переменные функции: если `async` реально ушёл в другой поток, а функция вернулась до `get()` — UB. Init-capture и копии — безопаснее. То же для `packaged_task` и promise, переданных в thread.

## valid() и use-after-move

После `std::move(fut)` в другой объект старый future пуст. Вызов `get()` на moved-from — ошибка. В векторе future следи за индексами после move.

## Типичные заблуждения

«Не вызываю get — задача в фоне сама» — деструктор future может синхронизировать. «async легче пула» — до первого профилирования. «deferred даёт фон» — нет, выполнение на ждущем.

<!-- expand:ch07-pitfalls.md -->
## Развёрнутый разбор

### Ловушки async/future — развёрнуто

**1.** В репозитории `training` эта тема стыкуется с tom 3 (потоки, mutex) и с §6 `THEORY.md` (async/future). Упражнение oracle — минимальный пример результата из другого потока без ручного promise. Тема главы: Ловушки async/future.

**2.** На Fedora собирай с `-std=c++17 -pthread`. Для coroutines **(C++20)** нужен отдельный флаг и том 5. Тема главы: Ловушки async/future.

**3.** Не спойлерь `multithread/solutions/`: книга описывает паттерны, не подставляет готовые advanced-реализации. Тема главы: Ловушки async/future.

**4.** При отладке зависания на `get()` смотри все потоки в gdb (том 6). Если future не спасает от гонки на общих данных — TSan (том 7). Тема главы: Ловушки async/future.

**5.** Код в примерах короткий; в проекте дроби длинные цепочки на функции с говорящими именами. Тема главы: Ловушки async/future.

**6.** Вопрос «кто владеет объектом до join/get» — тот же, что в томе 2 про capture: init-capture и move часто безопаснее `[&]`. Тема главы: Ловушки async/future.

**7.** Политика `std::launch::async` важна там, где тест или контракт требует **другого** `std::thread::id` — как в oracle. Тема главы: Ловушки async/future.

**8.** `shared_future` — когда много читателей **одного** значения; очередь — когда каждому нужен свой элемент. Тема главы: Ловушки async/future.

**9.** Packaged_task связывает отложенный запуск с future; пул ограничивает число потоков — complementary, не взаимозаменяемые автоматически. Тема главы: Ловушки async/future.

**10.** Executor — ответ на «куда submit’ить»; C++17 STL не даёт единого std::execution — пул и Asio закрывают дыру. Тема главы: Ловушки async/future.

**11.** Coroutines **(C++20)** — suspend/resume, не «бесплатный поток»; `co_await` зависит от awaitable. Тема главы: Ловушки async/future.

**12.** Блокирующий read в worker + future — простой C++17 путь; event loop — масштаб I/O. Тема главы: Ловушки async/future.

**13.** К собеседованию: уметь проговорить shared state, broken_promise, деструктор future от async, разницу deferred/async. Тема главы: Ловушки async/future.

**14.** Дальше по серии: том 5 (флаги), 6 (gdb), 7 (sanitizers). Тема главы: Ловушки async/future.

### Связь с oracle и THEORY.md

Oracle (`03_oracle`) и §6 `multithread/THEORY.md` — общий фундамент: результат из другого потока, явный `launch::async`, альтернатива promise+thread.

<!-- expand2 -->
## Углубление: ловушки

Пункт **1** (ловушки). Стандарт C++17 в репозитории `training` — база упражнений; C++20 coroutines помечены отдельно, чтобы не смешивать «как будто всё было в 17».
Пункт **2** (ловушки). Конспект `multithread/THEORY.md`, §6, разбирает `async`, `future`, `promise` и сценарии, когда async не подходит. Перечитывай §6 после глав 2–4.
Пункт **3** (ловушки). Упражнение `multithread/beginner/03_oracle/` требует другой поток и `.get()`; README рекомендует `std::launch::async`. Книга не подставляет код из `multithread/solutions/`.
Пункт **4** (ловушки). Том 3 даёт mutex, condition_variable, atomics и идеи пула/очереди; том 4 не отменяет синхронизацию общих данных — future защищает только канал одного результата.
Пункт **5** (ловушки). Том 2 про лямбды: capture `[&]` в фоне опасен, init-capture переносит копию; это критично для `async` и `packaged_task`.
Пункт **6** (ловушки). На Fedora: `g++ -std=c++17 -pthread`, mdBook для EPUB — `books/vol-04-async`, команда `mdbook build` (том 0).
Пункт **7** (ловушки). Деструктор `std::future` от `async` может блокировать — не fire-and-forget без плана (глава 7).
Пункт **8** (ловушки). `std::launch::deferred` выполняет work на потоке, который вызвал `get` — не путать с фоновым параллелизмом.
Пункт **9** (ловушки). `broken_promise` — promise уничтожен без set; `future_error` на второй `get`.
Пункт **10** (ловушки). Исключение из worker в `async` всплывает в `get()` вызывающего — ловить там (глава 6).
Пункт **11** (ловушки). Пул ограничивает потоки; лес `async` в цикле — red flag на собесе и в профиле.
Пункт **12** (ловушки). Packaged_task + пул — когда нужен future и контроль момента запуска; oracle — когда хватает async.
Пункт **13** (ловушки). Promise связывает void-колбэк библиотеки с blocking `get()` в main.
Пункт **14** (ловушки). Executor — «куда submit»; ручной пул из tom 3 — executor без std::execution в C++17.
Пункт **15** (ловушки). Asio `io_context` + strand — модель потоков для I/O handlers (глава 10, 13).
Пункт **16** (ловушки). Coroutines: `promise_type` coroutine ≠ `std::promise`; имена совпадают, сущности разные (глава 11).
Пункт **17** (ловушки). `co_await` приостанавливает coroutine, не обязательно освобождая поток — зависит от awaitable.
Пункт **18** (ловушки). Блокирующий read в thread + future прост для клиента; websocket-сервер на чистом async — нет.
Пункт **19** (ловушки). TSan (том 7) ловит гонки, которые future не предотвращает на ваших структурах.
Пункт **20** (ловушки). Gdb (том 6): все потоки, backtrace worker, breakpoint на `get`.
Пункт **21** (ловушки). К собесу: shared state, wait vs get, shared_future, когда promise вместо async.
Пункт **22** (ловушки). Миф: «future thread-safe для всего приложения» — только протокол shared state future.
Пункт **23** (ловушки). Миф: «coroutine = thread» — many coroutines on one thread возможны.
Пункт **24** (ловушки). Дальше: том 5 toolchain, 6 gdb, 7 sanitizers, 8 OOP/SOLID.

<!-- expand3 -->
### temporary future

`std::async(launch::async, f);` без сохранения — temporary future destructor waits. Это не detach fire-and-forget.

### цикл async

1000 итераций — 1000 потоков или pressure. Используй пул или batch.

### deferred destructor

Deferred может не run work if never waited — не полагайся на side effects без get.

### oracle id потоков

Без launch::async id может совпасть — провал oracle. Явная политика — must.

### dangling capture

`async(launch::async, [&]{ use(local); })` и return из функции до get — UB.

<!-- expand4 -->
## Практика и собес: ловушки

1. Destructor future async без get.
2. Default launch policy риск.
3. 100 async — сколько потоков может быть.
4. deferred не фон.
5. Порядок launch all get all.
6. [&] lifetime.
7. moved-from future.
8. Oracle explicit async policy.
9. Fire-and-forget async иллюзия.
10. Профиль vs пул.

<!-- expand5 -->
## Стандарт, ревью, серия (ch07 pitfalls)

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

Не игнорируй future от `async`. Явная `launch::async`. Не спамь `async` в цикле. Порядок launch/get влияет на параллелизм. Lifetime callable как в томе 2. Дальше — выбор модели.

## Вопросы для самопроверки

1. Что делает деструктор future от `async(async, ...)` без get?
2. Почему в oracle пишут политику явно?
3. Сколько потоков может создать цикл на 100 `async(async)`?
4. Безопасен ли `[&]` в async-лямбде при get в той же функции?
5. Чем deferred влияет на деструктор future?

## Обсуждение с репетитором

**Том:** 4 · **Глава:** `ch07` · **Файл:** `ch07-pitfalls.md`

**Дальше по курсу:** [Глава 8. Какую модель выбрать](./ch08-choose-model.md)
