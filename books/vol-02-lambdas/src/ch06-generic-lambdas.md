# Глава 6. Generic lambda и template-параметры

## Зачем эта тема

Лямбда с фиксированными типами параметров `(int, int)` подходит для `sort` над контейнером `int`. Если нужен один callable для `int`, `double` и `std::string` без макросов и без десятка перегрузок free function, с **C++14** появились **generic lambda**: параметр объявляют с **`auto`**, и для каждого вызова компилятор инстанцирует соответствующий **member template** `operator()`.

С **C++20** можно явно писать **template parameter list** в `[]` — лямбда как мини-шаблон с **`concept`** и **`requires`**. Эта глава связывает лямбды с темой шаблонов из тома 1 без повторения всей SFINAE и не подменяет главу про dependent names — только callable-сторона.

## C++14: `auto` в параметрах

```cpp
auto print_twice = [](auto x) {
    std::cout << x << ' ' << x << '\n';
};
print_twice(1);
print_twice("ab");
```

Компилятор создаёт closure type с **шаблонным** `operator()` (упрощённо):

```cpp
template<typename T>
void operator()(T x) const { ... }
```

Один объект `print_twice`, много инстанциаций при разных `T` — как один функтор-шаблон, но синтаксис локальный.

Несколько параметров:

```cpp
auto add = [](auto a, auto b) { return a + b; };
```

Типы `a` и `b` могут различаться; `return` выводится через правила `operator+` и `decltype`; иногда нужен явный `-> decltype(...)`.

## Ограничения C++14 generic lambda

Нельзя частично «зашаблонить» только один параметр в старом стиле без C++20 — оба `auto` или явные типы. Generic lambda **не** превращается в указатель на функцию: тип closure уникален и шаблонный внутри.

Для type erasure в API — `std::function` (с ограничениями) или шаблон принимающей функции `template<typename F> void run(F&& f)`.

## C++20: template lambda

```cpp
auto cmp = []<typename T>(const T& a, const T& b) {
    return a < b;
};
```

**Template parameter list** сразу после `[` (или после capture). Можно писать:

```cpp
auto f = []<typename T>(T x) requires std::integral<T> {
    return x * 2;
};
```

Преимущество перед C++14 — явные ограничения, перегрузки по template params, ближе к обычным шаблонным функциям и к **`if constexpr`** в теле.

Проверка на Fedora: `g++ -std=c++20 -Wall` — без C++20 строка с `<typename T>` не компилируется.

## Generic lambda и захват

Захват **не** становится «шаблонным»: поля closure **фиксированы** при создании объекта. Шаблонируется только **вызов** по типам аргументов.

```cpp
int base = 10;
auto g = [base](auto x) { return base + x; };
```

`base` — `int` в closure; `x` — любой тип с подходящим `operator+`. Если `base` меняют снаружи после создания `g`, захваченная **копия** `base` не меняется (capture by value в init или explicit).

## Сравнение подходов (без широкой таблицы)

**Generic lambda** — один объект, нулевая обёртка на вызове, но тип closure уникален и тяжёлый для `.cpp` API без шаблонов. **`template<typename F> void run(F f)`** — inline и perf, но код в заголовке или explicit instantiation. **`std::function`** — стабильный тип в `.h`, цена erasure и аллокаций. Выбор: hot path → шаблон или concrete lambda; граница библиотеки → `function` или type erasure иной формы.

## Ошибки компиляции

«No matching call» — нет `operator+` для пары типов. «Ambiguous» — несколько перегрузок `operator()` после generic. С C++20 `requires` false — substitution failure, иногда длинное сообщение.

## Deduction guides и лямбда (контекст C++17)

Class template argument deduction **не** применяется к самой лямбде как к типу; но generic lambda внутри `vector v{...}` не связана напрямую. Важнее: generic lambda как аргумент шаблонной функции выводит `F` как closure type — один call site может породить тяжёлую instanciation в бинарнике; для embedded иногда избегают generic lambda в hot headers.

## Ограничения с overload resolution

Несколько `auto` параметров могут дать неоднозначность, если `operator+` существует для многих пар. C++20 `requires` сужает набор. Без concepts приходится явно cast в теле или trailing return.

## Лямбда vs шаблонная free function

```cpp
template<typename T>
void print_twice(T x) { ... }
```

Free function шаблон виден в symbol table; generic lambda — member template **внутри** closure type. Для linkage и debug symbols имена разные; perf часто идентичен при inline.

## Generic lambda в алгоритмах

`std::sort` с `[](auto a, auto b) { return a < b; }` работает, если итераторы дают тип, для которого `<` определён. Для heterogeneous compare осторожно — strict weak ordering должна быть на одном типе элементов контейнера.

## SFINAE и generic lambda (край)

Generic lambda member template участвует в overload resolution как шаблон; если тело ill-formed для `T`, substitution failure — candidate removed. С C++20 `requires` ошибки короче. На C++17 длинные ошибки в `auto` lambda — норма при первом знакомстве.

## Explicit template args на вызове

Для template lambda **(C++20)** иногда можно указать template args на вызове `f.operator()<int>(1)` — редко в application code; знать для metaprogramming.

## Тест на Fedora

Файл `lambda_generic.cpp`: generic lambda + `-std=c++17`; добавь строку с `[]<typename T>(T x)` и убедись, что без C++20 падает — закрепляет пометки в книге.

## Limitation: recursive lambda

Recursive call inside lambda needs `std::function` erasure or Y combinator trick — not natural; use named function for recursion.

## Типичные заблуждения

«Generic lambda — это `std::function`» — нет. «Один auto параметр = шаблон всей лямбды» — шаблонируется `operator()`, не capture. «C++20 template lambda везде в C++17» — нет, нужен стандарт. «Захваченный base шаблонный» — нет, только параметры вызова.

## Concepts **(C++20)** с template lambda

```cpp
auto f = []<std::integral T>(T x) { return x * 2; };
```

Сокращает ошибки при `T` = `std::string`. Требует `-std=c++20` и поддержку compiler.

## Лямбда внутри template function

```cpp
template<typename Container>
void dump(const Container& c) {
    std::for_each(c.begin(), c.end(), [](auto const& x) {
        std::cout << x << '\n';
    });
}
```

Generic lambda внутри template — double template instantiations; compile time растёт — нормально для header utilities.

## Итог главы 6

Generic lambda — не magic type erasure; это **шаблонный operator()** внутри одного closure object.

## Краткое резюме

**(C++14)** `auto` параметры → member template `operator()`. **(C++20)** `[]<typename T>(T x)` и `requires`. Захват остаётся конкретным типом closure. Для API выбирай между шаблоном и erasure.

## Параграф для Kindle (закрепление)

Перечитай ключевую мысль главы без кода: запиши одно предложение от руки. Для этой главы сформулируй, **что** должно остаться в памяти после чтения, и **какую ошибку** ты больше не хочешь повторять в своём `.cpp`. Связь с треком multithread — только теория; код упражнений по-прежнему твой.

## Вопросы для самопроверки

1. Что инстанцируется при `print_twice(1)` и `print_twice(1.0)`?
2. Зачем C++20 template lambda, если есть C++14 auto?
3. Можно ли generic lambda с захватом присвоить в `void(*)(int)`?
4. Шаблонируется ли захваченный `base` в `[base](auto x)`?
5. Где смотреть ошибку «no matching call» для generic lambda?
6. Нужен ли `-std=c++20` для `requires` в лямбде?
7. Меняется ли sizeof closure от числа инстанциаций `operator()`?

## Обсуждение с репетитором

**Том:** 2 · **Глава:** `ch06` · **Файл:** `ch06-generic-lambdas.md`

**Дальше по курсу:** [Глава 7. Тип замыкания и хранение](./ch07-types-storage.md)
