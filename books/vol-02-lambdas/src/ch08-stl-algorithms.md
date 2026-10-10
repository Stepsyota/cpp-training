# Глава 8. Лямбды и алгоритмы STL

## Зачем эта тема

Стандартные алгоритмы — `std::sort`, `find_if`, `for_each`, `transform`, `accumulate` с custom op — принимают **callable** последним аргументом или как predicate. Лямбда — самый короткий способ передать **локальную логику** без имени функции в namespace. Понимание сигнатур predicate и компаратора избавляет от «почему sort не компилируется» и от лишних копий в `for_each`.

Глава не каталог всех алгоритмов — только **паттерны** с лямбдами в духе C++17 и связь с захватом из глав 3–4.

## Компаратор для sort

```cpp
std::vector<std::string> names{"bob", "alice", "zoe"};
std::sort(names.begin(), names.end(),
    [](const std::string& a, const std::string& b) {
        return a.size() < b.size();
    });
```

Компаратор должен задавать **strict weak ordering**: транзитивность, irreflexivity для `comp(a,a)`, связность для эквивалентности. Захват `[&names]` не нужен, если сравниваешь только параметры `a`, `b`. Если нужен внешний `locale` или порог `max_len`, захватывай явно `[&locale, max_len]`.

Ошибка: вернуть `int` вместо bool — иногда компилируется через conversion, но стиль опасный. Явный `bool` и понятные имена параметров помогают review.

## Predicate для find_if / count_if

```cpp
auto it = std::find_if(v.begin(), v.end(),
    [](int x) { return x % 2 == 0; });
```

Predicate — callable, возвращающий **bool** или contextually convertible to bool. Не путай с unary function `void(int)` — алгоритм ожидает условие.

Захват счётчика в синхронном проходе:

```cpp
int n = 0;
std::for_each(v.begin(), v.end(), [&](int) { ++n; });
```

`n` меняется через ссылку — осознанный side effect; лямбда не уходит за lifetime `n`.

## transform и генерация

```cpp
std::transform(in.begin(), in.end(), out.begin(),
    [](int x) { return x * x; });
```

Unary op: один аргумент — элемент из input iterator. Захват `[factor]` для масштаба — копия или ссылка по правилам lifetime.

## remove / erase idiom

```cpp
v.erase(
    std::remove_if(v.begin(), v.end(), [](int x) { return x < 0; }),
    v.end());
```

Лямбда только predicate; идиома erase-remove — отдельная тема STL. После `remove_if` «удалённые» элементы в хвосте — не полагайся на их значения.

## Захват тяжёлых объектов в алгоритме

```cpp
std::regex pat{R"(^\d+$)"};
std::copy_if(lines.begin(), lines.end(), out,
    [&pat](const std::string& line) {
        return std::regex_match(line, pat);
    });
```

`pat` по ссылке: алгоритм **синхронный**, lifetime `pat` покрывает весь вызов — OK. Для async обработки строк — копия regex (дорого) или `shared_ptr` на общий regex с sync.

## Комparator vs predicate: сигнатуры

**Binary predicate** для `sort`: два аргумента типа элементов (или convertible). **Unary predicate** для `find_if`: один аргумент. Перепутать arity — «no matching function». Const-ref на элементы — норма для больших типов.

## Лямбда и execution policies (C++17)

`std::sort(std::execution::par, ...)` и parallel algorithms требуют **associative** операций там, где это указано в стандарте; predicate и user callbacks должны быть **без гонок** на захваченной mutable state. `[&counter]` без atomic в `count_if` с `par` — data race. Том 3 углубит; здесь — красный флаг при захвате shared state.

## Лямбда vs function pointer в C API

`qsort` ждёт `int (*)(const void*, const void*)`. Лямбда **с capture** не подойдёт. Stateless lambda без capture иногда конвертируется — но для `qsort` всё равно нужен C calling convention и cast. В C++ коде предпочитай алгоритмы STL, не `qsort`.

## accumulate и custom binary op

```cpp
int sum = std::accumulate(v.begin(), v.end(), 0,
    [](int a, int b) { return a + b; });
```

Binary op захватывает нечего, если всё в параметрах. Для «sum + weight from capture» — `[w](int a, int b) { return a + w * b; }` следи, что форма соответствует `accumulate` (left fold).

## partition и stable_partition

Predicate с состоянием (mutable «уже N элементов») ломает предположения алгоритма — не делай. Predicate должен быть чистой функцией от элемента для предсказуемости.

## Алгоритмы и const iterator

Компаратор получает `const value_type&` или ссылки из iterator category. Лямбда с `(auto& x)` в generic контексте — другая история; для `vector<int>` параметры `int` vs `const int&` — следи за сигнатурой в ошибке компилятора.

## Связь с range algorithms (C++20 preview)

В C++20 ranges принимают **views** и проекции; лямбда часто идёт как **projection** `sort(v, {}, [](auto x) { return x.key; })` **(C++20)** — не смешивай с C++17 кодом без пометки. Том не углубляется в ranges; паттерн «лямбда как projection» — та же идея predicate.

## Читаемость и размер лямбды

Если тело алгоритма — больше пятнадцати строк, вынеси именованный функтор или named function — review проще. Лямбда идеальна для одно-двух statement.

## all_of / any_of / none_of

```cpp
bool ok = std::all_of(v.begin(), v.end(),
    [](int x) { return x > 0; });
```

Unary predicate без capture — идиоматично. С порогом из окружения: `[t](int x){ return x > t; }`.

## sort с tie-break

```cpp
std::sort(v.begin(), v.end(), [](const Item& a, const Item& b) {
    if (a.key != b.key) return a.key < b.key;
    return a.seq < b.seq;
});
```

Лямбда длиннее — всё ещё OK; если повторяется — функтор `LessItem`.

## Связь с упражнением parallel map (идея)

Parallel обработка кусков массива передаёт **function** на элемент; часто лямбда `[&f](const T& x){ return f(x); }` на куске — без деталей твоего `parallel_map.hpp`. Важно: куски не пересекаются, запись в `out` по индексам — отдельная логика; лямбда только unary op.

## nth_element

Same comparator rules as sort; lambda with capture threshold OK sync.

## Типичные заблуждения

«for_each быстрее range-for» — не обязательно, зависит от компилятора. «Захват [&] в sort безопасен всегда» — только пока живут referents и вызов синхронный. «Алгоритм всегда копирует лямбду один раз» — implementation-defined; обычно хранит по const ref внутри. «parallel sort с [&] на vector» — vector не меняется, но counter в capture — опасен.

## minmax_element и tie

Comparator как sort; лямбда с capture locale для case-insensitive compare — capture by ref sync OK.

## Custom iterator + algorithm

Если iterator custom, lambda signature unchanged — still predicate on `*it`.

## Практика без спойлера

Воз возьми `vector<int>`, `sort` с lambda, `find_if` с lambda — 10 минут на Fedora после главы. Muscle memory важнее ещё пяти абзацев.

## Краткое резюме

Лямбды — естественные comparator и predicate для STL. Явный захват для внешнего состояния; следи за lifetime и thread-safety при parallel. Сигнатура должна совпадать с ожиданием алгоритма.

## Параграф для Kindle (закрепление)

Перечитай ключевую мысль главы без кода: запиши одно предложение от руки. Для этой главы сформулируй, **что** должно остаться в памяти после чтения, и **какую ошибку** ты больше не хочешь повторять в своём `.cpp`. Связь с треком multithread — только теория; код упражнений по-прежнему твой.

## Вопросы для самопроверки

1. Что должен возвращать компаратор для `sort`?
2. Зачем `[&]` в `for_each` для счётчика?
3. Безопасен ли `[&pat]` в async `copy_if`?
4. Можно ли передать захватывающую лямбду в `qsort`?
5. Чем predicate отличается от произвольного `void(int)`?
6. Почему `execution::par` конфликтует с `[&total]++`?
7. Когда лучше именованный функтор вместо лямбды в sort?

## Обсуждение с репетитором

**Том:** 2 · **Глава:** `ch08` · **Файл:** `ch08-stl-algorithms.md`

**Дальше по курсу:** [Глава 9. Потоки и async](./ch09-threads-async.md)
