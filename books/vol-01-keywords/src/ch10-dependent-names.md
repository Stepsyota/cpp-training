# Глава 10. Зависимые имена

## Зачем эта тема

Открыл шаблон коллеги, вставил свой тип — и компилятор выдал «expected `;` before '<' token» или «'iterator' does not name a type». Исходник «для `vector` работал», а для твоей обёртки — нет. Чаще всего виноваты **dependent names (зависимые имена)**: идентификаторы, смысл которых **зависит от template parameter**, и компилятор не может окончательно решить «это тип или значение» до **инстанцирования**.

Keywords **`typename`** и **`template`** (как disambiguator, не как начало шаблона) снимают эту неоднозначность. Без них двухфазный lookup шаблонов кажется магией; с ними — предсказуемым правилом, которое один раз больно, потом пишешь автоматически.

Глава опирается на **`template`** и **`typename`** из главы 9. **(C++20)** **`concept`** сужает множество `T`, но **не отменяет** `typename`/`template` там, где имя всё ещё syntactically dependent.

## Два класса имён внутри шаблона

Раздели всё в теле шаблона на две кучки.

**Недостоверно зависимые (non-dependent)** — не содержат параметр шаблона в пути разрешения. Компилер проверяет их **в момент определения** шаблона (фаза 1), как в обычной функции.

**Зависимые (dependent)** — содержат `T`, `T::something`, `sizeof(T)` и т.п. Их окончательная семантика — на **инстанцировании** (фаза 2).

```cpp
template<typename T>
void foo(T obj) {
    int n = 5;              // non-dependent: int всегда тип
    n = obj;                // может быть ошибка на фазе 2, если T не int
    typename T::value_type v{};  // dependent type — нужен typename
}
```

Ошибка «только при `T = MyStruct`» — типичный след фазы 2: для одного `T` nested name есть, для другого — нет или это не тип.

## Двухфазный lookup — что проверяется когда

**Фаза 1 (definition time):** синтаксис и имена, которые можно разрешить без знания `T`. Если ты написал `T::iterator it` без `typename`, парсер может решить, что `iterator` — **статический член-объект**, и тогда следующий `<` в `T::iterator<int>` воспринимается как **оператор сравнения** — отсюда загадочные сообщения.

**Фаза 2 (instantiation time):** подставили `T = std::vector<int>`, разрешили `T::value_type` как `int`, проверили тело.

Шаблон может быть **синтаксически** валидным и **семантически** невалидным для конкретного `T` — это не баг компилятора, а контракт: «я требую у `T` nested `iterator`».

## `typename` — «это имя типа»

Keyword **`typename`** перед **qualified dependent type name** говорит: трактуй следующее как **type**, не как value.

```cpp
template<typename T>
void bar(T& c) {
    typename T::iterator it = c.begin();
    typename T::const_iterator cit = c.cbegin();
}
```

### Где ещё обязателен `typename`

В **return type**, **typedef/using alias**, **base class** (если dependent), **sizeof** не нужен, но для **declarations**:

```cpp
template<typename T>
using iter_t = typename T::iterator;

template<typename T>
typename T::value_type front_copy(T& c) {
    return c.front();
}
```

В **parameter declarations** dependent type тоже с `typename`:

```cpp
template<typename T>
void take(typename T::value_type v) { /* ... */ }
```

Не путай: в **`template<typename T>`** слово **`typename`** — часть grammar параметра, не disambiguator.

## `template` — «это шаблонный член»

Если слева от `<` стоит **dependent name**, компилятор не знает, что справа — **template arguments**, а не «меньше чем».

Вызов member function template:

```cpp
template<typename T>
void baz(T& obj) {
    obj.template method<42>();
}
```

Для указателя:

```cpp
template<typename T>
void qux(T* p) {
    p->template method<42>();
}
```

В **type** с nested template:

```cpp
template<typename T>
void nest() {
    typename T::template nested<int> x{};
}
```

Здесь два keyword подряд по ролям: **`typename`** (это тип) и **`template`** (внутри — шаблон).

Пропуск `.template` / `::template` — частая причина «`<` is not template» в STL-подобном коде.

## Паттерны из контейнеров и generic-кода

Обобщённая функция «очистить» часто не требует `typename`, если не **упоминает** nested types:

```cpp
template<typename C>
void clear_all(C& c) {
    c.clear();
}
```

`c.clear()` — dependent **вызов**, но не dependent **type name**; disambiguator не нужен.

Как только появляется **`T::something` как тип** — добавляй `typename`:

```cpp
template<typename T>
void append_default(std::vector<typename T::value_type>& out, T& src) {
    if (!src.empty())
        out.push_back(src.front());
}
```

**ADL (argument-dependent lookup)** подмешивает функции из namespace аргументов (`swap`, `begin`). В шаблоне ADL срабатывает на фазе 2; если в namespace типа нет нужной функции до instantiation — ошибка отложена. Это не keyword, но объясняет «почему `find` вдруг нашёлся / не нашёлся».

## Вложенные шаблоны и `template` в объявлении класса

Внутри class template определение **вложенного** class template снова начинается с **`template`**:

```cpp
template<typename T>
struct Outer {
    template<typename U>
    struct Inner {
        U x;
    };
};
```

Ссылаться снаружи:

```cpp
typename Outer<int>::template Inner<double> pair;
```

Без **`template`** после `::` парсер может споткнуться о `<`.

## **(C++20)** concepts не выключают disambiguation

Ограничение `template<typename T> requires Container<T>` может требовать наличие `typename T::value_type` в concept, но **в теле** при записи `T::iterator` компилятор всё равно может потребовать **`typename`**, потому что правило disambiguation — syntactic. Concepts улучшают **диагностику** («нет member iterator»), не заменяют grammar.

```cpp
#include <concepts>

template<typename T>
concept HasIter = requires { typename T::iterator; };

template<HasIter T>
void walk(T& c) {
    typename T::iterator it = c.begin();
}
```

## Связь с `using`, typedef и alias templates

**`using`** alias внутри шаблона для dependent type:

```cpp
template<typename T>
using value_t = typename T::value_type;
```

После alias в других местах файла имя **`value_t<T>`** может быть non-dependent в новом шаблоне — но при определении alias **`typename`** всё равно нужен. Надежда «раз я сделал alias, typename больше нигде не нужен» часто ложна.

## Когда ошибка «только в одном TU»

Шаблон в заголовке инстанцируется во многих `.cpp`. Один TU никогда не инстанцирует `foo<MyWeirdType>`, другой — да, и только там падает сборка. Решение: явно инстанцировать в тесте проблемный тип или добавить concept/static_assert с понятным сообщением.

## Типичные ошибки

Забытый **`typename`** перед `T::type` в typedef или return type. Вызов **`p->foo<0>()`** без **`p->template foo<0>()`**. Думать, что **`using` alias** всегда убирает need for `typename` в downstream коде. Тестировать только `T = int` или только `std::vector` — фаза 2 для custom wrapper не покрыта. Путать **non-dependent** `::iterator` в `std::vector<int>::iterator` (вне шаблона или при фиксированном типе) с **dependent** `T::iterator`. Писать **`template`** в списке параметров там, где нужен disambiguator в **выражении** — разные роли одного keyword.

## Краткое резюме

Dependent name зависит от template parameter; non-dependent проверяется при определении шаблона. **`typename`** перед dependent **type** name; **`template`** перед dependent **template** name в `::` / `.` / `->` цепочках. Двухфазный lookup объясняет отложенные и «странные» синтаксические ошибки. STL-подобные обёртки без `typename`/`template` ломаются на кастомных типах. **(C++20)** concepts уточняют требования, но disambiguators остаются.

## Вопросы для самопроверки

1. Когда перед `T::x` нужен `typename`?
2. Зачем писать `obj.template foo<0>()` вместо `obj.foo<0>()`?
3. Что проверяется на фазе 1 определения шаблона?
4. Почему ошибка может появиться только при `T = Specific`?
5. Чем dependent name отличается от локального `int n` внутри того же шаблона?

## Обсуждение с репетитором

**Том:** 1 · **Глава:** `ch10` · **Файл:** `ch10-dependent-names.md`

**Дальше по курсу:** [Глава 11. Управление потоком и coroutines](./ch11-control.md)
