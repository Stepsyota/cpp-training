# Глава 5. Виртуальный деструктор

## Зачем эта тема

Эта глава короткая по объёму API, но **критичная по последствиям**. Один пропущенный `virtual` перед `~Base()` превращает `delete base_ptr` в **неопределённое поведение**, если реально указыватель смотрит на `Derived`. На собеседованиях спрашивают часто; в коде ловят через ASan/valgrind как «утечка» или краш.

Если ты слаб в ООП — выучи правило как таблицу умножения: **полиморфное удаление → virtual destructor в базе**.

## Правило

```cpp
struct Base {
    virtual ~Base() = default;
};

struct Derived : Base {
    std::vector<int> data;
    ~Derived() override = default;
};

Base* p = new Derived{};
delete p;  // OK: сначала ~Derived, потом ~Base
```

Без `virtual ~Base()`:

```cpp
delete p;  // UB: вызван только ~Base, data не уничтожен
```

Компилятор может **не** предупредить. Программа «работает» до смены версии компилятора или размера Derived.

## Когда virtual destructor НЕ нужен

- Класс **не** предназначен для наследования: `final class Widget`.
- Удаление **только** через конкретный тип: `Derived* p = new Derived; delete p;` — без Base*.
- **Value types** без virtual: `Point`, `std::string` — деструктор не virtual (и не должен быть базой для полиморфизма).

Тест: «Будет ли когда-нибудь `delete` через указатель на этот класс, указывающий на derived?» Да → virtual.

## = default и = 0

Деструктор не бывает чистым virtual в смысле `= 0` для обычных классов (абстрактный класс может иметь virtual dtor = 0 в старых стилях; сегодня чаще `virtual ~I() = default` на интерфейсе).

**`= default`** в базе с `virtual` — нормально, если члены RAII.

## unique_ptr и полиморфизм

```cpp
std::unique_ptr<Shape> p = std::make_unique<Circle>(1.0);
```

Деструктор `unique_ptr<Shape>` вызовет `delete` на `Shape*`. Нужен **virtual ~Shape** в момент инстанцирования delete (обычно в заголовке, где виден полный Shape).

**Важно:** если Shape incomplete в точке delete — другая история (pimpl).

## shared_ptr

`shared_ptr<Base>` с `Derived` обычно работает с **правильным deleter** или virtual dtor на Base. Не полагайся на «shared_ptr всё починит» без virtual — deleter по умолчанию — `delete` через Base*.

## Наследование от std:: классов

Не наследуй public от `vector`, `thread`, etc. Если наследуешь от своего Base — dtor rule.

## ThreadPool и virtual

Пул в курсе **не** обязан быть полиморфной иерархией. Деструктор пула **join** воркеров — обычный non-virtual `~ThreadPool()`, если никто не делает `delete IExecutor* p = new ThreadPool`. Если бы был интерфейс `IExecutor` — virtual ~IExecutor.

## Сценарий отладки

Симптом: рост памяти, «утечка» при цикле create/destroy полиморфных объектов. Проверь: есть ли virtual ~Base. ASan leak report может указать на блоки, не освобождённые из Derived members.

## Сценарий: фабрика возвращает unique_ptr<Base>

```cpp
std::unique_ptr<Widget> make_widget(Kind k) {
    if (k == Kind::A) return std::make_unique<WidgetA>();
    return std::make_unique<WidgetB>();
}
```

Деструктор `unique_ptr<Widget>` **вызовет** `delete` через `Widget*`. Без `virtual ~Widget()` — UB и leak derived-части. Правило: **любой** тип, фигурирующий как `unique_ptr<Base>`, **обязан** иметь virtual destructor в Base (или custom deleter, знающий concrete type — редко).

## final и virtual dtor

`struct Base final` — наследовать нельзя; virtual dtor **избыточен** для полиморфного delete через Base* (таких указателей быть не должно). `final` на классе — сигнал «не база для иерархии».

## Виртуальный dtor и = default

```cpp
struct Interface {
    virtual ~Interface() = default;
    virtual void work() = 0;
};
```

`= default` **virtual** dtor в базе — норма для abstract interface. Derived могут `~Derived() override = default;` если нет особой логики.

## Связь с Rule of Five

Полиморфный базовый класс часто **delete** copy/move (или protected), чтобы не slice. Interface с virtual dtor + deleted copy — идиома. Производные хранятся как `unique_ptr<Base>`.

## Чеклист code review (5 секунд)

Есть `class Derived : Base` и `delete`/`unique_ptr` через `Base*`? → `virtual ~Base()`. Нет полиморфного delete? → **не** делай base virtual «на всякий случай».

## Типичные заблуждения

«Компилятор предупредит» — не всегда.

«Достаточно virtual методов, dtor не важен» — неверно.

«Все деструкторы virtual» — раздувает vtable value types.

## Сценарий: shared_ptr и enable_shared_from_this

Полиморфные объекты в `shared_ptr<Base>` с `make_shared<Derived>` обычно безопасны **с** virtual dtor. `enable_shared_from_this` в derived требует, чтобы первый `shared_ptr` создавался на том же типе, что и объект — отдельная ловушка lifetime, не замена virtual dtor.

## Stack allocation и polymorphism

```cpp
Circle c{1.0};
Base& ref = c;
// ref уничтожится как Circle при выходе из scope — OK, no delete
```

Virtual dtor критичен для **delete через pointer to base**. Stack/scope — другой механизм, dtor derived всё равно вызовется при правильном static type объекта.

## Краткое резюме

delete через Base* → virtual ~Base. Value types и final классы — без virtual. `unique_ptr` на полиморфный базовый тип требует virtual dtor в базе. Пул — полиморфия опциональна. Дальше — композиция.

## Вопросы для самопроверки

1. Сформулируй правило virtual destructor.
2. Почему UB без virtual?
3. Когда деструктор намеренно не virtual?
4. Что нужно для `unique_ptr<Iface>`?
5. Связь с slicing?
6. Нужен ли virtual dtor у ThreadPool в типичном дизайне?

## Обсуждение с репетитором

**Том:** 8 · **Глава:** `ch05` · **Файл:** `ch05-virtual-dtor.md`

**Дальше по курсу:** [Глава 6. Композиция и наследование](./ch06-composition-vs-inheritance.md)
