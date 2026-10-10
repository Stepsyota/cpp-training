# Глава 6. inline, virtual, noexcept…

## Зачем эта тема

Объявление функции в C++ — не только имя и тип параметров. Keywords вокруг тела задают **контракт для компилятора, линковщика и читателя**: можно ли держать определение в заголовке, будет ли вызов идти в производный класс, бросит ли функция исключение, допустимо ли неявное преобразование из `int` в твой handle-type.

**`inline`** исторически ассоциировали с «встроить в место вызова», но смысл keyword сегодня — **ODR** (One Definition Rule — одно определение на программу) и заголовки. **`virtual`**, **`override`**, **`final`** **(C++11)** управляют иерархиями. **`explicit`** режет неявные конверсии. **`noexcept`** **(C++11)** — про исключения и move в STL. **`friend`** — точечный доступ к private. **`constexpr`**, **`consteval`**, **`constinit`** **(C++20)** — когда вычисление и инициализация происходят.

Типичные аварии: полиморфная база без virtual destructor → UB при `delete` через базовый указатель; опечатка в override → тихий новый метод; `vector` копирует элементы, потому что move не `noexcept`. На Fedora `g++ -std=c++20` и `clang++ -std=c++20` покрывают большинство этих слов; `consteval` и расширенный `constexpr` сверяй с `--version`.

## `inline` — linkage, не «ускорение»

Компилятор и так может inline-ить вызовы без keyword. **`inline` на функции** говорит: **несколько translation units могут содержать определение**, линковщик сольёт в одну сущность:

```cpp
// math.hpp
inline int twice(int x) { return 2 * x; }
```

Десять `.cpp` включают заголовок — не duplicate symbol, если функция **`inline`**. Без `inline` определение функции в заголовке — нарушение ODR (глава 1).

Большая функция в `.cpp` с `inline` в forward declaration **не** заставит тело оказаться в каждом TU — keyword на определении в заголовке, не магическая директива оптимизатору.

### **(C++17)** inline variables

Та же идея для переменных:

```cpp
inline constexpr std::size_t max_name = 64;
```

Одно определение в header без `extern` в каждом `.cpp`. Удобно для constants рядом с шаблонами.

### Связь с `static` member (глава 2)

**`inline static`** data member в классе **(C++17)** — определение внутри класса без отдельного `.cpp`. Keyword **`inline`** здесь про ODR, **`static`** — один экземпляр на класс.

## `virtual` — динамический полиморфизм

**`virtual`** на методе означает: вызов через указатель или ссылку на **базовый** тип может разрешиться в **производный** (vtable / vptr — деталь реализации):

```cpp
struct Animal {
    virtual void speak() const;
    virtual ~Animal();
};
```

Без `virtual` выбор функции — **static**, по статическому типу указателя. С `virtual` — dynamic dispatch, если объект реально Derived.

### Pure virtual `= 0`

```cpp
virtual void draw() const = 0;
```

Класс **абстрактный** — нельзя создать `Animal a;` без реализации `draw`. **`= 0`** — не отдельный keyword `abstract`; это синтаксис pure virtual.

### Виртуальный деструктор — обязательная привычка

```cpp
std::unique_ptr<Base> p = std::make_unique<Derived>();
// ~Derived(), затем ~Base(), если ~Base() virtual
```

**`delete base_ptr;`** без **`virtual ~Base()`**, когда `base_ptr` указывает на `Derived`, — **UB** (часто утечка ресурсов Derived). Keyword **`virtual`** на деструкторе — стандартный паттерн для полиморфных баз.

## `override` (C++11)

**`override`** после списка параметров **проверяет**, что метод переопределяет virtual из базы:

```cpp
struct Dog : Animal {
    void speak() const override;
};
```

Опечатка `speakk` или несовпадение `const`/ссылки — **ошибка компиляции**, а не скрытый overload. Писать **`virtual ... override`** избыточно; достаточно **`override`** на конечном переопределении (virtualness наследуется).

## `final` (C++11)

На **классе** — запрет наследования:

```cpp
struct Sealed final : Animal { };
// struct X : Sealed { };  // ошибка
```

На **методе** — запрет дальнейших override:

```cpp
void speak() const final;
```

Компилятор может devirtualize в некоторых случаях. Keyword **`final`** не заменяет **`override`** — разные роли.

## `explicit` — контроль неявных конверсий

Конструктор **`explicit Handle(int fd)`** запрещает **`Handle h = 3;`**, но **`Handle h{3};`** или **`Handle h(3)`** — OK. Защита от «случайно передали int вместо handle».

### **(C++20)** explicit(bool)

Conversion operator может быть **`explicit operator bool() const;`** — не всплывает в `if (obj)` как неявный bool в старых стилях, если помечен explicit. Уточняет API «bool только когда явно просят».

### explicit на copy/move

Редко, но иногда блокируют неожиданные копии из rvalue в legacy API.

## `noexcept` — контракт исключений

**Спецификация** **`void f() noexcept`** обещает, что `f` не выходит через исключение (с оговорками стандарта для `noexcept(false)` и destructors). Если исключение всё же выходит из `noexcept` функции — **`std::terminate`**.

```cpp
void swap(int& a, int& b) noexcept;
void maybe() noexcept(false);
```

### Оператор `noexcept(expr)`

**`noexcept(noexcept(g(x)))`** — условный контракт для шаблонов: move assignment элементов `vector` зависит от `noexcept` move конструктора `T`.

### Связь с `std::vector` и move

При reallocation **`vector<T>`** предпочитает **move**, если move constructor **`noexcept`**. Иначе для strong exception guarantee может **копировать** — surprise performance. На Fedora проверь свой тип: **`T(T&&) noexcept = default;`** когда поля это позволяют.

Старый **`throw()`** — deprecated стиль; современный эквивалент — **`noexcept`**.

## `friend` — доверенный доступ

**`friend`** объявление в классе даёт функции или другому классу доступ к **private/protected** без public getter:

```cpp
class Engine {
    friend class Mechanic;
    friend void tune(Engine&);
    int rpm_;
};
```

**Не** наследование, **не** member. Внешний код по-прежнему не видит `rpm_`. Типично: оператор **`operator<<`**, unit tests, тонкая связка двух классов. Злоупотребление `friend` ломает инкапсуляцию — keyword для узких мест.

## `constexpr` на функциях (C++11+)

**`constexpr int sq(int x) { return x * x; }`** — функция *может* выполняться при компиляции. **(C++14)** несколько statement в теле; **(C++17)** **`if constexpr`** в другом контексте. **(C++20)** больше STL и **`constexpr` virtual** с жёсткими ограничениями — не жди полиморфизм runtime в `constexpr` контексте без чтения standard.

Переменная **`constexpr`** — const, но не каждая **`const`** функция — **`constexpr`**.

## `consteval` (C++20) — immediate function

**`consteval`** — функция **обязана** вычисляться при компиляции; вызов с runtime-аргументами — ошибка:

```cpp
consteval int compile_time_only(int x) { return x * 2; }
```

Жёстче **`constexpr`**, где допустим и runtime. **(C++20)** для библиотек compile-time таблиц; на практике проверяй поддержку в Clang/GCC твоей версии.

## `constinit` (C++20) — static initialization

**`constinit static int cache = expensive();`** гарантирует, что переменная с **static storage duration** инициализируется **статически** (до dynamic init), снижая риск **static initialization order fiasco** для *этой* переменной. **Не** делает объект **`constexpr`** автоматически и **не** делает функцию **`constinit`** — keyword только на объектах.

## `static` member functions (напоминание)

**`static int Id::next();`** — нет **`this`**, вызов **`Id::next()`**. Keyword **`static`** здесь про метод класса, не про local static внутри функции (глава 2).

## Перегрузка и keywords: `const`, `&`, `&&` на методах

Virtual override обязан совпадать по const-ности и типам параметров (ссылки считаются частью сигнатуры). **`void draw() override`** и **`void draw() const override`** — разные методы; **`override`** на несовпадающей базе — ошибка. Rvalue ref overloads редки на методах, но в free functions **`void f(string&&)`** vs **`void f(const string&)`** — классический split sink/view.

**`noexcept`** на move constructor часто пишут рядом с **`= default`**: **`Widget(Widget&&) noexcept = default;`**. Компилятор на Fedora генерирует noexcept, если все members noexcept move — иначе проверь trait **`noexcept`** вручную.

## Сценарий: полиморфный интерфейс

Библиотека экспортирует **`struct Plugin { virtual ~Plugin() = default; virtual void run() = 0; };`**. Плагин в `.so` наследует, фабрика возвращает **`std::unique_ptr<Plugin>`**. Без virtual destructor и без **`override`** на `run` рефакторинг базы ломает плагины тихо. С **`override final`** на leaf-классе — явный контракт.

## Типичные ошибки

Полиморфная база **без virtual destructor**. **`override`** с сигнатурой, отличающейся **`const`** от базы — думали override, получили overload. **`noexcept`** на функции, которая зовёт throwing callback. **`inline`** в `.cpp` как «ускорить» без понимания ODR. **`friend`** вместо нормального public API «навсегда». **`consteval`** в header, который включают в C++17 TU — ошибка стандарта.

## Краткое резюме

**`inline`** — ODR-safe определения в заголовках; **(C++17)** **`inline`** variables. **`virtual`** / **`override`** / **`final`** — иерархии и проверки компилятора; virtual destructor для полиморфизма. **`explicit`** — неявные конверсии; **(C++20)** **`explicit(bool)`**. **`noexcept`** — контракт и move в контейнерах. **`friend`** — узкий доступ. **`constexpr`** / **`consteval`** / **`constinit`** — compile-time и static init **(C++20)** для последних двух уровней строгости.

## Вопросы для самопроверки

1. Зачем **`inline`** на определении функции в `.hpp`, включаемом из многих `.cpp`?
2. Что произойдёт при **`delete base_ptr;`**, если **`~Base()`** не virtual, а объект — **`Derived`**?
3. Как **`override`** ловит опечатку в имени virtual-метода?
4. Почему **`std::vector<MyType>`** при росте может копировать элементы вместо move?
5. Чем **`consteval`** строже **`constexpr`** для вызова с неконstantным аргументом?

## Обсуждение с репетитором

**Том:** 1 · **Глава:** `ch06` · **Файл:** `ch06-function-keywords.md`

**Дальше по курсу:** [Глава 7. =default, =delete, special members](./ch07-special-members.md)
