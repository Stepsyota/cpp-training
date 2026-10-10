# Глава 5. packaged_task

## Зачем эта тема

Иногда callable нужно **отложить**: положить в очередь, передать в `std::thread`, вызвать позже на конкретном потоке — но при этом заранее иметь **`std::future`** на результат. `std::packaged_task<R(Args...)>` оборачивает функцию с фиксированной сигнатурой и связывает её с shared state, как promise, только запись в future происходит **при вызове** `operator()` пакета.

## Создание и future

```cpp
std::packaged_task<int(int)> job(compute);
std::future<int> ticket = job.get_future();
// позже, возможно в другом потоке:
job(10);
int r = ticket.get();
```

`get_future()` — снова один раз. Вызов `job(args...)` выполняет тело и кладёт return (или исключение) в future. До вызова `get()` может блокировать: результат ещё не готов.

## Move-only и очередь задач

`packaged_task` нельзя копировать — только move. Это созвучно пулу из тома 3: в очередь кладут **владение** задачей. В учебных пулах часто используют `std::function<void()>` без future на каждую мелочь; `packaged_task` уместен, когда вызывающему нужен именно **ответ** конкретной job.

Обернуть packaged_task в лямбду `void()` для очереди можно так: move task в closure, внутри вызвать `task(args)` — сигнатура и аргументы должны быть зафиксированы при постановке в очередь (часто через bind или лямбду с захваченными args).

## Отличие от async

| | `std::async` | `packaged_task` |
|---|--------------|-----------------|
| Когда запуск | Сразу (с policy) | Когда ты вызовешь `operator()` |
| Кто выбирает поток | Реализация / policy | Ты (вызов из нужного потока) |
| Сигнатура | Шаблон, любой callable | Явная `R(Args...)` |

`async` — «запусти сейчас и дай future». `packaged_task` — «вот будущий результат; выполни, когда скажу».

## make_ready_at_thread_exit

У `packaged_task` есть `make_ready_at_thread_exit` — результат публикуется при **выходе из потока**, в котором вызывался packaged_task. Узкий API для стыковки с C API и деструкторами; в повседневном коде редок. Знай, что существует, не строй на нём архитектуру без нужды.

## Лямбды и типы

Лямбда без преобразования к указателю на функцию может не подойти к `packaged_task<int(int)>` без обёртки — тип должен совпасть. Generic lambda **(C++20)** усложняет стирание в `packaged_task` с фиксированной сигнатурой; на практике явная функция или `std::function` с оговоренной сигнатурой.

## Связь с томом 2

В packaged_task часто лежит лямбда с init-capture: move тяжёлого состояния внутрь задачи до постановки в очередь. Захват `[&]` на поля пула опасен, если задача выполнится после уничтожения referent — те же правила, что в `ch09` тома 2.

## Типичные заблуждения

«packaged_task = async» — момент запуска разный. «Можно копировать в очередь» — только move. «get_future после вызова operator()» — future берут **до** выполнения.

<!-- expand:ch05-packaged-task.md -->
## Развёрнутый разбор

`std::packaged_task<R(Args...)>` — callable-обёртка: при `operator()(args...)` выполняет тело и публикует return в future, как promise+функция в одном.

### Момент запуска

Разница с `async`: packaged_task **не** стартует сам. Ты решаешь, вызвать сейчас на main, позже на worker пула, или один раз после dequeue.

### Очередь и move

Копировать нельзя — только move в `std::deque` или в lambda `void()` для пула. Паттерн: `tasks.push_back([t = std::move(task)]() mutable { t(args); });` — args часто захвачены в closure.

Эталон пула в `solutions/` не спойлерим; идея: очередь хранит type-erased `void()` job, future нужен только если caller ждёт ответ.

### Сравнение с async (таблица словами)

Async: реализация выбирает поток (policy). Packaged_task: **ты** выбираешь поток вызовом. Async: сразу после вызова `async`. Packaged_task: между `get_future` и `operator()` может пройти время.

### make_ready_at_thread_exit

Редкий API: результат при выходе из потока, где крутили task. Знай для чтения legacy; не строй архитектуру на этом без нужды.

### Сигнатура и лямбды

Лямбда с уникальным типом не всегда ложится в `packaged_task<int(int)>` без обёртки. Явная free function или state в capture с фиксированным `operator()`.

### К собесу

Когда packaged_task в пуле лучше async. Почему move-only.

<!-- expand2 -->
## Углубление: packaged_task

Пункт **1** (packaged_task). Стандарт C++17 в репозитории `training` — база упражнений; C++20 coroutines помечены отдельно, чтобы не смешивать «как будто всё было в 17».
Пункт **2** (packaged_task). Конспект `multithread/THEORY.md`, §6, разбирает `async`, `future`, `promise` и сценарии, когда async не подходит. Перечитывай §6 после глав 2–4.
Пункт **3** (packaged_task). Упражнение `multithread/beginner/03_oracle/` требует другой поток и `.get()`; README рекомендует `std::launch::async`. Книга не подставляет код из `multithread/solutions/`.
Пункт **4** (packaged_task). Том 3 даёт mutex, condition_variable, atomics и идеи пула/очереди; том 4 не отменяет синхронизацию общих данных — future защищает только канал одного результата.
Пункт **5** (packaged_task). Том 2 про лямбды: capture `[&]` в фоне опасен, init-capture переносит копию; это критично для `async` и `packaged_task`.
Пункт **6** (packaged_task). На Fedora: `g++ -std=c++17 -pthread`, mdBook для EPUB — `books/vol-04-async`, команда `mdbook build` (том 0).
Пункт **7** (packaged_task). Деструктор `std::future` от `async` может блокировать — не fire-and-forget без плана (глава 7).
Пункт **8** (packaged_task). `std::launch::deferred` выполняет work на потоке, который вызвал `get` — не путать с фоновым параллелизмом.
Пункт **9** (packaged_task). `broken_promise` — promise уничтожен без set; `future_error` на второй `get`.
Пункт **10** (packaged_task). Исключение из worker в `async` всплывает в `get()` вызывающего — ловить там (глава 6).
Пункт **11** (packaged_task). Пул ограничивает потоки; лес `async` в цикле — red flag на собесе и в профиле.
Пункт **12** (packaged_task). Packaged_task + пул — когда нужен future и контроль момента запуска; oracle — когда хватает async.
Пункт **13** (packaged_task). Promise связывает void-колбэк библиотеки с blocking `get()` в main.
Пункт **14** (packaged_task). Executor — «куда submit»; ручной пул из tom 3 — executor без std::execution в C++17.
Пункт **15** (packaged_task). Asio `io_context` + strand — модель потоков для I/O handlers (глава 10, 13).
Пункт **16** (packaged_task). Coroutines: `promise_type` coroutine ≠ `std::promise`; имена совпадают, сущности разные (глава 11).
Пункт **17** (packaged_task). `co_await` приостанавливает coroutine, не обязательно освобождая поток — зависит от awaitable.
Пункт **18** (packaged_task). Блокирующий read в thread + future прост для клиента; websocket-сервер на чистом async — нет.
Пункт **19** (packaged_task). TSan (том 7) ловит гонки, которые future не предотвращает на ваших структурах.
Пункт **20** (packaged_task). Gdb (том 6): все потоки, backtrace worker, breakpoint на `get`.
Пункт **21** (packaged_task). К собесу: shared state, wait vs get, shared_future, когда promise вместо async.
Пункт **22** (packaged_task). Миф: «future thread-safe для всего приложения» — только протокол shared state future.
Пункт **23** (packaged_task). Миф: «coroutine = thread» — many coroutines on one thread возможны.
Пункт **24** (packaged_task). Дальше: том 5 toolchain, 6 gdb, 7 sanitizers, 8 OOP/SOLID.

<!-- expand3 -->
### Обёртка void() для deque

```cpp
std::packaged_task<int()> task([] { return 42; });
auto fut = task.get_future();
queue.push([t = std::move(task)]() mutable { t(); });
```

Worker deque, вызывает `t()`, main `fut.get()`. Сигнатура task должна совпадать с тем, что вызовет worker.

### Отличие от std::function

function копируем, но move-only closure с unique_ptr внутри — проблема. packaged_task move-only, future привязан к одной task.

### Oracle и packaged_task

Oracle проще через async; packaged_task — когда submit в **существующий** пул, а не новый thread на каждый вопрос.

<!-- expand4 -->
## Практика и собес: packaged_task

1. get_future до operator().
2. move в очередь void jobs.
3. Отличие от async по выбору потока.
4. make_ready_at_thread_exit — когда встретишь.
5. Лямбда и фиксированная сигнатура.
6. Oracle vs packaged_task+pool.
7. Исключение через operator() в future.
8. Один packaged_task — один future.
9. Связь с tom3 pool идея без кода solutions.
10. Когда function<void()> без future хватит.

<!-- expand5 -->
## Стандарт, ревью, серия (ch05 packaged task)

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

`packaged_task` связывает отложенный вызов с `future`. Move-only, одна future на задачу. Удобен для пулов с результатом и ручного выбора потока выполнения. Дальше — исключения.

## Вопросы для самопроверки

1. В какой момент shared state получает значение при packaged_task?
2. Чем packaged_task лучше async для очереди пула?
3. Почему нельзя скопировать packaged_task в `std::deque`?
4. Сколько раз вызывают `get_future`?
5. Что произойдёт, если вызвать `get()` до `operator()` задачи?

## Обсуждение с репетитором

**Том:** 4 · **Глава:** `ch05` · **Файл:** `ch05-packaged-task.md`

**Дальше по курсу:** [Глава 6. Исключения и future](./ch06-exceptions.md)
