# Глава 3. Наследование

## Зачем эта тема

**Наследование** в C++ — это одновременно **отношение подтипа** («is-a») и **механизм компоновки памяти**: объект производного класса содержит подобъект базового. Первая часть нужна для **полиморфизма** (глава 4). Вторая объясняет **slicing**, layout, вызовы конструкторов. Путаница между «наследовать, чтобы переиспользовать код» и «наследовать, потому что подтип» — главный источник плохого ООП и нарушений LSP (глава 11).

Ты слабее в ООП — значит, чаще всего стоит **остановиться перед `class Derived : public Base`** и спросить: **действительно ли клиент должен подставлять Derived везде, где ждёт Base?**

## public, protected, private наследование

```cpp
struct Animal {
    void eat();
};
struct Dog : public Animal {
    void bark();
};
```

**public наследование** — «Dog is-an Animal» для внешнего кода: `Animal& a = dog;` допустимо.

**protected наследование**: снаружи `Dog` **не** substitutable для `Animal` — только для внутренней иерархии (редко).

**private наследование**: `Dog` реализован **с помощью** Animal (implementation inheritance), но не is-a — чаще заменяется **полем** `Animal a_;` (композиция).

В 95% учебного и прикладного кода тебе нужно только **public**. Остальное — advanced (policy-based design).

## Порядок конструирования и уничтожения

При создании `Dog`:

1. Конструктор `Animal` (база).
2. Конструктор `Dog` (члены, тело).

При уничтожении — **наоборот**: сначала `~Dog`, потом `~Animal`. Если база полиморфна — `~Animal` должен быть **virtual** (глава 5).

Если базовый конструктор требует аргументов — derived **обязан** передать их в initializer list:

```cpp
struct Base { explicit Base(int x); };
struct Derived : Base {
    Derived() : Base(42) {}
};
```

Забытый вызов базового конструктора — ошибка компиляции или неверная инициализация.

## Slicing — срезка объекта

```cpp
struct Base { virtual ~Base() = default; int b{1}; };
struct Derived : Base { int d{2}; };

Base slice = derived_object;  // НЕ полиморфно
```

Здесь **копируется только часть Base**. Поле `d` отброшено. Тип переменной `slice` — `Base`. **Полиморфизм работает через ссылку или указатель:**

```cpp
Base& ref = derived_object;
Base* ptr = &derived_object;
```

Любая функция, принимающая `Base` **по значению**, режет объект. API вида `void foo(Base b)` — красный флаг для иерархий.

## Переопределение, скрытие, using

Без `virtual` метод в derived **скрывает** имя базы, но **не переопределяет** полиморфно:

```cpp
struct Base { void f(); };
struct Derived : Base { void f(); };
Base* p = new Derived;
p->f();  // Base::f — статический тип Base*
```

С `virtual` — динамический выбор (глава 4). **`override`** ловит опечатки в сигнатуре.

Если база имеет overloads `void g(int)` и `void g(double)`, а derived объявляет только `void g(int)`, `g(double)` **скрыт** в derived. Лечение:

```cpp
using Base::g;
void g(int);
```

## Множественное наследование

```cpp
struct Worker : Employee, TaxPayer { };
```

Два подобъекта баз в одном `Worker`. Проблема **ромба**: два пути к общему `Person` — нужен **virtual inheritance** (сложно, медленнее). В прикладном коде чаще:

- один базовый интерфейс;
- остальное — **композиция** (роли).

## Наследование ради reuse — запах

«У меня есть `std::vector` элементов `int`, хочу унаследоваться от vector» — публичное наследование от STL контейнера **ломает** инварианты vector (любой код может вызвать неограниченный `push_back`). Лучше **поле** `std::vector<int> data_` внутри класса и узкий API.

«Наследуюсь от `ThreadPool`, чтобы добавить одну функцию» — почти всегда **композиция** или free function.

## Когда наследование уместно

- Полиморфная иерархия: `Shape`, `Parser`, `EventHandler`.
- CRTP и policy классы (шаблоны) — за рамками базового тома, но это не «классическое» runtime ООП.
- Тестовые mock: `MockStorage : IStorage`.

## Связь с курсом

`ThreadPool` **не** обязан наследовать `WorkerThread`. Воркеры — **композиция** `std::thread` + общее состояние. Наследование не появляется «потому что ООП», а когда есть **подтип**.

## Сценарий: «удобно вызвать метод базы»

```cpp
struct LoggedVector : std::vector<int> {
    void push_back(int x) {
        log(x);
        std::vector<int>::push_back(x);
    }
};
```

Любой код может вызвать `vector::insert` / `resize` и **обойти** логирование. Публичное наследование от STL — **утечка** инварианта. Композиция: `vector<int> data_;` + `void append(int)` — единственный путь мутации.

## Object slicing в контейнерах

```cpp
std::vector<Base> v;
v.push_back(Derived{});  // slice каждый раз
```

Полиморфная коллекция — `vector<unique_ptr<Base>>` или `vector<Derived>` без общей базы (variant). Ошибка slicing **тихая** — тип Derived «исчез» в vector.

## protected data — почему хрупко

Derived напрямую трогает `protected int count_` базы — база меняет layout или инвариант count — **все** derived ломаются. Предпочитай `protected` **virtual hook** или private + protected non-virtual interface в базе.

## К собесу

Slicing; порядок ctor/dtor; hiding vs override; почему не inherit vector; когда public inheritance.

## Типичные заблуждения

«Наследование = главный способ reuse» — композиция чаще.

«Slicing OK, если осторожно» — по значению — всегда срезка.

«protected data OK для наследников» — хрупко; prefer protected accessors или private + protected virtual hooks.

«Множественное наследование зло всегда» — зло без нужды; иногда два mixin интерфейса OK.

## Сценарий: «переиспользовали» код базы

```cpp
struct Base {
    void connect() { /* tcp */ }
};
struct SecureClient : Base {
    void connect() { Base::connect(); /* + tls */ }
};
```

Если `SecureClient` **не** substitutable там, где ждут «просто connect без tls» — is-a сомнителен. Композиция: поле `TcpConnection conn_;` + метод `connect()` orchestrates tls — LSP не страдает, потому что нет public Base.

## final class — закрыть иерархию

```cpp
class ThreadPool final { /* ... */ };
```

Никто не наследует — slicing и fragile hierarchy отсечены. Для value/service types часто лучше `final`, чем «может кто-то унаследится».

## Краткое резюме

Public наследование = is-a для клиента. Конструкторы base→derived, деструкторы наоборот. Полиморфизм — ссылка/указатель, не value. Не наследуй STL и не путай reuse с подтипом. Дальше — virtual.

## Вопросы для самопроверки

1. Что такое slicing и как его избежать?
2. Порядок вызова конструкторов при `new Derived`?
3. Чем hiding отличается от override?
4. Зачем `using Base::g`?
5. Почему плохо публично наследовать `std::vector`?
6. Нужно ли наследование для ThreadPool?
7. Когда private наследование вместо поля?

## Обсуждение с репетитором

**Том:** 8 · **Глава:** `ch03` · **Файл:** `ch03-inheritance.md`

**Дальше по курсу:** [Глава 4. Полиморфизм](./ch04-polymorphism.md)
