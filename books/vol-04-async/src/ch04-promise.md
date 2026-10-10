# Глава 4. promise вручную

## Зачем эта тема

`std::async` прячет пару promise/future внутри. Как только результат приходит **не из одного return** — колбэк GUI, пакет от сети, поток, который ты сам создал, — нужен **`std::promise<T>`**: ручка писателя в тот же shared state, из которого читатель берёт `std::future<T>`.

Конспект `multithread/THEORY.md` уже рисует схему «ящик»; здесь — связный разбор API и ошибок, без подстановки кода oracle.

## Две ручки, одно состояние

```cpp
std::promise<int> box;
std::future<int>  ticket = box.get_future();
```

`get_future()` вызывают **один раз**. Второй раз — `std::future_error`. После этого `box` и `ticket` связаны: что положишь в `box`, то заберёшь через `ticket.get()`.

`promise` **не** стартует поток. Это только канал. Поток, если нужен, создаёшь ты и в нём вызываешь `set_value` или передаёшь `promise` по move в работу.

## set_value, set_exception, void

```cpp
box.set_value(42);
// в другом месте:
int v = ticket.get();
```

Для `promise<void>` — `set_value()` без аргумента. Исключение из задачи в `async` попадает в future автоматически; вручную — `set_exception(std::make_exception_ptr(...))` (глава 6).

Повторный `set_value` на том же promise — ошибка. Писатель должен завершить состояние **ровно один раз**.

## broken_promise

Если уничтожить `promise`, не вызвав `set_value` / `set_exception`, читатель при `get()` получит **`std::future_errc::broken_promise`**. Это сигнал: обещание не выполнили. Типичные причины — ранний return из функции, забыли передать результат из колбэка, поток упал до `set_value`.

Планируй lifetime: promise должен жить до момента записи или корректно «сломаться» осознанно.

## Передача promise в другой поток

Идиома: move promise в лямбду потока, future оставить у вызывающего.

```cpp
std::promise<std::string> box;
auto ticket = box.get_future();
std::thread worker([p = std::move(box)]() mutable {
    p.set_value(fetch());
});
worker.detach(); // в учебнике лучше join — см. lifetime
std::string s = ticket.get();
```

`promise` move-only. В поток передаёшь **только promise** (писатель); `future` остаётся там, кто ждёт. Не передавай обе ручки «на всякий случай» в один поток без протокола — путаница с тем, кто владеет состоянием.

## Когда async не подходит (сценарии)

**Колбэк чужой библиотеки:** ты регистрируешь функцию «когда данные придут, вызови меня»; внутри колбэка — `p.set_value(...)`. `async` тут не к чему прикрутить, пока нет единого синхронного return.

**Несколько этапов с одним билетом:** редко — обычно очередь событий; но «первый готовый заголовок» можно опубликовать через promise, остальное — отдельно.

**Свой поток без обёртки async:** полный контроль над стеком и приоритетом; promise — только доставка ответа.

## Связь с async

`std::async` внутри создаёт shared state и по завершении callable делает эквивалент `set_value(return)`. Понимание promise объясняет, почему `get()` блокирует и почему исключение из лямбды всплывает в `get()`.

## Типичные заблуждения

«Нужно вызвать get_future дважды для двух читателей» — нет, нужен `shared_future`. «promise сам join’ит поток» — нет. «Можно set_value после get» — нет, состояние уже завершено.

<!-- expand:ch04-promise.md -->
## Развёрнутый разбор

Глава опирается на §6 `multithread/THEORY.md`: promise — **ручка писателя**, future — **ручка читателя**, между ними shared state.

`std::promise<T>` не знает про потоки. Это канал «положи ровно один результат». Запуск worker — твой `std::thread`, пул или колбэк библиотеки.

### get_future ровно один раз

Повторный `get_future()` бросает `std::future_error`. Два future на один promise стандарт не выдаёт — иначе два читателя забрали бы одно значение дважды без `shared_future`.

### set_value и set_exception

После успешного `set_value` состояние **завершено**. Второй set — ошибка. Если worker бросил, можно `set_exception(std::current_exception())` — читатель увидит throw в `get()` (глава 6).

Для `promise<void>` вызывают `set_value()` без аргумента — «работа сделана, значения нет».

### broken_promise

Уничтожили promise без set — читатель получит `broken_promise`. Это не «случайный краш», а **контракт**: обещание не выполнили. Частые причины: ранний return, исключение до set, забыли колбэк.

### Move promise в поток

Идиома init-capture: `[p = std::move(box)]() mutable { p.set_value(...); }`. Future остаётся у caller. **Не** отдавай future в тот же worker, который пишет — путаница владения.

### Когда async не подходит (из THEORY §6)

**Колбэк с void:** сеть вызывает `on_packet` — return наружу нет, `async(on_packet)` не ждёт пакет. **Кнопка UI:** нажатие позже, handler void. **Сигнал из середины функции:** `set_value` после заголовка файла, worker читает хвост дальше — один `async` на всю функцию задержит `get()` до конца.

Oracle **не** из этих случаев: один вызов `question` с return — как раз `async`.

### Oracle и promise

README oracle: можно promise+thread вместо async. Тогда ты явно создаёшь ящик, worker кладёт return `question`, main `get()`. Учит тому же §6, что «async внутри делает promise-подобное».

### К собесу

Три сущности: promise, future, shared state. broken_promise. Почему promise не join’ит поток.

<!-- expand2 -->
## Углубление: promise

Пункт **1** (promise). Стандарт C++17 в репозитории `training` — база упражнений; C++20 coroutines помечены отдельно, чтобы не смешивать «как будто всё было в 17».
Пункт **2** (promise). Конспект `multithread/THEORY.md`, §6, разбирает `async`, `future`, `promise` и сценарии, когда async не подходит. Перечитывай §6 после глав 2–4.
Пункт **3** (promise). Упражнение `multithread/beginner/03_oracle/` требует другой поток и `.get()`; README рекомендует `std::launch::async`. Книга не подставляет код из `multithread/solutions/`.
Пункт **4** (promise). Том 3 даёт mutex, condition_variable, atomics и идеи пула/очереди; том 4 не отменяет синхронизацию общих данных — future защищает только канал одного результата.
Пункт **5** (promise). Том 2 про лямбды: capture `[&]` в фоне опасен, init-capture переносит копию; это критично для `async` и `packaged_task`.
Пункт **6** (promise). На Fedora: `g++ -std=c++17 -pthread`, mdBook для EPUB — `books/vol-04-async`, команда `mdbook build` (том 0).
Пункт **7** (promise). Деструктор `std::future` от `async` может блокировать — не fire-and-forget без плана (глава 7).
Пункт **8** (promise). `std::launch::deferred` выполняет work на потоке, который вызвал `get` — не путать с фоновым параллелизмом.
Пункт **9** (promise). `broken_promise` — promise уничтожен без set; `future_error` на второй `get`.
Пункт **10** (promise). Исключение из worker в `async` всплывает в `get()` вызывающего — ловить там (глава 6).
Пункт **11** (promise). Пул ограничивает потоки; лес `async` в цикле — red flag на собесе и в профиле.
Пункт **12** (promise). Packaged_task + пул — когда нужен future и контроль момента запуска; oracle — когда хватает async.
Пункт **13** (promise). Promise связывает void-колбэк библиотеки с blocking `get()` в main.
Пункт **14** (promise). Executor — «куда submit»; ручной пул из tom 3 — executor без std::execution в C++17.
Пункт **15** (promise). Asio `io_context` + strand — модель потоков для I/O handlers (глава 10, 13).
Пункт **16** (promise). Coroutines: `promise_type` coroutine ≠ `std::promise`; имена совпадают, сущности разные (глава 11).
Пункт **17** (promise). `co_await` приостанавливает coroutine, не обязательно освобождая поток — зависит от awaitable.
Пункт **18** (promise). Блокирующий read в thread + future прост для клиента; websocket-сервер на чистом async — нет.
Пункт **19** (promise). TSan (том 7) ловит гонки, которые future не предотвращает на ваших структурах.
Пункт **20** (promise). Gdb (том 6): все потоки, backtrace worker, breakpoint на `get`.
Пункт **21** (promise). К собесу: shared state, wait vs get, shared_future, когда promise вместо async.
Пункт **22** (promise). Миф: «future thread-safe для всего приложения» — только протокол shared state future.
Пункт **23** (promise). Миф: «coroutine = thread» — many coroutines on one thread возможны.
Пункт **24** (promise). Дальше: том 5 toolchain, 6 gdb, 7 sanitizers, 8 OOP/SOLID.

<!-- expand3 -->
### Прогон времени (как в THEORY §6)

Worker может вызвать `set_value` до или после того, как main вошёл в `get`. Оба порядка законны. `get` блокирует, пока не будет set. `join` на thread всё равно нужен для handle потока, даже после get — §6 объясняет разделение «число забрал» vs «ручку thread закрыл».

### set_value из колбэка

Схема: promise в heap или shared_ptr, если lifetime колбэка длиннее stack frame регистрации. Не вызывай set после уничтожения promise. GUI: set в handler кнопки, get в worker логики — следи за потоком UI.

### Oracle vs promise

Oracle можно решить без явного promise — async достаточно. Promise учат, чтобы понимать колбэки и §6 «три сущности». На собесе «когда promise» — колбэк void, не single return path.

### broken_promise отладка

Если get бросает broken_promise, ищи ранний exit, необработанное исключение до set, или promise уничтожен на stack при выходе из scope до set.

<!-- expand4 -->
## Практика и собес: promise

1. get_future сколько раз.
2. broken_promise когда.
3. promise move в thread capture.
4. Колбэк void + set_value схема словами.
5. §6 три сценария без async.
6. Oracle: нужен ли promise (нет, но альтернатива).
7. set_exception vs throw в worker для reader.
8. join после get в ручной схеме.
9. Не передавать future writer thread без протокола.
10. promise не создаёт поток.

<!-- expand5 -->
## Стандарт, ревью, серия (ch04 promise)

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

`promise` — писатель в shared state; `get_future()` один раз; `set_value` / `set_exception` один раз; иначе `broken_promise`. Потоки и колбэки сами не появляются — их проектируешь ты. Дальше — `packaged_task` как callable с уже встроенным future.

## Вопросы для самопроверки

1. Кто вызывает `get_future` и сколько раз?
2. Что увидит поток на `get()`, если promise уничтожен без set?
3. Почему promise move-only?
4. Запускает ли promise выполнение callable?
5. Как oracle связан с promise, если в коде только async?

## Обсуждение с репетитором

**Том:** 4 · **Глава:** `ch04` · **Файл:** `ch04-promise.md`

**Дальше по курсу:** [Глава 5. packaged_task](./ch05-packaged-task.md)
