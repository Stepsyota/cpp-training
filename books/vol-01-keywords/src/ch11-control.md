# Глава 11. Управление потоком и coroutines

## Зачем эта тема

Пока компилятор не знает, **куда прыгнуть** дальше, программа — просто список объявлений. Keywords потока управления — **`if`**, **`else`**, **`switch`**, **`case`**, **`default`**, **`for`**, **`while`**, **`do`**, **`break`**, **`continue`**, **`return`**, **`goto`** — задают ветвление и циклы. Рядом **`try`**, **`throw`**, **`catch`** передают управление через **исключения**; контракт «не бросаю» выражается **`noexcept`** (глава 6), не этой тройкой.

С **(C++20)** появились **`co_await`**, **`co_yield`**, **`co_return`**: одна функция может **приостановиться** и продолжиться позже — **coroutines**. Их часто путают с **`std::thread`** и **`std::async`** из трека multithread: coroutine не равна «новому потоку ОС»; это state machine, которую генерирует компилятор.

Глава фиксирует **keywords языка**. Range-for, init-statement в `if`/`switch`, атрибут **`[[fallthrough]]`** — стандарт C++17, если не помечено иначе. Coroutines и **`if consteval`** **(C++23)** требуют более нового `-std=`.

## `if` и `else`: условие и **(C++17)** init-statement

Классическая форма: **`if`** вычисляет **condition** (приводится к `bool`), выполняет ветку или переходит к **`else`**.

```cpp
if (ready && !stopped)
    process_one();
else
    wait_idle();
```

**Init-statement** **(C++17)** ограничивает область видимости переменной **только веткой**, где она нужна:

```cpp
if (auto p = lookup(key); p != nullptr) {
    use(*p);
} else if (auto q = fallback(key); q) {
    use(*q);
}
// p и q здесь не видны
```

Это уменьшает «утечку» имён в наружный блок и снижает риск случайно использовать «полуинициализированный» указатель снаружи. **`else if`** с init — отдельные init на каждую цепочку; читай стандарт, если комбинируешь сложно.

**(C++20)** для некоторых типов условие в `if` может использовать **`explicit bool`** conversion более строго — для обычных `int` и указателей правила знакомые: ноль и `nullptr` — false.

## `switch`, `case`, `default`

**`switch`** сравнивает **condition** с **integral**, **enum** (в т.ч. **enum class** после приведения), **`char`** и т.п. Строки и `std::string` напрямую нельзя — нужен другой dispatch.

```cpp
enum class Kind { A, B, C };

void handle(Kind k) {
    switch (k) {
    case Kind::A:
        work_a();
        break;
    case Kind::B:
        work_b();
        break;
    default:
        work_unknown();
        break;
    }
}
```

**`break`** выходит из ближайшего **`switch`** или цикла. Без **`break`** выполнение **проваливается** в следующий **`case`** — иногда намеренно, чаще баг.

Init-statement **(C++17)**:

```cpp
switch (auto v = parse(); v.kind) {
case Kind::A:
    break;
default:
    break;
}
```

**`[[fallthrough]]`** **(C++17)** — **атрибут**, не keyword: помечает намеренный провал между `case`, подавляет предупреждение компилятора.

```cpp
case 1:
    prep();
    [[fallthrough]];
case 2:
    run();
    break;
```

## Циклы: `while`, `do`, `for`, range-for

**`while (cond)`** проверяет условие **до** тела. **`do { ... } while (cond);`** — **минимум одна** итерация, проверка в конце.

```cpp
while (!queue.empty()) {
    auto job = queue.pop();
    job();
}

int code = 0;
do {
    code = try_step();
} while (code == RETRY);
```

Классический **`for (init; cond; inc)`** — три части в одном заголовке. **Range-based for** **(C++11)** — синтаксический сахар с **`for`** и **`auto`** (не отдельный keyword «foreach»):

```cpp
for (const auto& x : container)
    sum += x;
```

В multithread range-for по очереди без синхронизации не заменяет **`mutex`**: итератор контейнера и сам контейнер должны быть защищены, если другой поток меняет его.

**`continue`** переходит к следующей итерации цикла; **`break`** выходит из цикла или **`switch`**.

## `return` и `goto`

**`return`** завершает функцию; в **`main`** **`return 0;`** — успешный код выхода (можно опустить в конце). В coroutine см. **`co_return`**.

**`goto`** прыгает на **метку** в той же функции:

```cpp
if (error) goto cleanup;
// ...
cleanup:
    release_handles();
```

В современном C++ **`goto`** редок; иногда один выход из вложенных циклов и C interop. Не путай с метками в других языках; **`goto`** не пересекает границы функции и не инициализирует переменные «мимо» их scope так, чтобы обойти правила C++.

## Исключения: `try`, `throw`, `catch`

**`try`** блок оборачивает код; **`throw`** выражение создаёт объект исключения и начинает **stack unwinding** до подходящего **`catch`**.

```cpp
try {
    risky_parse(path);
} catch (const std::invalid_argument& e) {
    log(e.what());
} catch (const std::exception& e) {
    log(e.what());
} catch (...) {
    log("unknown failure");
}
```

Ловить **полиморфные** типы **по значению** (`catch (std::exception e)`) опасно — **slicing**: теряется реальный тип. Идиома — **`const&`**.

**`catch (...)`** ловит всё; пустой handler скрывает баги — хотя бы логируй и **`std::rethrow_exception`**, если пробрасываешь дальше.

Динамический exception specification **`void f() throw(int);`** **устарело**; замена — **`noexcept`**. **`throw()`** как «не бросает» — легаси; пиши **`noexcept`**.

Исключения **не заменяют** синхронизацию потоков: data race от **`++`** без mutex не лечится **`try`**.

## Coroutines **(C++20)**: три keywords

Если в теле функции есть **`co_await`**, **`co_yield`** или **`co_return`**, функция становится **coroutine**: компилятор генерирует **promise**, **frame** и state machine. Нужны заголовки вроде `<coroutine>` и тип **`promise_type`** — это уже библиотека и соглашения, не отдельные keywords.

### `co_await`

Приостановка, пока **awaitable** не готов (тип с **`operator co_await`** или через ADL):

```cpp
// псевдо-API: task — ваш или библиотечный тип
task<void> run_session() {
    co_await async_read_config();
    co_await process_jobs();
}
```

**`co_await`** **не создаёт** поток ОС. Планировщик coroutine может крутиться в одном потоке; параллелизм — отдельное решение (executor, thread pool — tom 3–4).

### `co_yield`

Выдать значение и **приостановиться** — паттерн генератора:

```cpp
// упрощённый sketch; реальный generator — библиотека или C++23
generator<int> iota_to(int n) {
    for (int i = 0; i < n; ++i)
        co_yield i;
}
```

**(C++23)** **`std::generator`** в стандарте — проверяй **`g++ --version`** и `-std=c++23`; на C++17 проекта не опирайся.

### `co_return`

Завершение coroutine; может передать значение в promise:

```cpp
task<int> compute() {
    co_return 42;
}
```

Обычный **`return`** в coroutine с **`co_await`** в теле — ошибка; завершение через **`co_return`**.

## **`asm`** и прочие края

Keyword **`asm`** — **implementation-defined** встроенный ассемблер:

```cpp
asm volatile("nop" :::);
```

Не переносимо между компиляторами и архитектурами; в учебном прикладном коде не нужен.

## **(C++23)** `if consteval`

Ветвление **по контексту вычисления**: выполняется ли блок в **constant evaluation** или в runtime:

```cpp
consteval int square(int n) { return n * n; }

template<int N>
constexpr int pick() {
    if consteval {
        return square(N);
    } else {
        return N * N;  // runtime path в constexpr function
    }
}
```

Keywords **`if`** и **`consteval`** **(C++20)** вместе — отдельная грамматика C++23; на `-std=c++17` не соберётся.

## Как не смешивать с multithread

**`std::thread`** + **`join`** — поток ОС. **`std::async`** + **`future`** — асинхронный результат, часто с другим потоком. **`co_await`** — приостановка **coroutine** в рамках модели компилятора. **`condition_variable::wait`** — блокировка потока ОС под mutex. Таблица в голове: «что блокирует поток?», «кто создаёт параллелизм?», «где lifetime?».

**`break`** из цикла опроса очереди в кузнице — нормальный выход воркера при **`stopped && jobs.empty()`**; это не exception и не coroutine.

## Типичные ошибки

**`switch`** без **`break`** — случайный fallthrough. Использовать переменную из **`if (auto x = …)`** снаружи блока — ошибка компиляции, но copy-paste init иногда ломает рефакторинг. Ловить **`catch (...)`** и молчать. Думать, что **`co_await`** == новый **`std::thread`**. Путать **`return`** и **`co_return`** в одной coroutine. **`goto`** через инициализацию локальной переменной, которую потом используют — нарушение правил scope. Range-for по контейнеру, который другой поток меняет без синхронизации — data race, не «проблема for».

## Краткое резюме

**`if`/`else`**, **`switch`/`case`/`default`**, циклы и **`break`/`continue`/`return`/`goto`** задают структурный поток. **(C++17)** init-statement в **`if`** и **`switch`** сужает scope; **`[[fallthrough]]`** документирует провал. **`try`/`throw`/`catch`** — исключения; лови полиморфные типы по **`const&`**. **(C++20)** **`co_await`**, **`co_yield`**, **`co_return`** делают функцию coroutine; это не поток ОС. **(C++23)** **`if consteval`** разделяет compile-time и runtime ветки. **`asm`** — платформенный край.

## Вопросы для самопроверки

1. Где видна переменная из `if (auto x = f(); g(x))`?
2. Зачем **`[[fallthrough]]`**, если можно просто не писать **`break`**?
3. Какое keyword (или любое из трёх) делает функцию coroutine?
4. Почему **`catch (std::exception e)`** по значению опасно?
5. Чем **`co_await`** принципиально отличается от **`std::thread`**?

## Обсуждение с репетитором

**Том:** 1 · **Глава:** `ch11` · **Файл:** `ch11-control.md`

**Дальше по курсу:** [Глава 12. Модули import/export](./ch12-modules-export.md)
