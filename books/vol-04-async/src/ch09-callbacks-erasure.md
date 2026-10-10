# Глава 9. Колбэки и стирание типа

## Зачем эта тема

`std::future` — типизированный конец одной операции. Реальные системы годами жили на **колбэках**: «когда готово — вызови `void (*)(void*)`». В C++ колбэк — часто `std::function<void()>`, указатель на функцию или лямбда с уникальным типом closure. Чтобы класть разные лямбды в одну очередь, нужно **стирание типа**; у него цена.

## std::function

`std::function<R(Args...)>` хранит любой callable с совместимой сигнатурой. Копируемый (если внутри копируемый callable). Может аллоцировать на куче для маленькой лямбды. Для hot path с миллионами вызовов иногда заменяют на шаблон `template<class F> void submit(F&&)` без стирания — как в продвинутых пулах; в учебнике достаточно знать trade-off.

Ограничение: `std::function` **не** держит move-only closure напрямую в старых практиках; **(C++11)** уже позволяет с оговорками, но move-only лямбды с unique ресурсами часто кладут в `std::packaged_task` или custom type erasure.

## Колбэк vs future

Колбэк инвертирует контроль: библиотека звонит **тебе**, когда удобно. Future — ты звонишь **`get()`**, когда готов ждать. Связать их можно: в колбэке `p.set_value(x)`, снаружи `future.get()`. Continuation «когда future готов — сделай ещё» в C++17 STL **нет**; пишут цикл, `then` в Boost/threading, или coroutines **(C++20)**.

## Владение и поток вызова

Вопросы при каждом колбэке: в **каком потоке** вызовут? Можно ли блокировать? Живут ли захваченные указатели? GUI часто требует «обработчик только в UI thread» — тогда из worker колбэка **post** в очередь UI, а не трогать виджеты напрямую. Это тот же invariant, что join и lifetime в томе 2, только поток вызова выбирает чужой код.

## Указатели на функции и legacy

C API: `void on_complete(int status, void* user_data)`. C++ обёртка кладёт `promise*` в `user_data` или использует статическую трамплин-функцию. Не теряй исключения: C не знает про throw — лови в трамплине и `set_exception`.

`std::bind` **(legacy)** комбинировал аргументы для колбэков; в новом коде предпочтительны лямбды с явным захватом (том 2, глава 11).

## packaged_task как erasure с future

`packaged_task` фиксирует сигнатуру и даёт future — узкий erasure без полного `function`. Очередь `function<void()>` вокруг task — паттерн «стереть всё в void job».

## Типичные заблуждения

«function бесплатен» — может аллоцировать. «Колбэк всегда в том же потоке» — зависит от API. «future и колбэк взаимоисключают» — мостят promise.

<!-- expand:ch09-callbacks-erasure.md -->
## Развёрнутый разбор

### Колбэки и type erasure — развёрнуто

**1.** В репозитории `training` эта тема стыкуется с tom 3 (потоки, mutex) и с §6 `THEORY.md` (async/future). Упражнение oracle — минимальный пример результата из другого потока без ручного promise. Тема главы: Колбэки и type erasure.

**2.** На Fedora собирай с `-std=c++17 -pthread`. Для coroutines **(C++20)** нужен отдельный флаг и том 5. Тема главы: Колбэки и type erasure.

**3.** Не спойлерь `multithread/solutions/`: книга описывает паттерны, не подставляет готовые advanced-реализации. Тема главы: Колбэки и type erasure.

**4.** При отладке зависания на `get()` смотри все потоки в gdb (том 6). Если future не спасает от гонки на общих данных — TSan (том 7). Тема главы: Колбэки и type erasure.

**5.** Код в примерах короткий; в проекте дроби длинные цепочки на функции с говорящими именами. Тема главы: Колбэки и type erasure.

**6.** Вопрос «кто владеет объектом до join/get» — тот же, что в томе 2 про capture: init-capture и move часто безопаснее `[&]`. Тема главы: Колбэки и type erasure.

**7.** Политика `std::launch::async` важна там, где тест или контракт требует **другого** `std::thread::id` — как в oracle. Тема главы: Колбэки и type erasure.

**8.** `shared_future` — когда много читателей **одного** значения; очередь — когда каждому нужен свой элемент. Тема главы: Колбэки и type erasure.

**9.** Packaged_task связывает отложенный запуск с future; пул ограничивает число потоков — complementary, не взаимозаменяемые автоматически. Тема главы: Колбэки и type erasure.

**10.** Executor — ответ на «куда submit’ить»; C++17 STL не даёт единого std::execution — пул и Asio закрывают дыру. Тема главы: Колбэки и type erasure.

**11.** Coroutines **(C++20)** — suspend/resume, не «бесплатный поток»; `co_await` зависит от awaitable. Тема главы: Колбэки и type erasure.

**12.** Блокирующий read в worker + future — простой C++17 путь; event loop — масштаб I/O. Тема главы: Колбэки и type erasure.

**13.** К собеседованию: уметь проговорить shared state, broken_promise, деструктор future от async, разницу deferred/async. Тема главы: Колбэки и type erasure.

**14.** Дальше по серии: том 5 (флаги), 6 (gdb), 7 (sanitizers). Тема главы: Колбэки и type erasure.

### Связь с oracle и THEORY.md

Oracle (`03_oracle`) и §6 `multithread/THEORY.md` — общий фундамент: результат из другого потока, явный `launch::async`, альтернатива promise+thread.

<!-- expand2 -->
## Углубление: колбэки

Пункт **1** (колбэки). Стандарт C++17 в репозитории `training` — база упражнений; C++20 coroutines помечены отдельно, чтобы не смешивать «как будто всё было в 17».
Пункт **2** (колбэки). Конспект `multithread/THEORY.md`, §6, разбирает `async`, `future`, `promise` и сценарии, когда async не подходит. Перечитывай §6 после глав 2–4.
Пункт **3** (колбэки). Упражнение `multithread/beginner/03_oracle/` требует другой поток и `.get()`; README рекомендует `std::launch::async`. Книга не подставляет код из `multithread/solutions/`.
Пункт **4** (колбэки). Том 3 даёт mutex, condition_variable, atomics и идеи пула/очереди; том 4 не отменяет синхронизацию общих данных — future защищает только канал одного результата.
Пункт **5** (колбэки). Том 2 про лямбды: capture `[&]` в фоне опасен, init-capture переносит копию; это критично для `async` и `packaged_task`.
Пункт **6** (колбэки). На Fedora: `g++ -std=c++17 -pthread`, mdBook для EPUB — `books/vol-04-async`, команда `mdbook build` (том 0).
Пункт **7** (колбэки). Деструктор `std::future` от `async` может блокировать — не fire-and-forget без плана (глава 7).
Пункт **8** (колбэки). `std::launch::deferred` выполняет work на потоке, который вызвал `get` — не путать с фоновым параллелизмом.
Пункт **9** (колбэки). `broken_promise` — promise уничтожен без set; `future_error` на второй `get`.
Пункт **10** (колбэки). Исключение из worker в `async` всплывает в `get()` вызывающего — ловить там (глава 6).
Пункт **11** (колбэки). Пул ограничивает потоки; лес `async` в цикле — red flag на собесе и в профиле.
Пункт **12** (колбэки). Packaged_task + пул — когда нужен future и контроль момента запуска; oracle — когда хватает async.
Пункт **13** (колбэки). Promise связывает void-колбэк библиотеки с blocking `get()` в main.
Пункт **14** (колбэки). Executor — «куда submit»; ручной пул из tom 3 — executor без std::execution в C++17.
Пункт **15** (колбэки). Asio `io_context` + strand — модель потоков для I/O handlers (глава 10, 13).
Пункт **16** (колбэки). Coroutines: `promise_type` coroutine ≠ `std::promise`; имена совпадают, сущности разные (глава 11).
Пункт **17** (колбэки). `co_await` приостанавливает coroutine, не обязательно освобождая поток — зависит от awaitable.
Пункт **18** (колбэки). Блокирующий read в thread + future прост для клиента; websocket-сервер на чистом async — нет.
Пункт **19** (колбэки). TSan (том 7) ловит гонки, которые future не предотвращает на ваших структурах.
Пункт **20** (колбэки). Gdb (том 6): все потоки, backtrace worker, breakpoint на `get`.
Пункт **21** (колбэки). К собесу: shared state, wait vs get, shared_future, когда promise вместо async.
Пункт **22** (колбэки). Миф: «future thread-safe для всего приложения» — только протокол shared state future.
Пункт **23** (колбэки). Миф: «coroutine = thread» — many coroutines on one thread возможны.
Пункт **24** (колбэки). Дальше: том 5 toolchain, 6 gdb, 7 sanitizers, 8 OOP/SOLID.

<!-- expand3 -->
### function overhead

Small buffer optimization не гарантирован; большие lambda heap.

### post to UI thread

Worker set_value или queue to UI; не touch widgets from worker.

### C trampoline

static function calls user_data promise set.

### continuation gap

C++17 no future::then in STL; loop or coroutines or Boost.

<!-- expand4 -->
## Практика и собес: колбэки

1. function аллокации.
2. Колбэк vs get инверсия.
3. promise мост.
4. UI thread post.
5. C API trampoline.
6. bind legacy vs lambda.
7. packaged_task erasure с future.
8. then нет в C++17 STL.
9. move-only closure problem.
10. Поток вызова колбэка узнать из docs.

<!-- expand5 -->
## Стандарт, ревью, серия (ch09 callbacks erasure)

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

Очереди и API любят стирание типа; платишь копированием и аллокациями. Колбэки требуют знать поток вызова и lifetime. Promise связывает колбэк с future. Дальше — executors.

## Вопросы для самопроверки

1. Зачем пулу `function<void()>` вместо шаблона?
2. Как доставить исключение из C колбэка в C++ future?
3. Чем init-capture помогает в задаче очереди?
4. Есть ли `then` у `std::future` в C++17?
5. Почему move-only лямбду не всегда кладут в function?

## Обсуждение с репетитором

**Том:** 4 · **Глава:** `ch09` · **Файл:** `ch09-callbacks-erasure.md`

**Дальше по курсу:** [Глава 10. Executors и куда бежит работа](./ch10-executors.md)
