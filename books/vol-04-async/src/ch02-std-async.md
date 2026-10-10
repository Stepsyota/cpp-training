# Глава 2. std::async и политики запуска

## Зачем эта тема

`std::async` — самый короткий путь от «есть функция» к «есть `std::future` на её результат». Для учебных задач и прототипов этого хватает; для продакшена на десятки тысяч мелких задач — часто нет. Чтобы не удивляться тестам и поведению на Fedora с **libstdc++**, нужно знать **шаблон вызова** и **политику запуска** (`std::launch`).

## Заголовок и сигнатура

```cpp
#include <future>

auto fut = std::async(std::launch::async, [] { return 42; });
int answer = fut.get();
```

`async` — функция-шаблон в namespace `std`. Первый аргумент после политики (если она есть) — **callable**; дальше — аргументы, которые будут переданы в callable (как при `std::thread`). Возвращаемый тип — `std::future<std::invoke_result_t<F, Args...>>` (в голове: future от типа return).

Callable может быть лямбдой, `std::bind` **(legacy)**, указателем на функцию. Захват в лямбде подчиняется правилам тома 2: копии и init-capture переживают вызов `async`, ссылки — только если referent живёт до завершения работы и до `get()`.

## std::launch::async

`std::launch::async` — требование: выполнение **как будто** в новом потоке (стандарт формулирует через «вызов может выполняться в отдельном потоке»). На практике для `03_oracle` и любых тестов «два разных `std::thread::id`» пиши политику **явно**:

```cpp
auto fut = std::async(std::launch::async, worker, arg);
```

Без явной политики реализация может выбрать отложенное выполнение или смешанную стратегию — см. главу 7.

## std::launch::deferred

`std::launch::deferred` откладывает вызов callable до момента, когда на future зовут `wait` или `get`. Тогда работа часто выполняется **в потоке того, кто ждёт**. Это не «бесплатный фон»: пока ты не вызвал `get()`, тяжёлой работы может не быть вообще. Плюс — не создаётся лишний поток; минус — блокировка на `get()` совпадает с выполнением задачи, параллелизма нет.

Комбинация флагов `async | deferred` даёт реализации право выбрать — ещё одна причина явно указывать `async`, когда нужен именно другой поток.

## Связь с упражнением oracle

Сценарий «спросить что-то в другом потоке и вернуть строку» — канонический `async` + `launch::async` + `.get()`. Внутри `async` библиотека заводит shared state, запускает поток (или эквивалент), по завершении callable кладёт return в future. Тебе не нужно вручную создавать `promise` — но понимание ящика (глава 4) снимает ощущение магии.

## Параллельный запуск нескольких async

```cpp
auto a = std::async(std::launch::async, phase_a);
auto b = std::async(std::launch::async, phase_b);
use(a.get(), b.get());
```

Пока не вызван `get()`, обе задачи могут идти параллельно. Порядок `get()` влияет только на то, **когда** ты блокируешься за каждым результатом, а не обязательно на порядок завершения на ядрах. Если `phase_b` падает с исключением, оно всплывёт при `b.get()` — глава 6.

## Ограничения модели

`async` привязан к **одному return** (или void). Нельзя «подписаться» на поток промежуточных значений без своего протокола. Нельзя передать в стандартный `async` «выполни на вот этом executor из пула» — в C++17 нет параметра executor; пул из тома 3 — отдельный дизайн. Отмена задачи стандартом C++17 для `async` не описана; **(C++20)** `std::jthread` и `stop_token` — другая линия (том 3, глава 13).

## Fedora / GCC

С `-std=c++17 -pthread` линкуй как для потоков. `std::async` в libstdc++ historically использовал thread pool или новые потоки в зависимости от версии — для переносимости опирайся на **стандарт**, а не на деталь реализации. Если сомневаешься, измеряй и смотри `thread::id` в тестах.

## Типичные заблуждения

«`async` всегда создаёт поток» — только с подходящей политикой и реализацией. «Можно fire-and-forget `async` без `get`» — деструктор `future` может блокировать (глава 7). «`async` заменяет пул» — при лавине мелких задач — плохая идея.

<!-- expand:ch02-std-async.md -->
## Развёрнутый разбор

Функция `std::async` — шаблон в `<future>`. Компилятор выводит тип результата из callable так же, как для `std::thread` и `std::invoke_result_t` в parallel_map (том 3): return callable становится типом `future`. Если callable возвращает `void`, получишь `future<void>` — `get()` только синхронизирует завершение.

Первый аргумент — политика `std::launch`. В C++17 это битовая маска: `async`, `deferred`, или OR обоих. **Не** передавать политику — значит «реализация решает». На Fedora с libstdc++ это не абстракция: поведение влияет на тесты oracle и на профилирование.

### Oracle без спойлера решения

README **`03_oracle`** формулирует три условия: вернуть то, что вернула `question`; выполнить её **не** в вызывающем потоке; не вернуться раньше конца работы. `std::async(std::launch::async, question)` (или эквивалент с аргументами) запускает работу; `.get()` на future блокирует до готовности и возвращает значение. §6 **`THEORY.md`** показывает ту же однострочную идею и объясняет, почему без `launch::async` id потоков может совпасть.

Альтернатива из README — `promise`/`future` + `std::thread`: promise уходит в worker, future остаётся у caller, `set_value` с return `question`, снаружи `get()` и `join`. Больше строк, тот же контракт. Oracle не заставляет выбирать — обучает оба пути.

### Deferred по шагам

При `std::launch::deferred` тело callable **не** обязано выполниться в момент вызова `async`. Оно выполняется, когда кто-то на связанном future вызовет `wait` или `get` — **в том потоке**, который вызвал wait/get. Параллелизма нет: это ленивое вычисление с блокировкой на точке ожидания. Полезно для редких тяжёлых функций, которые могут и не понадобиться; вредно, если ты **ждал** другой поток для разгрузки UI.

`wait_for` на deferred future до первого `get` может вернуть `deferred`, а не `ready` — см. главу 3. Путаница deferred/async — частая причина «почему в gdb один поток».

### Несколько async и порядок get

Идиома «запусти все, потом забери все» сохраняет параллелизм:

```cpp

auto fa = std::async(std::launch::async, work_a);

auto fb = std::async(std::launch::async, work_b);

auto ra = fa.get();

auto rb = fb.get();

```

Если между `async` и `get` вставить другую длинную работу в main, это не отменяет фоновые задачи — но откладывает момент, когда ты забираешь результат. Если второй `async` стоит **после** `get` первого, второй стартует позже — иногда так и задумано (цепочка зависимостей), иногда это ошибка планирования.

### Ограничения и executor

В C++17 у `async` **нет** параметра «выполни на пуле X». Каждый вызов с `launch::async` — потенциально новый поток ОС (глава 7). Пул из тома 3 — **ручной executor**: ты сам кладёшь job. `packaged_task` (глава 5) — мост «future + выбор момента вызова».

### Fedora / GCC / Clang

Сборка как у упражнений: `-std=c++17 -pthread`. Заголовок `<future>` тянет threading support. Для отладки oracle: `cmake --build build --target 03_oracle`, бreakpoint на `get()` и сравнение `std::this_thread::get_id()` — том 6. Не полагайся на «libstdc++ всегда создаёт поток» без `launch::async` — стандарт этого не обещает.

### К собесу

Объясни default policy и почему oracle требует `async`. Чем `deferred` отличается по потоку выполнения. Почему `async` не заменяет thread pool при 10k задач.

<!-- expand2 -->
## Углубление: std::async

Пункт **1** (std::async). Стандарт C++17 в репозитории `training` — база упражнений; C++20 coroutines помечены отдельно, чтобы не смешивать «как будто всё было в 17».
Пункт **2** (std::async). Конспект `multithread/THEORY.md`, §6, разбирает `async`, `future`, `promise` и сценарии, когда async не подходит. Перечитывай §6 после глав 2–4.
Пункт **3** (std::async). Упражнение `multithread/beginner/03_oracle/` требует другой поток и `.get()`; README рекомендует `std::launch::async`. Книга не подставляет код из `multithread/solutions/`.
Пункт **4** (std::async). Том 3 даёт mutex, condition_variable, atomics и идеи пула/очереди; том 4 не отменяет синхронизацию общих данных — future защищает только канал одного результата.
Пункт **5** (std::async). Том 2 про лямбды: capture `[&]` в фоне опасен, init-capture переносит копию; это критично для `async` и `packaged_task`.
Пункт **6** (std::async). На Fedora: `g++ -std=c++17 -pthread`, mdBook для EPUB — `books/vol-04-async`, команда `mdbook build` (том 0).
Пункт **7** (std::async). Деструктор `std::future` от `async` может блокировать — не fire-and-forget без плана (глава 7).
Пункт **8** (std::async). `std::launch::deferred` выполняет work на потоке, который вызвал `get` — не путать с фоновым параллелизмом.
Пункт **9** (std::async). `broken_promise` — promise уничтожен без set; `future_error` на второй `get`.
Пункт **10** (std::async). Исключение из worker в `async` всплывает в `get()` вызывающего — ловить там (глава 6).
Пункт **11** (std::async). Пул ограничивает потоки; лес `async` в цикле — red flag на собесе и в профиле.
Пункт **12** (std::async). Packaged_task + пул — когда нужен future и контроль момента запуска; oracle — когда хватает async.
Пункт **13** (std::async). Promise связывает void-колбэк библиотеки с blocking `get()` в main.
Пункт **14** (std::async). Executor — «куда submit»; ручной пул из tom 3 — executor без std::execution в C++17.
Пункт **15** (std::async). Asio `io_context` + strand — модель потоков для I/O handlers (глава 10, 13).
Пункт **16** (std::async). Coroutines: `promise_type` coroutine ≠ `std::promise`; имена совпадают, сущности разные (глава 11).
Пункт **17** (std::async). `co_await` приостанавливает coroutine, не обязательно освобождая поток — зависит от awaitable.
Пункт **18** (std::async). Блокирующий read в thread + future прост для клиента; websocket-сервер на чистом async — нет.
Пункт **19** (std::async). TSan (том 7) ловит гонки, которые future не предотвращает на ваших структурах.
Пункт **20** (std::async). Gdb (том 6): все потоки, backtrace worker, breakpoint на `get`.
Пункт **21** (std::async). К собесу: shared state, wait vs get, shared_future, когда promise вместо async.
Пункт **22** (std::async). Миф: «future thread-safe для всего приложения» — только протокол shared state future.
Пункт **23** (std::async). Миф: «coroutine = thread» — many coroutines on one thread возможны.
Пункт **24** (std::async). Дальше: том 5 toolchain, 6 gdb, 7 sanitizers, 8 OOP/SOLID.

<!-- expand3 -->
### Пошагово: вызов async на Fedora

Ты включаешь `<future>`, линкуешь `-pthread`. Вызываешь `std::async` с политикой и callable. Реализация создаёт shared state и запускает или откладывает work. Возвращается `future`. Caller сохраняет future, если результат нужен позже; иначе временный future уничтожится в конце выражения — для `launch::async` это часто скрытый join (глава 7).

Для oracle callable — функция `question`, переданная в `ask_oracle`. Тип future совпадает с return type `question`. `.get()` блокирует caller до return worker и возвращает копию или move строки.

### invoke_result и perfect forwarding

Async передаёт аргuments в callable как forwarding reference в шаблоне. Если передаёшь lvalue, callable может получить копию; rvalue — move. Для тяжёлых объектов это важно: `async(launch::async, f, std::move(big))` vs копия по ошибке.

### Политика OR async|deferred

Если передать оба бита, реализация **может** выбрать любой. Тесты, требующие новый поток, **не** должны полагаться на OR. Явный `async` или явный `deferred` — контракт с компилятором и командой.

### Когда не async

Долгоживущий worker с cv и очередью — thread/pool. Колбэк void — promise. Один return из функции — async подходит. Oracle — второй случай с политикой.

<!-- expand4 -->
## Практика и собес: std::async

1. Напиши устно сигнатуру: async + launch::async + callable.
2. Почему README oracle упоминает launch::async.
3. Что может выбрать реализация без политики.
4. Deferred: кто выполняет callable.
5. Сравни moment запуска async и packaged_task.
6. Параллельный launch a и b, затем get — зачем так.
7. Заголовки и -pthread на Linux.
8. Можно ли async с void return — что даёт get.
9. Почему 1000 async в цикле — проблема (forward ch7).
10. invoke_result_t связь с типом future.

<!-- expand5 -->
## Стандарт, ревью, серия (ch02 std async)

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

`std::async` запускает callable и возвращает `future` на результат. **`std::launch::async`** — явный другой поток для учебных и production-сценариев, где это важно. **`deferred`** — отложить до `get()`. Дальше — устройство `future` и ожидание.

## Вопросы для самопроверки

1. Какой заголовок и флаг линковки нужны на Linux вместе с `async`?
2. Чем `deferred` отличается от `async` по моменту выполнения callable?
3. Зачем в oracle писать `launch::async` явно?
4. Можно ли передать аргументы в callable так же, как в `std::thread`?
5. Сколько значений типа `int` доставляет один `std::future<int>` без `shared_future`?

## Обсуждение с репетитором

**Том:** 4 · **Глава:** `ch02` · **Файл:** `ch02-std-async.md`

**Дальше по курсу:** [Глава 3. future и shared_future](./ch03-future-shared.md)
