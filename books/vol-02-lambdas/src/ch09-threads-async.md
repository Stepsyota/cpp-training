# Глава 9. Лямбды, потоки и async

## Зачем эта тема

`std::thread` и `std::async` принимают **callable** — часто лямбду — и запускают его в другом контексте выполнения. Ошибки здесь не синтаксические: **висячий захват**, гонка на захваченной ссылке, уничтожение объекта с `[this]` пока поток ещё крутится. Ты уже проходил `01_rocket_launch` и `03_oracle`; эта глава формулирует **инварианты** без разбора кода `thread_pool.hpp` и `bounded_queue.hpp` из упражнений.

Том 3 даст memory order и модель памяти; здесь — только callable, захват, join и связь с future.

## std::thread с лямбдой

```cpp
void worker(int id) {
    std::thread t([id]() {
        process(id);
    });
    t.join();
}
```

Копия `int` в closure переживает стек `worker` **до** join: поток может читать `id` из своей копии в closure после того, как `worker` вернулся бы без join — но мы join'им до выхода. Если **detach** без синхронизации — closure всё ещё живёт в потоке, копия `id` OK, но доступ к **общим** данным без mutex — data race.

Паттерн «запустил thread с лямбдой и забыл join» — `std::terminate` при уничтожении joinable `std::thread` (из твоего трека: ручка потока, не «ядро ОС»).

## Захват по ссылке и join

```cpp
void bad_if_no_join() {
    int data = 0;
    std::thread t([&data]() { data = 1; });
    t.join();
}
```

Пока `t` joinable и не joined, `data` должен жить. **Join в той же области**, что и захваченные ссылки — минимальный безопасный паттерн. Вынести `thread` наружу без переноса `data` — опасно.

## std::async и лямбда

```cpp
auto fut = std::async(std::launch::async, [x = compute()]() {
    return heavy(x);
});
int r = fut.get();
```

Init-capture копирует или переносит состояние в closure; `get()` synchronizes и возвращает результат. Политика **`launch::deferred`** откладывает выполнение до `get()` — лямбда должна быть валидна до тогда; захват by reference на локальные из `compute()` не должен висеть.

`03_oracle` — та же идея: результат из другого контекста; захват должен пережить выполнение callable.

## packaged_task и promise (обзор)

`std::packaged_task<R(Args...)>` оборачивает callable; внутри может быть лямбда. Связь с future — как в oracle: не теряй exception safety; захват в лямбде внутри task должен быть valid at invoke time.

## Пул потоков — идея без спойлера

Воркер в цикле ждёт задачу и вызывает **callable из очереди**. Задача часто типа «void()» через erasure или шаблон с move-only closure. Инварианты:

- **Владение** задачей передаётся в очередь (move, init-capture), а не ссылка на локальную лямбду в `submit`.
- Воркер с `[this]` читает поля пула; пул не уничтожают, пока воркеры не завершены и очередь не drained — **join** в деструкторе **без** удержания того же mutex, что ждёт воркер (deadlock из твоего опыта с кузницей — на уровне идеи, не эталон).

## async vs thread для возврата значения

`thread` не возвращает `int` напрямую — нужен promise, shared state или capture в общую переменную под mutex (плохо). `async` + future — естественнее для «посчитай и верни». Лямбда в обоих случаях — carrier behavior + state.

## Что копируется в новый поток

Closure object **move/copy** в память, доступную новому потоку (implementation). Это не «лямбда стала функцией ОС» — это объект с `operator()`, который вызывает runtime thread start.

## condition_variable и лямбда (идея из трека)

В `04_elevator` predicate для `wait` — часто лямбда `[&] { return ready; }`. Predicate вызывается под mutex; `[&]` на `ready` безопасен, пока mutex и `ready` живут в том же объекте, что и waiting thread, и нет use-after-return. Это **не** async fire-and-forget — другой lifetime pattern, чем detach.

## jthread и stop_token **(C++20, пометка)**

**`std::jthread`** с **`stop_token`** в лямбде `[st = stop.get_token()] { ... }` — современная альтернатива «flag + join»; в репозитории training база C++17, но при чтении статей не путай с C++17. Init-capture token — тот же синтаксис, что глава 4.

## exception в worker lambda

Если лямбда в `thread` бросает исключение и не поймана — **`std::terminate`** (обычно). Пулы часто оборачивают `job()` в try/catch на уровне идеи «не убить воркер». Твой код кузницы — проверь сам; эталон не спойлерим.

## Чеклист перед thread + lambda

1. Все ссылки в capture живут до **последнего** вызова `operator()` в потоке (обычно до join).
2. `this` — объект жив; dtor не гоняется параллельно с job на `[this]`.
3. Тяжёлые строки: `[=]` осознанно; maybe move init-capture.
4. Исключения из лямбды: в пуле часто `try/catch` в job — идея из production, не спойлер твоего кода.

## rocket launch и join (отсылка к 01)

`01_rocket_launch` — другой поток + join; лямбда `[](...) { ... }` могла бы быть free function. Смысл упражнения — lifecycle `std::thread`, не capture. Когда добавишь захват — вернись к ch03.

## future from lambda vs packaged_task

Оба упаковывают callable; `async` convenience, `packaged_task` control. Capture rules **одинаковые**: state в closure must outlive invoke.

## Не путать: потоки ОС и объекты thread

Много `std::thread` с лямбдами — много потоков ОС; SMT 6/12 из THEORY не лимитирует число `std::thread`. Лямбда не «облегчает» потоки — только syntax job.

## hardware_concurrency hint

Not lambda topic but often same file: thread count vs capture copies — orthogonal.

## Типичные заблуждения

«Лямбда в thread копируется в регистры CPU» — копируется closure object. «async всегда создаёт поток» — зависит от policy и реализации. «`[=]` делает потокобезопасным» — нет, только копии; shared mutable state всё ещё гонка. «detach + [&] локального» — UB после выхода из функции.

## launch policy recap

`async` default may defer; `launch::async` request thread. Lambda must stay valid until invoked. Capture by value for data used inside after parent returns.

## thread + exception

Join thread to rethrow? `std::thread` destructor terminate if joinable — catch inside lambda before exit.

## Мост к тому 3

Lambda correctness necessary but not sufficient: mutex for shared data, atomics for counters — tom 3.

## Деталь: что копируется при `std::thread` constructor

Стандарт описывает запуск нового потока с callable, который будет **invoked** в новом execution agent. Реализация обычно move-construct или copy-construct closure в storage, доступное новому потоку — это **не** «копия всего стека родителя». Поэтому capture-by-value `std::string` в thread может аллоцировать дважды (copy into closure, then thread stack/heap). Для больших payload — `shared_ptr` к immutable data или move init-capture единственного owner.

## `async` и destructor of future

Если уничтожить `std::future` без `get`, destructor может block (deferred) или оставить task running (async policy) — зависит от implementation. Лямбда должна оставаться valid. Не захватывай referents, которые уничтожаются при уничтожении future holder раньше выполнения task.

## Краткое резюме

В потоках предпочитай копию и init-capture; `[&]` только при жёстком lifetime и join. `get()` на future синхронизирует. Пул — очередь callables и согласованность с lifetime `this`. Детали mutex — том 3.

## Согласованность с `03_oracle` и future

В oracle ты передавал callable в `async` и забирал результат через `future`. Лямбда там — носитель кода; **future** — канал результата. Capture должен пережить момент, когда runtime вызовет `operator()` лямбды, не момент создания `future`. Если лямбда захватила ссылку на локальную переменную функции, которая уже вернулась, `get()` может вернуть мусор или упасть — синхронизация future не лечит dangling referent.

Правило thumb: для `async` с `launch::async` предполагай, что тело может выполниться **после** return из функции, создавшей `async` — unless join-like поведение через `get()` в той же функции до return.

## Параграф для Kindle (закрепление)

Перечитай ключевую мысль главы без кода: запиши одно предложение от руки. Для этой главы сформулируй, **что** должно остаться в памяти после чтения, и **какую ошибку** ты больше не хочешь повторять в своём `.cpp`. Связь с треком multithread — только теория; код упражнений по-прежнему твой.

## Вопросы для самопроверки

1. Почему `[id]` безопаснее `[&id]` после выхода из функции без join?
2. Что должно жить дольше лямбды с `[&data]`?
3. Зачем move в задачу для очереди пула?
4. Чем `async` удобнее `thread` для возврата значения?
5. Опасен ли `[this]` в воркере пула концептуально?
6. Что будет с joinable `thread` в деструкторе?
7. Когда `launch::deferred` откладывает выполнение лямбды?

## Обсуждение с репетитором

**Том:** 2 · **Глава:** `ch09` · **Файл:** `ch09-threads-async.md`

**Дальше по курсу:** [Глава 10. Lifetime и ловушки](./ch10-lifetime-pitfalls.md)
