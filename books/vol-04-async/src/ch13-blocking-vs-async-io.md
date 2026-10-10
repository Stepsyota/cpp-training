# Глава 13. Блокирующий и асинхронный I/O

## Зачем эта тема

Многопоточность из тома 3 и future из этого тома часто встречаются вокруг **ввода-вывода**: диск, сокет, pipe. Блокирующий `read` в worker потоке + future на результат — рабочая C++17 схема. «Настоящий» async I/O на одном потоке с тысячами соединений — другая архитектура (epoll, io_uring, completion ports). Различать модели важно, чтобы не ждать от `async` магии сети.

## Блокирующий I/O в отдельном потоке

```cpp
auto fut = std::async(std::launch::async, [] {
    return read_file_blocking(path);
});
auto content = fut.get();
```

Пока `read` блокирует worker, поток ОС занят. Для пары файлов — нормально. Для десяти тысяч соединений — десять тысяч потоков не масштабируются. Вывод: **модель «поток на соединение»** уступает event loop + неблокирующие дескрипторы на высокой нагрузке.

## promise и колбэк от poll/epoll

В event loop: главный цикл `epoll_wait`; когда сокет готов, вызывается handler. Handler может `set_value` в promise, main или другой поток ждёт `future`. `async` тут не создаёт поток на каждый read — поток(и) event loop уже есть. Это стык **главы 4** и **главы 10**.

## Асинхронный API библиотеки

Boost.Asio: `async_read` с completion token; **(C++20)** можно completion как coroutine. libcurl multi interface, gRPC async — свой callback contract. Общее: **инициатор** не блокирует; **завершение** приходит позже на известном executor.

Том не учит настраивать Asio — только чтобы ты узнал паттерн в чужом коде.

## std::async и сеть

Оборачивать каждый `recv` в `async(async)` — плохо: потоки, latency, нет backpressure. Для клиента с малым числом запросов — терпимо. Для сервера — нет.

## backpressure и очереди

Когда producer быстрее consumer, нужна **bounded queue** (том 3): не бесконечный список future. Async pipeline: стадии + очереди между ними, не цепочка unbounded `async`.

## Таймауты и wait_for

`future.wait_for` видит готовность read, но **не отменяет** блокирующий read в другом потоке в C++17. Отмена — отдельный протокол (close socket, `pthread_cancel` не в игре, stop flag + shutdown). **(C++20)** jthread + stop для worker цикла.

## Связь с томами 5–7

Сборка с Asio, флаги `-pthread`, линковка — **том 5**. Отладка зависания на `get()` — **том 6** (gdb, все потоки). Гонка между handler и main без синхронизации — **том 7** (TSan).

## Типичные заблуждения

«Future делает I/O неблокирующим» — нет, только переносит блок в другой поток. «Один io_context на весь датацентр» — конфигурация и strands. «async подходит для websocket сервера» — только с оговорками масштаба.

<!-- expand:ch13-blocking-vs-async-io.md -->
## Развёрнутый разбор

### I/O модели — развёрнуто

**1.** В репозитории `training` эта тема стыкуется с tom 3 (потоки, mutex) и с §6 `THEORY.md` (async/future). Упражнение oracle — минимальный пример результата из другого потока без ручного promise. Тема главы: I/O модели.

**2.** На Fedora собирай с `-std=c++17 -pthread`. Для coroutines **(C++20)** нужен отдельный флаг и том 5. Тема главы: I/O модели.

**3.** Не спойлерь `multithread/solutions/`: книга описывает паттерны, не подставляет готовые advanced-реализации. Тема главы: I/O модели.

**4.** При отладке зависания на `get()` смотри все потоки в gdb (том 6). Если future не спасает от гонки на общих данных — TSan (том 7). Тема главы: I/O модели.

**5.** Код в примерах короткий; в проекте дроби длинные цепочки на функции с говорящими именами. Тема главы: I/O модели.

**6.** Вопрос «кто владеет объектом до join/get» — тот же, что в томе 2 про capture: init-capture и move часто безопаснее `[&]`. Тема главы: I/O модели.

**7.** Политика `std::launch::async` важна там, где тест или контракт требует **другого** `std::thread::id` — как в oracle. Тема главы: I/O модели.

**8.** `shared_future` — когда много читателей **одного** значения; очередь — когда каждому нужен свой элемент. Тема главы: I/O модели.

**9.** Packaged_task связывает отложенный запуск с future; пул ограничивает число потоков — complementary, не взаимозаменяемые автоматически. Тема главы: I/O модели.

**10.** Executor — ответ на «куда submit’ить»; C++17 STL не даёт единого std::execution — пул и Asio закрывают дыру. Тема главы: I/O модели.

**11.** Coroutines **(C++20)** — suspend/resume, не «бесплатный поток»; `co_await` зависит от awaitable. Тема главы: I/O модели.

**12.** Блокирующий read в worker + future — простой C++17 путь; event loop — масштаб I/O. Тема главы: I/O модели.

**13.** К собеседованию: уметь проговорить shared state, broken_promise, деструктор future от async, разницу deferred/async. Тема главы: I/O модели.

**14.** Дальше по серии: том 5 (флаги), 6 (gdb), 7 (sanitizers). Тема главы: I/O модели.

### Связь с oracle и THEORY.md

Oracle (`03_oracle`) и §6 `multithread/THEORY.md` — общий фундамент: результат из другого потока, явный `launch::async`, альтернатива promise+thread.

<!-- expand2 -->
## Углубление: I/O

Пункт **1** (I/O). Стандарт C++17 в репозитории `training` — база упражнений; C++20 coroutines помечены отдельно, чтобы не смешивать «как будто всё было в 17».
Пункт **2** (I/O). Конспект `multithread/THEORY.md`, §6, разбирает `async`, `future`, `promise` и сценарии, когда async не подходит. Перечитывай §6 после глав 2–4.
Пункт **3** (I/O). Упражнение `multithread/beginner/03_oracle/` требует другой поток и `.get()`; README рекомендует `std::launch::async`. Книга не подставляет код из `multithread/solutions/`.
Пункт **4** (I/O). Том 3 даёт mutex, condition_variable, atomics и идеи пула/очереди; том 4 не отменяет синхронизацию общих данных — future защищает только канал одного результата.
Пункт **5** (I/O). Том 2 про лямбды: capture `[&]` в фоне опасен, init-capture переносит копию; это критично для `async` и `packaged_task`.
Пункт **6** (I/O). На Fedora: `g++ -std=c++17 -pthread`, mdBook для EPUB — `books/vol-04-async`, команда `mdbook build` (том 0).
Пункт **7** (I/O). Деструктор `std::future` от `async` может блокировать — не fire-and-forget без плана (глава 7).
Пункт **8** (I/O). `std::launch::deferred` выполняет work на потоке, который вызвал `get` — не путать с фоновым параллелизмом.
Пункт **9** (I/O). `broken_promise` — promise уничтожен без set; `future_error` на второй `get`.
Пункт **10** (I/O). Исключение из worker в `async` всплывает в `get()` вызывающего — ловить там (глава 6).
Пункт **11** (I/O). Пул ограничивает потоки; лес `async` в цикле — red flag на собесе и в профиле.
Пункт **12** (I/O). Packaged_task + пул — когда нужен future и контроль момента запуска; oracle — когда хватает async.
Пункт **13** (I/O). Promise связывает void-колбэк библиотеки с blocking `get()` в main.
Пункт **14** (I/O). Executor — «куда submit»; ручной пул из tom 3 — executor без std::execution в C++17.
Пункт **15** (I/O). Asio `io_context` + strand — модель потоков для I/O handlers (глава 10, 13).
Пункт **16** (I/O). Coroutines: `promise_type` coroutine ≠ `std::promise`; имена совпадают, сущности разные (глава 11).
Пункт **17** (I/O). `co_await` приостанавливает coroutine, не обязательно освобождая поток — зависит от awaitable.
Пункт **18** (I/O). Блокирующий read в thread + future прост для клиента; websocket-сервер на чистом async — нет.
Пункт **19** (I/O). TSan (том 7) ловит гонки, которые future не предотвращает на ваших структурах.
Пункт **20** (I/O). Gdb (том 6): все потоки, backtrace worker, breakpoint на `get`.
Пункт **21** (I/O). К собесу: shared state, wait vs get, shared_future, когда promise вместо async.
Пункт **22** (I/O). Миф: «future thread-safe для всего приложения» — только протокол shared state future.
Пункт **23** (I/O). Миф: «coroutine = thread» — many coroutines on one thread возможны.
Пункт **24** (I/O). Дальше: том 5 toolchain, 6 gdb, 7 sanitizers, 8 OOP/SOLID.

<!-- expand3 -->
### thread per connection

Simple; breaks at 10k.

### epoll promise bridge

Handler set_value; main get.

### wait_for no cancel

Blocking read continues unless socket shutdown.

### tom 5 6 7

build flags gdb tsan cross refs.

<!-- expand4 -->
## Практика и собес: I/O

1. blocking read + async thread.
2. 10k threads problem.
3. epoll handler set_value.
4. wait_for не cancel read.
5. bounded queue pipeline.
6. Asio vs std async one para.
7. backpressure.
8. gdb hang on get.
9. TSan handler main.
10. tom 5 build asio.

<!-- expand5 -->
## Стандарт, ревью, серия (ch13 blocking vs async io)

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

Блокирующий I/O + thread/async — простой C++17 путь. Высокая конкуррентность — event loop + колбэки/coroutines + executor. promise связывает миры. Очереди дают backpressure. Дальше — итог тома.

## Вопросы для самопроверки

1. Почему «поток на соединение» ломается на 10k клиентов?
2. Где в схеме epoll + promise сидит future?
3. Отменяет ли `wait_for` блокирующий read?
4. Зачем bounded queue между стадиями pipeline?
5. Чем Asio отличается от `std::async` одним абзацем?

## Обсуждение с репетитором

**Том:** 4 · **Глава:** `ch13` · **Файл:** `ch13-blocking-vs-async-io.md`

**Дальше по курсу:** [Глава 14. Резюме и мифы](./ch14-summary-myths.md)
