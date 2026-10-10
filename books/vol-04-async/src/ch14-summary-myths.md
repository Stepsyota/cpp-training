# Глава 14. Резюме и мифы

## Зачем эта тема

Четыре части тома — от `async` до coroutines и I/O — легко смешать в голове в один «асинхронный C++». Финальная глава собирает **карту**, **мифы** и короткий блок **к собесу**, чтобы перед томом 5 (флаги toolchain) ты мог перечитать только её.

## Карта тома в одном проходе

**Задача с результатом** — `future`; запуск с return — `async` с явным `launch::async`, когда нужен другой поток. **Ручной ящик** — `promise`; **отложенный callable с future** — `packaged_task`. **Исключения** летят в `get()`. **Ловушки** — деструктор future, default policy, лес потоков. **Выбор** — thread / async / пул / coroutine по частоте и форме ответа. **Колбэки** — `function`, поток вызова, мост через promise. **Executors** — куда отдавать работу; Asio и P2300 **(C++26)** — ландшафт. **Coroutines** **(C++20)** — suspend, awaitable, task types из библиотек. **I/O** — блок в worker vs event loop.

Практика репозитория: oracle на `async`; пул и очередь — том 3; синхронизация памяти — том 3, не отменяется future.

## Мифы (развенчание)

**«Async в std = async/await»** — в C++17 только future/get; await — **(C++20)** и библиотеки.

**«Не вызываю get — ушло в фон»** — деструктор future от async часто ждёт.

**«Future thread-safe для всего приложения»** — только shared state future; твои `vector` под mutex сами.

**«promise и std::promise_type — одно»** — разные уровни (future vs coroutine).

**«Coroutines бесплатны и всегда быстрее»** — аллокации frame, сложная отладка.

**«std::async заменяет серверный фреймворк»** — нет executor, нет масштаба I/O.

**«packaged_task и async дублируют друг друга всегда»** — разный момент запуска и выбор потока.

**«deferred даёт параллелизм»** — выполнение на ждущем потоке.

## К собесу

- Объясни shared state, `get_future` один раз, `broken_promise`.
- Разница `wait` / `get`, когда `shared_future`.
- Что бросает `get()` и откуда исключение в `async`.
- Почему цикл на 1000 `async(async)` — red flag.
- Когда promise вместо async (колбэк, нет single return).
- Что такое executor; чем пул — ручной executor.
- Coroutine vs thread; что такое `co_await` без деталей ABI.
- Блокирующий read в thread + future vs epoll + handler.

## Что читать дальше в серии

**Том 5** — `-std=c++17/20`, оптимизации, LTO, отличия MSVC: без правильных флагов coroutines и TSan не соберёшь. **Том 6** — gdb, зависание на `get()`, все потоки. **Том 7** — TSan, когда future не спасает от гонки на общих данных. **Том 8** — ООП/SOLID после инструментов.

Практику `multithread/` не закрывай: книга дополняет, не заменяет репетитора.

<!-- expand:ch14-summary-myths.md -->
## Развёрнутый разбор

### Итог тома — развёрнуто

**1.** В репозитории `training` эта тема стыкуется с tom 3 (потоки, mutex) и с §6 `THEORY.md` (async/future). Упражнение oracle — минимальный пример результата из другого потока без ручного promise. Тема главы: Итог тома.

**2.** На Fedora собирай с `-std=c++17 -pthread`. Для coroutines **(C++20)** нужен отдельный флаг и том 5. Тема главы: Итог тома.

**3.** Не спойлерь `multithread/solutions/`: книга описывает паттерны, не подставляет готовые advanced-реализации. Тема главы: Итог тома.

**4.** При отладке зависания на `get()` смотри все потоки в gdb (том 6). Если future не спасает от гонки на общих данных — TSan (том 7). Тема главы: Итог тома.

**5.** Код в примерах короткий; в проекте дроби длинные цепочки на функции с говорящими именами. Тема главы: Итог тома.

**6.** Вопрос «кто владеет объектом до join/get» — тот же, что в томе 2 про capture: init-capture и move часто безопаснее `[&]`. Тема главы: Итог тома.

**7.** Политика `std::launch::async` важна там, где тест или контракт требует **другого** `std::thread::id` — как в oracle. Тема главы: Итог тома.

**8.** `shared_future` — когда много читателей **одного** значения; очередь — когда каждому нужен свой элемент. Тема главы: Итог тома.

**9.** Packaged_task связывает отложенный запуск с future; пул ограничивает число потоков — complementary, не взаимозаменяемые автоматически. Тема главы: Итог тома.

**10.** Executor — ответ на «куда submit’ить»; C++17 STL не даёт единого std::execution — пул и Asio закрывают дыру. Тема главы: Итог тома.

**11.** Coroutines **(C++20)** — suspend/resume, не «бесплатный поток»; `co_await` зависит от awaitable. Тема главы: Итог тома.

**12.** Блокирующий read в worker + future — простой C++17 путь; event loop — масштаб I/O. Тема главы: Итог тома.

**13.** К собеседованию: уметь проговорить shared state, broken_promise, деструктор future от async, разницу deferred/async. Тема главы: Итог тома.

**14.** Дальше по серии: том 5 (флаги), 6 (gdb), 7 (sanitizers). Тема главы: Итог тома.

### Связь с oracle и THEORY.md

Oracle (`03_oracle`) и §6 `multithread/THEORY.md` — общий фундамент: результат из другого потока, явный `launch::async`, альтернатива promise+thread.

<!-- expand2 -->
## Углубление: мифы

Пункт **1** (мифы). Стандарт C++17 в репозитории `training` — база упражнений; C++20 coroutines помечены отдельно, чтобы не смешивать «как будто всё было в 17».
Пункт **2** (мифы). Конспект `multithread/THEORY.md`, §6, разбирает `async`, `future`, `promise` и сценарии, когда async не подходит. Перечитывай §6 после глав 2–4.
Пункт **3** (мифы). Упражнение `multithread/beginner/03_oracle/` требует другой поток и `.get()`; README рекомендует `std::launch::async`. Книга не подставляет код из `multithread/solutions/`.
Пункт **4** (мифы). Том 3 даёт mutex, condition_variable, atomics и идеи пула/очереди; том 4 не отменяет синхронизацию общих данных — future защищает только канал одного результата.
Пункт **5** (мифы). Том 2 про лямбды: capture `[&]` в фоне опасен, init-capture переносит копию; это критично для `async` и `packaged_task`.
Пункт **6** (мифы). На Fedora: `g++ -std=c++17 -pthread`, mdBook для EPUB — `books/vol-04-async`, команда `mdbook build` (том 0).
Пункт **7** (мифы). Деструктор `std::future` от `async` может блокировать — не fire-and-forget без плана (глава 7).
Пункт **8** (мифы). `std::launch::deferred` выполняет work на потоке, который вызвал `get` — не путать с фоновым параллелизмом.
Пункт **9** (мифы). `broken_promise` — promise уничтожен без set; `future_error` на второй `get`.
Пункт **10** (мифы). Исключение из worker в `async` всплывает в `get()` вызывающего — ловить там (глава 6).
Пункт **11** (мифы). Пул ограничивает потоки; лес `async` в цикле — red flag на собесе и в профиле.
Пункт **12** (мифы). Packaged_task + пул — когда нужен future и контроль момента запуска; oracle — когда хватает async.
Пункт **13** (мифы). Promise связывает void-колбэк библиотеки с blocking `get()` в main.
Пункт **14** (мифы). Executor — «куда submit»; ручной пул из tom 3 — executor без std::execution в C++17.
Пункт **15** (мифы). Asio `io_context` + strand — модель потоков для I/O handlers (глава 10, 13).
Пункт **16** (мифы). Coroutines: `promise_type` coroutine ≠ `std::promise`; имена совпадают, сущности разные (глава 11).
Пункт **17** (мифы). `co_await` приостанавливает coroutine, не обязательно освобождая поток — зависит от awaitable.
Пункт **18** (мифы). Блокирующий read в thread + future прост для клиента; websocket-сервер на чистом async — нет.
Пункт **19** (мифы). TSan (том 7) ловит гонки, которые future не предотвращает на ваших структурах.
Пункт **20** (мифы). Gdb (том 6): все потоки, backtrace worker, breakpoint на `get`.
Пункт **21** (мифы). К собесу: shared state, wait vs get, shared_future, когда promise вместо async.
Пункт **22** (мифы). Миф: «future thread-safe для всего приложения» — только протокол shared state future.
Пункт **23** (мифы). Миф: «coroutine = thread» — many coroutines on one thread возможны.
Пункт **24** (мифы). Дальше: том 5 toolchain, 6 gdb, 7 sanitizers, 8 OOP/SOLID.

<!-- expand3 -->
### reread map

Ch1 task vs thread; ch2 async; ch3 future; ch4 promise; ch5 packaged; ch6 exceptions; ch7 pitfalls; ch8 choose; ch9 callbacks; ch10 executors; ch11-12 coroutines; ch13 io; practice oracle tom3.

### interview drill

Five minute oral on oracle path launch async get.

### series next

Vol 5 flags before heavy coroutines in prod.

### repo pointers

THEORY md section 6; 03_oracle readme; no solutions paste.

<!-- expand4 -->
## Практика и собес: итог

1. Повтори карту 14 глав за 2 мин.
2. Миф JS async = C++17.
3. Миф future thread-safe all.
4. Миф coroutine free.
5. Five oracle facts.
6. Five promise facts.
7. Five pitfall facts.
8. Next tom 5.
9. THEORY §6 reread.
10. solutions не спойлер.

<!-- expand5 -->
## Стандарт, ревью, серия (ch14 summary myths)

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

Том 4 — стандартная доставка результатов и ошибок (C++17) и обзор executors/coroutines/I/O. Держи в голове границы API и не смешивай модели без причины.

## Вопросы для самопроверки

1. Назови три причины явного `launch::async`.
2. Как доставить исключение из worker в main в C++17?
3. Почему пул лучше для мелких job?
4. Чем coroutine принципиально не является потоком?
5. Куда в серии смотреть TSan после этого тома?

## Обсуждение с репетитором

**Том:** 4 · **Глава:** `ch14` · **Файл:** `ch14-summary-myths.md`

**Конец тома 4.** Дальше по серии: **том 5** — флаги GCC/Clang на Fedora (`books/vol-05-toolchain-flags`, по плану в `BOOK-CONTEXT.md`).
