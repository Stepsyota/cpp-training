# Глава 12. Генераторы и task-типы **(C++20)**

## Зачем эта тема

Механизм coroutine в стандарте **минимальный**: он не даёт готовый `std::task<T>` для сети, как в других языках. Экосистема заполняет пробел: **генераторы** (`co_yield`), **lazy sequences**, **asio::awaitable**, Folly Coro, cppcoro. Глава фиксирует, что искать в документации и как не смешать с future из C++17.

## Генераторы и ленивость

Генератор выдаёт значения по одному при итерации. Вместо `vector` на миллион элементов сразу — coroutine отдаёт следующий по запросу. Память и CPU экономятся, когда потребитель не читает всё. Реализация — свой `promise_type` с `yield_value` и итератор-обёртка снаружи.

Граница: генератор **не** параллелен сам по себе; параллелизм добавляется отдельно (pipeline, том 3).

## Task как возвращаемый тип async функции

Идиома в библиотеках:

```cpp
task<int> compute_async(); // псевдосигнатура
```

Внутри `co_await` на timer, socket, другой task. Вызов `compute_async()` возвращает объект, который можно `co_await` снаружи или связать с callback/future на границе системы.

Стандартизированного `std::task` в C++20 **нет** — тип из вашей библиотеки или корпоративного фреймворка.

## co_await цепочки

Линейный код:

```cpp
auto data = co_await read_packet();
auto parsed = co_await parse(data);
co_return parsed;
```

Эквивалент без coroutines — вложенные колбэки или ручной state machine. Читаемость — главный выигрыш; производительность сравнивай с care.

## Lifetime и ссылки в coroutine

Локальные ссылки и `[&]` в coroutine опаснее, чем в короткой лямбде: suspend может отложить resume на долго, referent умрёт. Предпочитай значения, `shared_ptr`, явные контракты awaitable. Параллель с томом 2, глава 10.

## Отмена **(C++20)**

`std::stop_token` / `std::jthread` (том 3) интегрируются с некоторыми task-библиотеками: при stop запросе coroutine бросает или завершает await. У `std::async` C++17 отмены нет — ещё одно сравнение моделей.

## Когда не coroutines

Тесный CPU-bound loop на все ядра — parallel algorithms или пул. Простой oracle — `async`. Coroutines окупаются при **многоступенчатом ожидании** и читаемости.

## Связь с executors

Task обычно привязан к `io_context` или executor: resume после I/O на правильном потоке. Без этого — data race на UI или на соединении.

## Типичные заблуждения

«std::generator есть в C++20 везде» — **(C++23)** `std::generator` в стандарте; на старых компиляторах — своя реализация. «Любая async функция должна быть coroutine» — нет. «co_await заменяет future.get» — только с правильным awaitable.

<!-- expand:ch12-generators-tasks.md -->
## Развёрнутый разбор

### Генераторы и task types — развёрнуто

**1.** В репозитории `training` эта тема стыкуется с tom 3 (потоки, mutex) и с §6 `THEORY.md` (async/future). Упражнение oracle — минимальный пример результата из другого потока без ручного promise. Тема главы: Генераторы и task types.

**2.** На Fedora собирай с `-std=c++17 -pthread`. Для coroutines **(C++20)** нужен отдельный флаг и том 5. Тема главы: Генераторы и task types.

**3.** Не спойлерь `multithread/solutions/`: книга описывает паттерны, не подставляет готовые advanced-реализации. Тема главы: Генераторы и task types.

**4.** При отладке зависания на `get()` смотри все потоки в gdb (том 6). Если future не спасает от гонки на общих данных — TSan (том 7). Тема главы: Генераторы и task types.

**5.** Код в примерах короткий; в проекте дроби длинные цепочки на функции с говорящими именами. Тема главы: Генераторы и task types.

**6.** Вопрос «кто владеет объектом до join/get» — тот же, что в томе 2 про capture: init-capture и move часто безопаснее `[&]`. Тема главы: Генераторы и task types.

**7.** Политика `std::launch::async` важна там, где тест или контракт требует **другого** `std::thread::id` — как в oracle. Тема главы: Генераторы и task types.

**8.** `shared_future` — когда много читателей **одного** значения; очередь — когда каждому нужен свой элемент. Тема главы: Генераторы и task types.

**9.** Packaged_task связывает отложенный запуск с future; пул ограничивает число потоков — complementary, не взаимозаменяемые автоматически. Тема главы: Генераторы и task types.

**10.** Executor — ответ на «куда submit’ить»; C++17 STL не даёт единого std::execution — пул и Asio закрывают дыру. Тема главы: Генераторы и task types.

**11.** Coroutines **(C++20)** — suspend/resume, не «бесплатный поток»; `co_await` зависит от awaitable. Тема главы: Генераторы и task types.

**12.** Блокирующий read в worker + future — простой C++17 путь; event loop — масштаб I/O. Тема главы: Генераторы и task types.

**13.** К собеседованию: уметь проговорить shared state, broken_promise, деструктор future от async, разницу deferred/async. Тема главы: Генераторы и task types.

**14.** Дальше по серии: том 5 (флаги), 6 (gdb), 7 (sanitizers). Тема главы: Генераторы и task types.

### Связь с oracle и THEORY.md

Oracle (`03_oracle`) и §6 `multithread/THEORY.md` — общий фундамент: результат из другого потока, явный `launch::async`, альтернатива promise+thread.

<!-- expand2 -->
## Углубление: generators

Пункт **1** (generators). Стандарт C++17 в репозитории `training` — база упражнений; C++20 coroutines помечены отдельно, чтобы не смешивать «как будто всё было в 17».
Пункт **2** (generators). Конспект `multithread/THEORY.md`, §6, разбирает `async`, `future`, `promise` и сценарии, когда async не подходит. Перечитывай §6 после глав 2–4.
Пункт **3** (generators). Упражнение `multithread/beginner/03_oracle/` требует другой поток и `.get()`; README рекомендует `std::launch::async`. Книга не подставляет код из `multithread/solutions/`.
Пункт **4** (generators). Том 3 даёт mutex, condition_variable, atomics и идеи пула/очереди; том 4 не отменяет синхронизацию общих данных — future защищает только канал одного результата.
Пункт **5** (generators). Том 2 про лямбды: capture `[&]` в фоне опасен, init-capture переносит копию; это критично для `async` и `packaged_task`.
Пункт **6** (generators). На Fedora: `g++ -std=c++17 -pthread`, mdBook для EPUB — `books/vol-04-async`, команда `mdbook build` (том 0).
Пункт **7** (generators). Деструктор `std::future` от `async` может блокировать — не fire-and-forget без плана (глава 7).
Пункт **8** (generators). `std::launch::deferred` выполняет work на потоке, который вызвал `get` — не путать с фоновым параллелизмом.
Пункт **9** (generators). `broken_promise` — promise уничтожен без set; `future_error` на второй `get`.
Пункт **10** (generators). Исключение из worker в `async` всплывает в `get()` вызывающего — ловить там (глава 6).
Пункт **11** (generators). Пул ограничивает потоки; лес `async` в цикле — red flag на собесе и в профиле.
Пункт **12** (generators). Packaged_task + пул — когда нужен future и контроль момента запуска; oracle — когда хватает async.
Пункт **13** (generators). Promise связывает void-колбэк библиотеки с blocking `get()` в main.
Пункт **14** (generators). Executor — «куда submit»; ручной пул из tom 3 — executor без std::execution в C++17.
Пункт **15** (generators). Asio `io_context` + strand — модель потоков для I/O handlers (глава 10, 13).
Пункт **16** (generators). Coroutines: `promise_type` coroutine ≠ `std::promise`; имена совпадают, сущности разные (глава 11).
Пункт **17** (generators). `co_await` приостанавливает coroutine, не обязательно освобождая поток — зависит от awaitable.
Пункт **18** (generators). Блокирующий read в thread + future прост для клиента; websocket-сервер на чистом async — нет.
Пункт **19** (generators). TSan (том 7) ловит гонки, которые future не предотвращает на ваших структурах.
Пункт **20** (generators). Gdb (том 6): все потоки, backtrace worker, breakpoint на `get`.
Пункт **21** (generators). К собесу: shared state, wait vs get, shared_future, когда promise вместо async.
Пункт **22** (generators). Миф: «future thread-safe для всего приложения» — только протокол shared state future.
Пункт **23** (generators). Миф: «coroutine = thread» — many coroutines on one thread возможны.
Пункт **24** (generators). Дальше: том 5 toolchain, 6 gdb, 7 sanitizers, 8 OOP/SOLID.

<!-- expand3 -->
### co_yield protocol

yield_value in promise_type; iterator pulls next.

### std::generator C++23

May not be on Fedora yet; custom or library.

### task type boundary

Interop with callback via co_await wrapper around future.

### stop when async enough

oracle stays async in C++17 project.

<!-- expand4 -->
## Практика и собес: generators/tasks

1. co_yield ленивость.
2. generator не parallel alone.
3. task from library.
4. co_await chain readability.
5. lifetime coroutine refs.
6. stop_token cancel.
7. std generator C++23 note.
8. Oracle остаётся async.
9. executor on resume.
10. profile coroutine cost.

<!-- expand5 -->
## Стандарт, ревью, серия (ch12 generators tasks)

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

Генераторы — `co_yield` и ленивость. Task types — из библиотек, не из C++17 STL. Цепочки `co_await` читаемы, но требуют дисциплины lifetime и executor. Дальше — I/O контекст.

## Вопросы для самопроверки

1. Чем генератор отличается от `vector` по памяти?
2. Есть ли в стандарте C++20 готовый сетевой task?
3. Почему ссылка на локальную переменную в coroutine опасна?
4. Как stop_token связан с отменой?
5. Когда oracle лучше оставить на async?

## Обсуждение с репетитором

**Том:** 4 · **Глава:** `ch12` · **Файл:** `ch12-generators-tasks.md`

**Дальше по курсу:** [Глава 13. Блокирующий и асинхронный I/O](./ch13-blocking-vs-async-io.md)
