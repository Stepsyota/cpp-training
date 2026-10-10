# Глава 11. Coroutines — основы **(C++20)**

## Зачем эта тема

Цепочка «запусти async, потом в другом потоке get, потом снова async» утомляет и плодит ошибки lifetime. **Coroutines** в C++20 — не отдельные потоки, а **функции, которые могут приостановиться** и возобновиться, сохраняя локальное состояние в куче под капотом компилятора. Синтаксис: `co_await`, `co_yield`, `co_return`. В C++17 этого нет — включай `-std=c++20` и проверяй поддержку на Fedora.

## Что такое coroutine для языка

Обычная функция: вход → тело → return. Coroutine: вход → тело → **suspend** → (позже) resume → … → `co_return`. Приостановка не обязана освобождать стек вызова ОС — состояние живёт в **coroutine frame**, управляемом через `std::coroutine_handle` **(C++20)**.

Компилятор превращает тело в **state machine**: каждая точка `co_await` — возможная граница состояний.

## promise_type и return object

Чтобы функция стала coroutine, её возвращаемый тип должен поддерживать специальный протокол: вложенный `promise_type`, методы вроде `get_return_object`, `initial_suspend`, `final_suspend`, `return_void` / `return_value`, `unhandled_exception`. Это **не** то же самое, что `std::promise` из future — имена совпадают, сущности разные.

Ты редко пишешь всё с нуля: берёшь готовый **task type** из библиотеки или минимальный генератор из учебника.

## co_await

Выражение после `co_await` должно быть **awaitable**: для него находят `operator co_await` или traits `await_ready`, `await_suspend`, `await_resume`. Если `await_ready()` false — coroutine suspend’ится, управление возвращается вызывающему; когда операция готова — resume.

Простейший mental model: `co_await` = «подожди событие, не блокируя поток ОС целиком» **если** awaitable так устроен (как в Asio). Наивный awaitable может и заблокировать поток — дисциплина на авторе типа.

## Минимальный генератор (идея)

```cpp
// Упрощённая схема — полный код в учебниках по C++20
generator<int> range(int n) {
    for (int i = 0; i < n; ++i)
        co_yield i;
}
```

`co_yield x` публикует значение наружу и suspend’ится до следующего запроса. Это ленивая последовательность без ручного state struct.

## Ключевые слова coroutine

В томе 1 они в группе control: `co_await`, `co_yield`, `co_return`. Функция с любым из них — coroutine. Обычный `return` в такой функции запрещён (нужен `co_return`).

## Ограничения и стоимость

Аллокация frame (часто на каждый вызов coroutine). Отладка сложнее, чем у линейного кода. Нельзя просто «включить coroutines» в hot loop без профилирования. Нельзя смешивать неосторожно с mutex: при suspend lock может держаться — deadlock.

## Связь с future

Стандарт не даёт `co_await std::future` из коробки в C++20 так просто, как в других языках. Библиотеки предоставляют awaitable обёртки. Для C++17-проекта остаёшься на future/get.

## Типичные заблуждения

«Coroutine = thread» — нет, много coroutines на одном потоке возможно. «co_await всегда async I/O» — зависит от awaitable. «Включил C++20 — coroutines бесплатны» — нет.

<!-- expand:ch11-coroutines-basics.md -->
## Развёрнутый разбор

### Coroutines C++20 — развёрнуто

**1.** В репозитории `training` эта тема стыкуется с tom 3 (потоки, mutex) и с §6 `THEORY.md` (async/future). Упражнение oracle — минимальный пример результата из другого потока без ручного promise. Тема главы: Coroutines C++20.

**2.** На Fedora собирай с `-std=c++17 -pthread`. Для coroutines **(C++20)** нужен отдельный флаг и том 5. Тема главы: Coroutines C++20.

**3.** Не спойлерь `multithread/solutions/`: книга описывает паттерны, не подставляет готовые advanced-реализации. Тема главы: Coroutines C++20.

**4.** При отладке зависания на `get()` смотри все потоки в gdb (том 6). Если future не спасает от гонки на общих данных — TSan (том 7). Тема главы: Coroutines C++20.

**5.** Код в примерах короткий; в проекте дроби длинные цепочки на функции с говорящими именами. Тема главы: Coroutines C++20.

**6.** Вопрос «кто владеет объектом до join/get» — тот же, что в томе 2 про capture: init-capture и move часто безопаснее `[&]`. Тема главы: Coroutines C++20.

**7.** Политика `std::launch::async` важна там, где тест или контракт требует **другого** `std::thread::id` — как в oracle. Тема главы: Coroutines C++20.

**8.** `shared_future` — когда много читателей **одного** значения; очередь — когда каждому нужен свой элемент. Тема главы: Coroutines C++20.

**9.** Packaged_task связывает отложенный запуск с future; пул ограничивает число потоков — complementary, не взаимозаменяемые автоматически. Тема главы: Coroutines C++20.

**10.** Executor — ответ на «куда submit’ить»; C++17 STL не даёт единого std::execution — пул и Asio закрывают дыру. Тема главы: Coroutines C++20.

**11.** Coroutines **(C++20)** — suspend/resume, не «бесплатный поток»; `co_await` зависит от awaitable. Тема главы: Coroutines C++20.

**12.** Блокирующий read в worker + future — простой C++17 путь; event loop — масштаб I/O. Тема главы: Coroutines C++20.

**13.** К собеседованию: уметь проговорить shared state, broken_promise, деструктор future от async, разницу deferred/async. Тема главы: Coroutines C++20.

**14.** Дальше по серии: том 5 (флаги), 6 (gdb), 7 (sanitizers). Тема главы: Coroutines C++20.

### Связь с oracle и THEORY.md

Oracle (`03_oracle`) и §6 `multithread/THEORY.md` — общий фундамент: результат из другого потока, явный `launch::async`, альтернатива promise+thread.

<!-- expand2 -->
## Углубление: coroutines

Пункт **1** (coroutines). Стандарт C++17 в репозитории `training` — база упражнений; C++20 coroutines помечены отдельно, чтобы не смешивать «как будто всё было в 17».
Пункт **2** (coroutines). Конспект `multithread/THEORY.md`, §6, разбирает `async`, `future`, `promise` и сценарии, когда async не подходит. Перечитывай §6 после глав 2–4.
Пункт **3** (coroutines). Упражнение `multithread/beginner/03_oracle/` требует другой поток и `.get()`; README рекомендует `std::launch::async`. Книга не подставляет код из `multithread/solutions/`.
Пункт **4** (coroutines). Том 3 даёт mutex, condition_variable, atomics и идеи пула/очереди; том 4 не отменяет синхронизацию общих данных — future защищает только канал одного результата.
Пункт **5** (coroutines). Том 2 про лямбды: capture `[&]` в фоне опасен, init-capture переносит копию; это критично для `async` и `packaged_task`.
Пункт **6** (coroutines). На Fedora: `g++ -std=c++17 -pthread`, mdBook для EPUB — `books/vol-04-async`, команда `mdbook build` (том 0).
Пункт **7** (coroutines). Деструктор `std::future` от `async` может блокировать — не fire-and-forget без плана (глава 7).
Пункт **8** (coroutines). `std::launch::deferred` выполняет work на потоке, который вызвал `get` — не путать с фоновым параллелизмом.
Пункт **9** (coroutines). `broken_promise` — promise уничтожен без set; `future_error` на второй `get`.
Пункт **10** (coroutines). Исключение из worker в `async` всплывает в `get()` вызывающего — ловить там (глава 6).
Пункт **11** (coroutines). Пул ограничивает потоки; лес `async` в цикле — red flag на собесе и в профиле.
Пункт **12** (coroutines). Packaged_task + пул — когда нужен future и контроль момента запуска; oracle — когда хватает async.
Пункт **13** (coroutines). Promise связывает void-колбэк библиотеки с blocking `get()` в main.
Пункт **14** (coroutines). Executor — «куда submit»; ручной пул из tom 3 — executor без std::execution в C++17.
Пункт **15** (coroutines). Asio `io_context` + strand — модель потоков для I/O handlers (глава 10, 13).
Пункт **16** (coroutines). Coroutines: `promise_type` coroutine ≠ `std::promise`; имена совпадают, сущности разные (глава 11).
Пункт **17** (coroutines). `co_await` приостанавливает coroutine, не обязательно освобождая поток — зависит от awaitable.
Пункт **18** (coroutines). Блокирующий read в thread + future прост для клиента; websocket-сервер на чистом async — нет.
Пункт **19** (coroutines). TSan (том 7) ловит гонки, которые future не предотвращает на ваших структурах.
Пункт **20** (coroutines). Gdb (том 6): все потоки, backtrace worker, breakpoint на `get`.
Пункт **21** (coroutines). К собесу: shared state, wait vs get, shared_future, когда promise вместо async.
Пункт **22** (coroutines). Миф: «future thread-safe для всего приложения» — только протокол shared state future.
Пункт **23** (coroutines). Миф: «coroutine = thread» — many coroutines on one thread возможны.
Пункт **24** (coroutines). Дальше: том 5 toolchain, 6 gdb, 7 sanitizers, 8 OOP/SOLID.

<!-- expand3 -->
### compiler state machine

Each co_await split point; frame on heap often.

### initial_suspend final_suspend

Controls start/end suspension policy.

### unhandled_exception

Goes to promise_type method; not terminate in coroutine body if handled.

### vs thread

Many coroutines one thread; thread stack separate.

<!-- expand4 -->
## Практика и собес: coroutines

1. co_await vs thread.
2. promise_type vs std::promise.
3. frame аллокация.
4. co_return vs return.
5. await_ready suspend resume.
6. initial_suspend смысл.
7. mutex при suspend danger.
8. std future co_await не из коробки.
9. C++17 flag нет co_await.
10. Когда coroutine overkill.

<!-- expand5 -->
## Стандарт, ревью, серия (ch11 coroutines basics)

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

Coroutines — suspend/resume с state machine от компилятора. Нужен тип с `promise_type`. `co_await` ждёт awaitable. C++17 без этого; для сети смотри библиотеки + главу 12. Дальше — генераторы и task types.

## Вопросы для самопроверки

1. Чем coroutine отличается от `std::thread`?
2. Зачем у return type coroutine есть `promise_type`?
3. Что делает `co_yield` в генераторе?
4. Можно ли в C++17 писать `co_await`?
5. Почему имена promise путают с `std::promise`?

## Обсуждение с репетитором

**Том:** 4 · **Глава:** `ch11` · **Файл:** `ch11-coroutines-basics.md`

**Дальше по курсу:** [Глава 12. Генераторы и task-типы](./ch12-generators-tasks.md)
