# Глава 9. template, concept, requires

## Зачем эта тема

Один и тот же алгоритм часто должен работать и с `int`, и с `double`, и с твоим `Order` из кузницы multithread. Дублировать функции под каждый тип — шум; C++ решает это **шаблонами (templates)**: один исходный текст, а компилятор при вызове **инстанцирует (instantiation)** конкретную версию — `min<int>`, `Box<std::string>` и т.д.

Keywords **`template`**, **`typename`** и **`class`** (в списке параметров) задают эту подграмматику. С **(C++20)** к ним добавились **`concept`** и **`requires`** — способ сказать «параметр `T` должен уметь X» **до** тела функции, с сообщениями об ошибках понятнее, чем у старого SFINAE.

Без этой главы не читаются `std::vector<T>`, `std::invoke_result_t` в `parallel_map`, ограничения в STL **(C++20)** и половина заголовков современного C++. Зависимые имена внутри шаблона — отдельно в главе 10; здесь — **введение** шаблона и **ограничения** на параметры.

## `template` — точка входа в параметрический код

Keyword **`template`** всегда стоит **перед** объявлением, которое параметризовано. Дальше в угловых скобках — **template-parameter-list**: типы, константы времени компиляции, иногда шаблоны.

### Шаблон функции

```cpp
template<typename T>
const T& min(const T& a, const T& b) {
    return (b < a) ? b : a;
}
```

Вызов `min(1, 2)` не пишет `<int>` явно: компилятор **выводит** аргументы (`template argument deduction`). Вызов `min<double>(1, 2.0)` фиксирует `T` явно. Для каждой новой комбинации типов в программе может появиться отдельная машинная функция — это цена гибкости.

### Шаблон класса

```cpp
template<typename T>
struct Box {
    T value{};
    explicit Box(T v) : value(std::move(v)) {}
};
```

Здесь `T` — **member type**: поле `value` имеет тип параметра. `Box<int> b{3};` инстанцирует весь класс под `int`.

### Нетиповые параметры (NTTP)

```cpp
template<int N>
struct Buf {
    char data[N]{};
    static constexpr int size = N;
};
```

`N` — значение времени компиляции, не тип. **(C++17)** можно `template<auto V>` — параметр выводится как тип значения. **(C++20)** NTTP расширены (floating point, struct с operator<=> в некоторых случаях) — сверяй стандарт и версию GCC.

### Параметр-шаблон (редко)

```cpp
template<typename T, template<typename> class Container>
struct Adapter {
    Container<T> storage;
};
```

Keyword `template` повторяется внутри списка параметров. Встречается в policy-based design; в учебном коде — редко, но без этого keyword список не прочитать.

## `typename` и `class` в параметрах — одно и то же

```cpp
template<class T>
void f(T);

template<typename U>
void g(U);
```

Для **type parameter** слова **`class`** и **`typename`** **эквивалентны**. Исторически писали `class`; стиль часто выбирают `typename`, чтобы не путать с «`T` обязан class-type» — это не так: `T` может быть и `int`, если так выведено.

Внутри **тела** шаблона `typename` имеет **второй** смысл — disambiguator для зависимых имён (глава 10). В списке параметров это просто часть grammar.

## Специализация: полная и частичная

Иногда общий шаблон не подходит для конкретного `T`. Компилятор выбирает **наиболее специализированную** версию.

Полная специализация — для **конкретного** набора аргументов:

```cpp
template<>
struct Box<bool> {
    bool value;
    // компактное хранение, другая логика
};
```

Перед `<>` снова keyword **`template`**. Частичная специализация возможна **только для class templates**, не для function templates:

```cpp
template<typename T>
struct Box<T*> {
    T* value{};
};
```

Путаница: **перегрузка** функции `void f(int)` / `void f(double)` — не specialization; у функций — только полная специализация или отдельные overloads / constraints.

## `typename` в теле — коротко, подробно в ch10

```cpp
template<typename T>
void use_iter(T& obj) {
    typename T::iterator it = obj.begin();
}
```

Без `typename` компилятор на фазе 1 может принять `iterator` за статический член-объект. Не пропускай главу 10, если пишешь свои контейнерные шаблоны.

## **(C++20)** `concept` — именованное ограничение

Keyword **`concept`** вводит **имя** для набора требований к типу. Определение — булево выражение времени компиляции, часто через **`requires`**.

```cpp
#include <concepts>

template<typename T>
concept Integral = std::is_integral_v<T>;
```

Можно комбинировать concepts:

```cpp
template<typename T>
concept SignedIntegral = Integral<T> && std::signed_integral<T>;
```

### Requires-expression внутри concept

Keyword **`requires`** задаёт **requires-expression**: «для гипотетических значений этих типов выражение должно компилироваться».

```cpp
template<typename T>
concept Addable = requires(T a, T b) {
    { a + b } -> std::convertible_to<T>;
};
```

Стрелка `-> std::convertible_to<T>` — **requirement on expression** **(C++20)**. Ошибка при нарушении указывает на concept, а не на `enable_if` в глубине instantiation.

## **(C++20)** `requires` clause — где писать ограничение

Сокращённая форма параметра:

```cpp
template<Integral T>
T double_it(T x) { return x + x; }
```

Эквивалент `template<typename T> requires Integral<T>`. **Trailing requires** после параметров функции:

```cpp
template<typename T>
T clone(const T& x) requires std::copy_constructible<T> {
    return x;
}
```

**Constrained placeholder**:

```cpp
Integral auto n = 42;
```

Нужны `-std=c++20` и `#include <concepts>` (или свои concepts). На C++17 проекта `training` такие строки не соберутся — это нормально; книга помечает версию.

## SFINAE до concepts — зачем знать легаси

До **(C++20)** отсекали «плохие» `T` через **SFINAE** (Substitution Failure Is Not An Error): невалидная подстановка в overload удаляет кандидата, а не роняет весь translation unit.

```cpp
template<typename T,
         typename = std::enable_if_t<std::is_integral_v<T>>>
void old_way(T x) { /* ... */ }
```

В новом коде читаемее `requires Integral<T>`. SFINAE остаётся в старых библиотеках и в trick с `std::void_t` для detection idiom — tom 2 углубит метапрограммирование.

## Где живут шаблоны: заголовки, ODR, `extern template`

Определение шаблона должно быть **видно** в каждом TU, который его инстанцирует — поэтому функции и member functions шаблонов почти всегда в `.hpp`. Если определение спрятать только в `.cpp` без **explicit instantiation**, получишь **link error** «undefined reference to …».

**One Definition Rule (ODR)** для шаблонов: одинаковые определения во всех TU; linker схлопывает идентичные инстанции (COMDAT). Keyword **`extern template`** **(C++11)** — «инстанцируй **здесь** один раз, в остальных TU только объявление» — экономия времени компиляции в больших проектах.

Старый **`export template`** из C++03 **удалён**. Современный keyword **`export`** **(C++20)** — про **модули** (глава 12), не про экспорт инстанций шаблона из `.cpp`.

## Инстанцирование и ошибки «стены текста»

Ошибка в **внутренности** шаблона часто печатается только при **конкретном** `T` — «instantiation of … required from here». **(C++20)** concepts укорачивают верх стека: «`T` does not satisfy `Addable`». Без concepts ищи **первое** сообщение с твоим типом внизу лога.

`static_assert` внутри шаблона — не keyword из этой главы, но дополняет concepts: «если дошли сюда с `T=void`, сообщи понятным текстом».

## Связь с multithread и томом 2

`parallel_map` использует **`std::invoke_result_t<F, const T&>`** — alias на результат `F`; за ним шаблон `std::invoke_result`. Пул и очередь могут быть шаблонами по типу задачи. Concepts **(C++20)** в STL (`ranges`, `sortable`) — тот же язык keywords, что ты читаешь здесь.

## Типичные ошибки

Забыть **`template`** перед вложенным dependent template (`p->nested<int>()` → нужно `p->template nested<int>()` — глава 10). Класть **единственное** определение function template в `.cpp` без explicit instantiation. Писать **`concept`** без стандарта C++20 или без заголовков. Путать **partial specialization** с overload function template. Ожидать, что **`export template`** из статьи 2000-х переносится в модули один в один. Два разных определения одного и того же шаблона в разных заголовках — ODR violation, иногда тихая порча.

## Краткое резюме

Keyword **`template`** открывает параметрические функции, классы и метаданные (NTTP). **`typename`** / **`class`** в списке параметров равны для типов; в теле `typename` ещё помечает dependent type. Специализации настраивают class templates; function templates — через overload или полную specialization. **(C++20)** **`concept`** и **`requires`** задают ограничения и улучшают диагностику. Шаблоны живут в заголовках; помни ODR и **`extern template`**. Старый `export template` мёртв; новый **`export`** — модули.

## Вопросы для самопроверки

1. Чем полная специализация class template отличается от частичной?
2. Зачем `concept` вместо одного только `static_assert` внутри функции?
3. Почему определения шаблонов функций часто лежат в `.hpp`?
4. Эквивалентны ли `template<class T>` и `template<typename T>` в параметрах?
5. Что означает SFINAE одним предложением и зачем это знать после C++20?

## Обсуждение с репетитором

**Том:** 1 · **Глава:** `ch09` · **Файл:** `ch09-template-keywords.md`

**Дальше по курсу:** [Глава 10. Зависимые имена](./ch10-dependent-names.md)
