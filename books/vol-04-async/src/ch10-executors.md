# Глава 10. Executors и куда бежит работа

## Зачем эта тема

Слово **executor** в современном C++ значит: **абстракция места выполнения** — «запусти эту работу здесь» (конкретный поток, пул, strand, GPU — в других экосистемах). `std::async` в C++17 **не** принимает executor: реализация сама решает, где крутить. Пул из тома 3 — **ручной** executor. Понимание идеи готовит к Asio, TBB и к **senders/receivers** в будущих стандартах.

## Что не так с «голым» async

Ты не можешь сказать: «все сетевые колбэки на два потока, все CPU job — на пул из N». Без общей очереди каждый `async` — лотерея. В сервере это превращается в непредсказуемую латентность и исчерпание потоков. Executor — именованная политика: **куда** отдавать `submit(f)`.

## Ручной пул как executor

Интерфейс мысленно: `execute(F&& f)` кладёт `f` в очередь; worker’ы забирают. Ограничение числа потоков, метрика `busy`, `wait_idle` — всё про **управление ресурсом**. Future на каждую задачу опционален. Это то, что ты уже проходил концептуально в томе 3; здесь акцент на роли «исполнитель», а не на mutex внутри.

## Intel oneTBB, OpenMP, task arenas

Вне стандартной библиотеки **TBB** и **OpenMP** дают планировщики задач с work-stealing: idle поток крадёт job у другого. Для CPU-bound параллелизма на контейнерах это часто быстрее самодельного пула. Связь с книгой: `std::execution::par` в libstdc++ может опираться на backend; детали — реализация и том 5 (флаги).

## Boost.Asio и io_context

Asio **(библиотека, не STL)** моделирует асинхронный I/O: `io_context::run()` крутит handlers на небольшом числе потоков; `post` / `dispatch` отправляют колбэк на executor контекста. **Strand** сериализует handlers (как mutex для очереди колбэков). Это ответ на вопрос «в каком потоке вызовут мой колбэк» из главы 9. Полный учебник Asio — не этот том; достаточно знать, что **сеть + async** почти всегда executor + очередь, не `async` из заголовка future.

## C++20 без единого std::execution в 17

**(C++20)** добавил coroutines и `jthread`, но **универсальный executor в core** долго обсуждался отдельно. Черновик **P2300** (*std::execution*) описывает **senders**, **receivers**, **`schedule`**, **`then`** — композиция асинхронных шагов без callback hell. В **C++26** часть попадает в стандарт **(пометка: проверяй версию GCC/Clang на Fedora)**. До внедрения в твоём toolchain мысленная модель: **описать граф работ** и **привязать к контексту выполнения**.

## Связь с coroutines

Coroutine `co_await` на awaitable часто привязан к executor: «приостановись, продолжи на этом io_context». Библиотеки (`asio::awaitable`, folly::coro) связывают task type с конкретным executor. Стандартный минимум в C++20 — только механизм coroutine, не сетевой стек.

## Выбор на практике (C++17 проект)

Один процесс, CPU parallel map — `thread` + деление диапазона или `par`. Постоянная фоновая работа — свой пул. Редкие тяжёлые запросы — `async`. Сеть — Asio или аналог с executor. Не смешивай три модели без границ подсистем.

## Типичные заблуждения

«Executor = std::async» — нет. «C++20 решил executors полностью» — экосистема шире стандарта. «Один пул на всё» — I/O и CPU лучше разделять.

<!-- expand:ch10-executors.md -->
## Развёрнутый разбор

### Executors — развёрнуто

**1.** В репозитории `training` эта тема стыкуется с tom 3 (потоки, mutex) и с §6 `THEORY.md` (async/future). Упражнение oracle — минимальный пример результата из другого потока без ручного promise. Тема главы: Executors.

**2.** На Fedora собирай с `-std=c++17 -pthread`. Для coroutines **(C++20)** нужен отдельный флаг и том 5. Тема главы: Executors.

**3.** Не спойлерь `multithread/solutions/`: книга описывает паттерны, не подставляет готовые advanced-реализации. Тема главы: Executors.

**4.** При отладке зависания на `get()` смотри все потоки в gdb (том 6). Если future не спасает от гонки на общих данных — TSan (том 7). Тема главы: Executors.

**5.** Код в примерах короткий; в проекте дроби длинные цепочки на функции с говорящими именами. Тема главы: Executors.

**6.** Вопрос «кто владеет объектом до join/get» — тот же, что в томе 2 про capture: init-capture и move часто безопаснее `[&]`. Тема главы: Executors.

**7.** Политика `std::launch::async` важна там, где тест или контракт требует **другого** `std::thread::id` — как в oracle. Тема главы: Executors.

**8.** `shared_future` — когда много читателей **одного** значения; очередь — когда каждому нужен свой элемент. Тема главы: Executors.

**9.** Packaged_task связывает отложенный запуск с future; пул ограничивает число потоков — complementary, не взаимозаменяемые автоматически. Тема главы: Executors.

**10.** Executor — ответ на «куда submit’ить»; C++17 STL не даёт единого std::execution — пул и Asio закрывают дыру. Тема главы: Executors.

**11.** Coroutines **(C++20)** — suspend/resume, не «бесплатный поток»; `co_await` зависит от awaitable. Тема главы: Executors.

**12.** Блокирующий read в worker + future — простой C++17 путь; event loop — масштаб I/O. Тема главы: Executors.

**13.** К собеседованию: уметь проговорить shared state, broken_promise, деструктор future от async, разницу deferred/async. Тема главы: Executors.

**14.** Дальше по серии: том 5 (флаги), 6 (gdb), 7 (sanitizers). Тема главы: Executors.

### Связь с oracle и THEORY.md

Oracle (`03_oracle`) и §6 `multithread/THEORY.md` — общий фундамент: результат из другого потока, явный `launch::async`, альтернатива promise+thread.

<!-- expand2 -->
## Углубление: executors

Пункт **1** (executors). Стандарт C++17 в репозитории `training` — база упражнений; C++20 coroutines помечены отдельно, чтобы не смешивать «как будто всё было в 17».
Пункт **2** (executors). Конспект `multithread/THEORY.md`, §6, разбирает `async`, `future`, `promise` и сценарии, когда async не подходит. Перечитывай §6 после глав 2–4.
Пункт **3** (executors). Упражнение `multithread/beginner/03_oracle/` требует другой поток и `.get()`; README рекомендует `std::launch::async`. Книга не подставляет код из `multithread/solutions/`.
Пункт **4** (executors). Том 3 даёт mutex, condition_variable, atomics и идеи пула/очереди; том 4 не отменяет синхронизацию общих данных — future защищает только канал одного результата.
Пункт **5** (executors). Том 2 про лямбды: capture `[&]` в фоне опасен, init-capture переносит копию; это критично для `async` и `packaged_task`.
Пункт **6** (executors). На Fedora: `g++ -std=c++17 -pthread`, mdBook для EPUB — `books/vol-04-async`, команда `mdbook build` (том 0).
Пункт **7** (executors). Деструктор `std::future` от `async` может блокировать — не fire-and-forget без плана (глава 7).
Пункт **8** (executors). `std::launch::deferred` выполняет work на потоке, который вызвал `get` — не путать с фоновым параллелизмом.
Пункт **9** (executors). `broken_promise` — promise уничтожен без set; `future_error` на второй `get`.
Пункт **10** (executors). Исключение из worker в `async` всплывает в `get()` вызывающего — ловить там (глава 6).
Пункт **11** (executors). Пул ограничивает потоки; лес `async` в цикле — red flag на собесе и в профиле.
Пункт **12** (executors). Packaged_task + пул — когда нужен future и контроль момента запуска; oracle — когда хватает async.
Пункт **13** (executors). Promise связывает void-колбэк библиотеки с blocking `get()` в main.
Пункт **14** (executors). Executor — «куда submit»; ручной пул из tom 3 — executor без std::execution в C++17.
Пункт **15** (executors). Asio `io_context` + strand — модель потоков для I/O handlers (глава 10, 13).
Пункт **16** (executors). Coroutines: `promise_type` coroutine ≠ `std::promise`; имена совпадают, сущности разные (глава 11).
Пункт **17** (executors). `co_await` приостанавливает coroutine, не обязательно освобождая поток — зависит от awaitable.
Пункт **18** (executors). Блокирующий read в thread + future прост для клиента; websocket-сервер на чистом async — нет.
Пункт **19** (executors). TSan (том 7) ловит гонки, которые future не предотвращает на ваших структурах.
Пункт **20** (executors). Gdb (том 6): все потоки, backtrace worker, breakpoint на `get`.
Пункт **21** (executors). К собесу: shared state, wait vs get, shared_future, когда promise вместо async.
Пункт **22** (executors). Миф: «future thread-safe для всего приложения» — только протокол shared state future.
Пункт **23** (executors). Миф: «coroutine = thread» — many coroutines on one thread возможны.
Пункт **24** (executors). Дальше: том 5 toolchain, 6 gdb, 7 sanitizers, 8 OOP/SOLID.

<!-- expand3 -->
### pool as executor

submit/execute mental model; bounded queue backpressure tom 3.

### async without executor param

Cannot bind async to named pool in C++17.

### Asio post vs dispatch

dispatch may run inline if on context thread; post queues.

### P2300 preview

senders receivers schedule; C++26 check compiler tom 5.

<!-- expand4 -->
## Практика и собес: executors

1. Пул = executor словами.
2. async без executor param.
3. Asio io_context run.
4. strand зачем.
5. TBB OpenMP one sentence.
6. P2300 sender one sentence.
7. Разделить CPU и I/O pools.
8. post vs dispatch.
9. Coroutines resume on executor.
10. C++17 project выбор.

<!-- expand5 -->
## Стандарт, ревью, серия (ch10 executors)

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

Executor отвечает **где** бежит работа. C++17: пул руками, async без выбора, библиотеки сами. C++26+: смотреть P2300/std::execution. Asio — эталон мысли для I/O. Дальше — coroutines как синтаксис ожидания.

## Вопросы для самопроверки

1. Чем пул из тома 3 похож на executor?
2. Зачем Asio strand?
3. Почему async не задаёт executor?
4. Что такое sender в P2300 одним предложением?
5. Связь `par` и внутреннего пула компилятора?

## Обсуждение с репетитором

**Том:** 4 · **Глава:** `ch10` · **Файл:** `ch10-executors.md`

**Дальше по курсу:** [Глава 11. Coroutines — основы](./ch11-coroutines-basics.md)
