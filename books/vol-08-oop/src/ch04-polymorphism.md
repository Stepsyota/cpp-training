# Глава 4. Полиморфизм

## Зачем эта тема

**Полиморфизм** — «много форм»: один интерфейс, разное поведение в runtime. В C++17 без reflection и без единого `Object` это почти всегда связка **virtual functions** + вызов через **указатель или ссылку на базовый класс**. Понимание vtable не обязательно для ежедневной работы, но снимает магию: почему `override` важен, почему `final` ускоряет devirtualization, почему шаблон — другой вид полиморфизма.

Для слабого места в ООП полиморфизм — частая ловушка: путают **перегрузку** (compile-time) с **override** (runtime), забывают `virtual` на деструкторе, передают `Derived` по значению в `Base`.

## virtual — динамическое связывание

```cpp
struct Shape {
    virtual double area() const = 0;  // чистый virtual
    virtual ~Shape() = default;
};

struct Circle : Shape {
    double r;
    double area() const override { return 3.141592653589793 * r * r; }
};

void print_area(const Shape& s) {
    std::cout << s.area() << '\n';
}
```

`print_area` не знает, Circle или Rectangle внутри — вызывается **реализация объекта**. Без `virtual` на `area` вызвалась бы `Shape::area` (если бы она существовала).

## Чистый virtual и абстрактный класс

`= 0` делает функцию **чистой virtual**: класс **абстрактный**, нельзя `Shape s;`. Можно `Shape*` / `Shape&`. Это C++-способ сказать «интерфейс с поведением по умолчанию опционально».

Производный **обязан** реализовать все чистые virtual, иначе тоже абстрактный.

## override и final (том 1)

**`override`** — компилятор проверяет, что в базе есть совместимый virtual. Опечатка `void area() override` с другой const-квалификацией — ошибка, а не тихий новый метод.

**`final`** на методе — дальше не override. На классе — запрет наследования. Используй, когда иерархия **закрыта** намеренно.

## Как это работает (vtable, кратко)

Объект полиморфного класса обычно содержит указатель на **таблицу функций** (vtable). Вызов `s.area()` через `Shape&` — indirect call по слоту. Размер объекта + один указатель (на x86_64). **Не** виртуальный метод — вызов может быть inline.

`dynamic_cast` и `typeid` используют runtime type information (RTTI) — можно отключить `-fno-rtti` в embedded; тогда `dynamic_cast` на указателях ограничен.

## Полиморфизм compile-time: шаблоны

```cpp
template<class F>
void apply(const std::vector<int>& v, F f) {
    for (int x : v) f(x);
}
```

`F` может быть лямбда, функция, функтор — **без virtual**. Это **статический** полиморфизм: код для каждого `F` генерируется отдельно. Плюс: нет indirect call. Минус: код bloat, всё в заголовках часто.

`parallel_map` в курсе — шаблон, не иерархия `Mapper`. OCP (глава 10) достигается параметром `F`, не `virtual map`.

## std::function — runtime без иерархии

```cpp
using Handler = std::function<void(Event&)>;
void subscribe(Handler h);
```

Type erasure — другая цена (аллокации). Полиморфизм «по смыслу», не по наследованию.

## Ошибки новичка

1. Забыли `virtual` на методе — «полиморфизм не работает».
2. `void take(Shape s)` — slicing.
3. Нет virtual `~Shape` — leak/UB при `delete Shape* p = new Circle`.
4. Вызов virtual из **конструктора базы** — dynamic type ещё Base (не вызывается Circle::foo).
5. Вызов virtual из **деструктора** — то же.

## Сценарий: добавить новый Shape

С иерархией + `print_area(Shape&)`: добавляешь `class Triangle : Shape` с `area() override` — **не** меняешь `print_area`. Это OCP на уровне иерархии (глава 10).

Со switch:

```cpp
double area(const Shape& s) {
    if (auto* c = dynamic_cast<const Circle*>(&s)) ...
}
```

Каждый новый тип — правка здесь — OCP нарушен.

## Связь с многопоточностью

Виртуальные вызовы **не** синхронизируют. Если `Shape` mutable и два потока вызывают методы на одном объекте без mutex — гонка. Полиморфизм не заменяет том 3.

## Сценарий: «забыли override»

```cpp
struct Base { virtual void run(); };
struct Worker : Base { void run(); };  // без override — hiding если сигнатура иная
```

С `-Wsuggest-override` / `-Werror=suggest-override` (том 5) компилятор помогает. Без `override` опечатка в const-квалификаторе создаёт **новый** метод — «полиморфизм не работает», долгое дебаг-сессия.

## NVI (Non-Virtual Interface) — идиома

```cpp
struct Parser {
    void parse(string_view s) {  // public non-virtual
        auto tokens = tokenize(s);
        on_tokens(tokens);       // virtual hook
    }
protected:
    virtual void on_tokens(span<Token>) = 0;
};
```

Клиент вызывает **стабильный** `parse`; derived переопределяет hook. Контроль инварианта в базе — связь с LSP (глава 11).

## Сравнение: virtual vs template vs variant

| Подход | Расширение | Стоимость | Когда |
|--------|------------|-----------|--------|
| virtual | новый derived | indirect call, vtable | открытый набор типов runtime |
| template | новый `F` | код bloat | hot path, compile-time |
| variant | новый member | visit switch | закрытый набор типов |

`parallel_map` — template; plugin host — virtual; AST узлы — часто variant.

## Отладка «virtual не вызывается»

Проверь: (1) `virtual` на базе; (2) параметр `Base&`/`Base*`, не value; (3) `override` сигнатура; (4) не вызов из ctor/dtor базы; (5) объект не slice'нут. В GDB `print typeid(*ptr).name()` (если RTTI включён) — том 6.

## Типичные заблуждения

«virtual на всём» — overhead и запрет constexpr в том же методе.

«dynamic_cast везде» — медленно и хрупко; предпочитай virtual interface.

«Шаблоны заменяют ООП полностью» — разные trade-offs.

## Сценарий: plugin DLL (идея)

Host держит `vector<unique_ptr<Plugin>>` с `virtual void run() = 0`. Новый `.so` — новый derived, **без** пересборки host (на практике ABI C++ хрупок — нужен стабильный C API или same compiler; для учебника — идея OCP+polymorphism). Virtual dtor на `Plugin` обязателен.

## override и const — ловушка

```cpp
struct B { virtual int f() const; };
struct D : B { int f() override; };  // OK — const match
// int f() const override vs int f() override — внимание!
```

Несовпадение const — **другой** метод, не override. Компилятор с `override` спасёт.

## Краткое резюме

Runtime полиморфизм: virtual + override + вызов по Base&/Base*. Абстрактный класс = чистые virtual. Статический: шаблоны, function. Не slice, virtual dtor. Дальше — деструктор подробно.

## Вопросы для самопроверки

1. Зачем `override`?
2. Чем `Shape&` отличается от `Shape` по значению?
3. Что такое чистый virtual?
4. Назови два вида полиморфизма в C++ без Java.
5. Почему virtual в конструкторе базы — подвох?
6. Как parallel_map полиморфен без virtual?
7. Нужен ли mutex «потому что virtual»?

## Обсуждение с репетитором

**Том:** 8 · **Глава:** `ch04` · **Файл:** `ch04-polymorphism.md`

**Дальше по курсу:** [Глава 5. Виртуальный деструктор](./ch05-virtual-dtor.md)
