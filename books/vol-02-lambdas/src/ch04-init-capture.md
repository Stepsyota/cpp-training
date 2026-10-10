# Глава 4. Init-capture и move в лямбду

## Зачем эта тема

Default capture **`[=]`** копирует всё подряд, что используется в теле. Копирование — не всегда возможно и не всегда желательно. Иногда нужно **переместить** `unique_ptr` внутрь лямбды, инициализировать поле сложным выражением, переименовать захват или вычислить константу **один раз** при создании closure, а не при каждом вызове `operator()`.

С **C++14** для этого есть **init-capture**: имя в списке захвата с инициализатором, синтаксис как у поля struct с default member initializer, только в `[ ]`. Без init-capture в C++11 приходилось оборачивать `unique_ptr` в `shared_ptr`, хранить сырой указатель в vector или писать отдельный struct-функтор — шум и лишние аллокации. Init-capture — идиоматичный способ передать **владение** в задачу для `std::thread` или в очередь пула **на уровне идеи** (без разбора твоего `submit`).

## Синтаксис

```cpp
auto ptr = std::make_unique<int>(42);
auto job = [p = std::move(ptr)]() {
    return *p;
};
// ptr теперь nullptr
```

Имя `p` — **новое** имя члена closure; справа — любое выражение. Move-only типы так захватываются легально; копировать `unique_ptr` в `[=]` нельзя — компилятор выдаст ошибку.

Другие формы:

```cpp
[x = expensive()] { use(x); }
[counter = 0] { return ++counter; }
[data = std::vector<int>(1000, 1)] { ... }
```

Внутри тела используешь `p`, `x`, `counter` как обычные локальные имена — они на самом деле поля объекта.

## Когда выполняется инициализатор

**Один раз** — в момент **конструирования** closure object, когда выполняется строка с лямбдой. Каждый вызов `job()` не пересчитывает `expensive()` слева от `=`. Если нужно «лениво» при каждом вызове — вызывай функцию **внутри тела** или храни callable отдельно.

Сценарий: таймер стартует при создании задачи, а не при каждом retry:

```cpp
auto task = [started = std::chrono::steady_clock::now()] {
    return std::chrono::steady_clock::now() - started;
};
```

## Init-capture vs параметр функции

Параметр `std::unique_ptr<T> p` в функции, которая **возвращает** лямбду, тоже передаёт владение в closure при return. Init-capture удобен, когда лямбда создаётся **на месте** и сразу уходит в `thread`, `emplace_back` или `push` очереди:

```cpp
void launch(std::unique_ptr<Work> w) {
    std::thread([work = std::move(w)]() {
        work->run();
    }).detach();  // учебный пример; detach осторожно в проде
}
```

Параметр `w` move'ится в поле `work` closure; отдельный struct не нужен. В реальном коде чаще `join` и RAII вместо `detach`.

## Копия с переименованием

```cpp
int id = 7;
auto f = [id_copy = id]() { return id_copy; };
```

Явно видно: в closure попала копия на момент создания; внешний `id` можно менять независимо. Это читаемее, чем `[=]` среди десяти других имён.

## Захват move-only и `[*this]` (C++17)

`[*this]` создаёт **копию** текущего объекта в closure — альтернатива `[this]`, когда задача должна пережить метод и копия объекта осмысленна. Init-capture обобщает идею «поле = выражение при конструировании closure» для произвольных типов, не только `*this`.

Нельзя написать `[p = ptr]` для `unique_ptr` без `std::move` — копирование запрещено.

## Init-capture и mutable

Если захват `[counter = 0]` и нужно `++counter` в теле, всё равно может понадобиться **`mutable`**, потому что `operator()` по умолчанию const и const не может менять поля. См. главу 5: init-capture не отменяет const на `operator()`.

## Ограничения и диагностика

Init-capture нельзя смешать с дублированием того же имени. Выражение справа должно быть well-formed. GCC при попытке `[=]` с move-only членом подскажет про move или init-capture.

Для «захватить ссылку с новым именем» иногда пишут `[ref = std::ref(x)]` — это обёртка, lifetime referent всё равно на тебе (глава 10).

## Связь с `std::move` из THEORY

В `multithread/THEORY.md` move — «метка для перегрузки», не «физически перенёс байты в другое место на диске». Init-capture — типичное место, где `std::move` **обязателен**, иначе компилятор попытается copy-construct и откажет для move-only типов. После move исходная переменная (`ptr`) остаётся в valid-but-unspecified state — ею не пользуются, кроме присваивания и destroy.

## Generalized capture и structured bindings (идея)

Init-capture может инициализировать несколько «логических» полей через struct:

```cpp
auto pack = [pair = std::make_pair(1, 2)] {
    return pair.first + pair.second;
};
```

**(C++17)** можно захватить результат structured binding через init-capture от tuple — паттерн для «разложить и унести в worker» без отдельного struct в namespace.

## Move и const

Move из `const T` не move — копия, если T copyable. Init-capture с `std::move` требует **non-const** source для move-only типов. Если accidentally захватил `const auto&` на unique_ptr — move не сработает.

## Захват exception_ptr и shared state (обзор)

`[ep = std::current_exception()]` — редкий паттерн для передачи исключения между контекстами; lifetime `exception_ptr` управляется стандартом. Не путать с захватом ссылки на локальный `std::exception` — снова dangling.

## Сравнение с returning lambda from factory

Функция `auto make() { return [v = std::move(x)]{}; }` move'ит в closure member при return (RVO/NRVO на closure object). Init-capture в return path — тот же механизм, что и локальная лямбда.

## Сценарий: задача с ресурсом

Файловый дескриптор или буфер в `unique_ptr`: владение передаётся в worker одним move в capture — worker уничтожает closure в конце и освобождает ресурс. Без init-capture пришлось бы делить `shared_ptr` или дублировать API.

## Паттерн «guard» с unique resource

```cpp
auto guard = [g = std::move(guard_obj)]() mutable {
    g.release();  // если API mutable
};
```

Move guard в closure и вызов при scope exit через RAII wrapper — composition лямбды и destructors. Init-capture — ключ move-only guard в callable.

## Ошибка: capture structured binding **(C++17)**

```cpp
auto [a, b] = pair;
// [a, b] в capture до C++20 — нельзя напрямую;
// [cap = pair] или захват pair by value
```

Стандарт эволюционировал; на C++17 часто init-capture всего tuple/pair одним полем.

## Copy elision and closure

Return `[x = std::move(val)]{}` from factory — NRVO on closure optional; move into member still happens.

## Brace init in capture

`[v = std::vector{1,2,3}]` **(C++17)** list init in capture — convenient for tests.

## Типичные заблуждения

«Init-capture выполняется при каждом вызове лямбды» — нет, при создании объекта. «Можно `[ptr = ptr]` без move для unique_ptr» — нет. «Init-capture заменяет default `[=]`» — дополняет, не заменяет. «Move в capture копирует объект» — move-construct поля closure.

## Init-capture и designated initializers **(C++20 struct)**

Если closure «как struct» с несколькими полями, init-capture list — аналог aggregate init members. Читаемость: длинный capture list лучше, чем `[=]` + hidden copies.

## Performance note

Init once at construction means expensive work **не** в hot `operator()` loop sort comparator — хорошо. Если expensive в init каждой итерации цикла `for (...) { auto L = [x=expensive()]{...}; }` — плохо: создаёшь closure каждый раз.

## Связь с queue push

Bounded queue conceptually `push(T&& item)` move'ит элемент. Callable с `[payload = std::move(item)]` move'ит в closure — та же философия владения, что move в queue, без кода factory.

## Краткое резюме

**(C++14)** `[id = expr]` создаёт член closure с инициализацией; move-only типы — через `std::move`. Выражение один раз при создании лямбды. Идиома для владения в асинхронных задачах и очередях.

## Параграф для Kindle (закрепление)

Перечитай ключевую мысль главы без кода: запиши одно предложение от руки. Для этой главы сформулируй, **что** должно остаться в памяти после чтения, и **какую ошибку** ты больше не хочешь повторять в своём `.cpp`. Связь с треком multithread — только теория; код упражнений по-прежнему твой.

## Вопросы для самопроверки

1. Когда вычисляется `expensive()` в `[x = expensive()]`?
2. Почему `[=]` не подходит для `unique_ptr`?
3. Чем init-capture отличается от поля struct с конструктором?
4. Можно ли после `[p = std::move(ptr)]` безопасно разыменовывать `ptr`?
5. Зачем писать `[id_copy = id]` вместо `[=]`?
6. Нужен ли `mutable` для `++counter` при `[counter = 0]`?
7. Чем init-capture лучше «обернуть всё в shared_ptr»?

## Обсуждение с репетитором

**Том:** 2 · **Глава:** `ch04` · **Файл:** `ch04-init-capture.md`

**Дальше по курсу:** [Глава 5. mutable](./ch05-mutable.md)
