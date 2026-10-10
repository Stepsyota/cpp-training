# Глава 7. =default, =delete, special members

## Зачем эта тема

Класс в C++ — не только поля и методы. Компилятор **неявно объявляет** набор **special member functions** (особые функции-члены): конструктор по умолчанию, копирование, move **(C++11)**, присваивания, деструктор. Keywords **`=default`** и **`=delete`** **(C++11)** позволяют сказать явно: «сгенерируй как обычно» или «этот overload вызывать нельзя».

От этого зависят RAII, move-only типы вроде **`std::thread`**, попадание в **`vector<T>`** без лишних копий и безопасность полиморфных иерархий. Половинчатое «написал только деструктор» для класса с **`new`** — классика утечек; **`=delete`** на copy — яснее, чем private без определения. На Fedora GCC и Clang следуют одним правилам implicit generation; при сомнении смотри `-Wdeprecated-copy` и сообщения про implicitly deleted.

## Шесть special members (rule of five)

Для class type **`T`** компилятор **может** объявить:

1. Default constructor **`T()`**
2. Copy constructor **`T(const T&)`**
3. Copy assignment **`T& operator=(const T&)`**
4. Move constructor **`T(T&&)`** **(C++11)**
5. Move assignment **`T& operator=(T&&)`** **(C++11)**
6. Destructor **`~T()`**

«Rule of five» — если управляешь ресурсом вручную, определи или **`=delete`**, или корректно реализуй **все пять** (плюс деструктор — шесть сущностей, но «five» считают без default, в зависимости от учебника; здесь важен принцип: **ресурс — полный набор**).

Объявление **любого** из них влияет на **implicit generation** остальных: например, user-declared destructor может подавить неявный move в старых правилах; в **C++11+** move не генерируется, если copy или destructor объявлены пользователем — читай таблицу в standard / cppreference для своей версии.

Практическая шпаргалка для C++11 и новее: объявил **copy constructor** или **copy assignment** — компилятор **не** генерирует move constructor и move assignment. Объявил **move** или **destructor** — неявный copy может быть deprecated или suppressed в зависимости от версии; в новом коде явно **`= default`** или **`= delete`** copy/move, чтобы не гадать. **`= default`** destructor на polymorphic base — почти всегда **`virtual`**.

Default constructor не генерируется, если есть user-defined constructor другого вида — тогда нужен **`MyType() = default;`** явно, если default init всё ещё нужен.

## `=default` — явная генерация

**`=default`** означает: «хочу эту special function, пусть компилятор сгенерирует trivial / memberwise версию»:

```cpp
struct Packet {
    Packet() = default;
    Packet(const Packet&) = default;
    Packet& operator=(const Packet&) = default;
    ~Packet() = default;
};
```

Для типа только из **`std::string`** и **`vector<int>`** это эквивалентно неписанию — но явность документирует намерение.

### В теле класса vs в `.cpp`

**`= default`** внутри класса часто даёт **trivial** / **constexpr**-friendly поведение там, где out-of-class **`Widget::Widget() = default;`** в `.cpp` может отличаться по triviality и instantiations. Для move в header-only библиотеках **`= default`** move в классе обычно предпочтительнее, чем define в `.cpp` — иначе клиенты получают лишние копии.

## `=delete` — запрет вызова

**`= delete`** делает функцию **ill-formed** при использовании — **ошибка компиляции**, не link error из private:

```cpp
struct UniqueFile {
    UniqueFile(const UniqueFile&) = delete;
    UniqueFile& operator=(const UniqueFile&) = delete;
};
```

Сценарий handle-type: дескriptor файла, socket — копировать нельзя, только move (если move не delete).

### Удаление перегрузок

```cpp
void process(int) = delete;
void process(double);
```

**`process(42)`** — ошибка: **`int`** overload удалён. Полезно отсечь «плохие» типы без SFINAE. **`void sink(std::string&&) = delete;`** вместе с **`sink(const std::string&)`** запрещает binding к temporary через overload resolution — тонкий приём API.

## Rule of zero

**Не объявляй** special members, если все поля сами корректны:

```cpp
struct Order {
    std::string name;
    std::vector<int> items;
};
```

Компилятор генерирует copy/move/destructor из memberwise rules. **`std::mutex`** member сделает класс **move-only** или non-copyable автоматически — не пытайся «=default copy» вокруг mutex.

Rule of zero — предпочтительный стиль на Fedora в новом коде: RAII-обёртки **`unique_ptr`**, **`vector`**, не сырой **`new`**.

## Rule of five / rule of three (легаси)

Управляешь **`new[]`**, FILE*, custom buffer — определи destructor + copy + copy assign или перейди на **`unique_ptr<T[]>`** (rule of zero). Move добавь **(C++11)** явно, если класс copyable и heavy:

```cpp
Buffer(Buffer&&) noexcept = default;
Buffer& operator=(Buffer&&) noexcept = default;
```

Половинчатый destructor без copy assign — double free при копировании.

## `explicit` + `=default` (C++11)

**`explicit Token() = default;`** запрещает некоторые forms copy-initialization **`Token t = {};`** в зависимости контекста — оставляет **`Token t{};`**. Keyword **`explicit`** на default constructor — design для strong types.

## Полиморфные классы

```cpp
struct Base {
    virtual ~Base() = default;
    Base(const Base&) = delete;
    Base& operator=(const Base&) = delete;
};
```

**`= default`** virtual destructor — норма. Copy для полиморфной базы часто **`= delete`**: slice при копии **`Derived`** в **`Base`** — логическая ошибка; вместо этого **`clone()`** virtual factory.

## Move-only типы и `std::thread`

**`std::thread`** copy **= delete**, move enabled. Класс с полем **`thread worker_;`** наследует: копирование класса — delete, если не напишешь свою семантику. То же для **`unique_ptr`**, **`atomic`** (copy deleted). Keywords **`= default`** move constructor явно документируют move-only wrapper.

## Тривиальность — не keywords, но след

**Trivially copyable**, **standard layout** — traits для **`std::atomic<T>`** (когда **`T`** подходит), **`memcpy`**-подобных API. Ручные destructor/copy могут снять triviality — важно для embedded и interop, не для каждого приложения.

## **(C++20)** и special members

**`= default`** для **`constexpr`** destructor в trivial types участвует в compile-time containers. **`= delete`** **`constexpr`** function — если когда-либо instantiated, ошибка. Для повседневного прикладного кода C++17 на Fedora достаточно понимать **=default/delete**; C++20 расширяет **`constexpr`** special members — помечай **(C++20)**, если используешь в `constexpr` алгоритмах.

## Сценарий: wrapper вокруг FILE*

```cpp
struct FileHandle {
    FileHandle() = default;
    explicit FileHandle(FILE* f) : f_(f) {}
    ~FileHandle() { if (f_) std::fclose(f_); }
    FileHandle(const FileHandle&) = delete;
    FileHandle& operator=(const FileHandle&) = delete;
    FileHandle(FileHandle&& o) noexcept : f_(std::exchange(o.f_, nullptr)) {}
    FileHandle& operator=(FileHandle&& o) noexcept {
        if (this != &o) {
            if (f_) std::fclose(f_);
            f_ = std::exchange(o.f_, nullptr);
        }
        return *this;
    }
private:
    FILE* f_ = nullptr;
};
```

Альтернатива rule of zero — **`unique_ptr<FILE, Deleter>`** без ручного five.

## Aggregates и special members **(C++20)**

**Aggregate** (агрегат) — struct без user-provided constructor и с особыми правилами инициализации **`{ ... }`**. Добавление **`= default`** user constructor ломает aggregate status — brace-init меняется. **(C++20)** уточнили списки; если твой тип «просто поля», не объявляй лишний constructor без нужды.

**`= delete`** default constructor делает тип non-default-constructible — полезно для static-only utility structs.

## Компиляторная диагностика на Fedora

Сообщение «use of deleted function» при copy — след **`= delete`**. «implicitly deleted» — компилятор сам запретил (например, copy класса с **`atomic`** member). **`= default`** на move в header и предупреждение о deprecated implicitly-declared copy — повод явно **`= delete`** copy в public API. С **`clang++ -stdlib=libstdc++`** и **`g++`** формулировки различаются, смысл один.

## Сценарий: класс в `vector` и exception safety

Если copy assign бросает, **`vector`** при insert может откатиться; move **`noexcept`** (глава 6) + **= default** move — часть контракта. **`= delete`** copy + **= default** move — idiom move-only resource.

## Типичные ошибки

Объявить **destructor** → ожидать неявный **move** там, где compiler не генерирует — surprise copies. **`= default`** move в **`.cpp`** для type в header — клиенты не inline move. Копируемый wrapper с **`mutex`** или **`unique_ptr`**. Public copy для handle вместо **`= delete`**. Забыть **`virtual ~Base() = default`** при **`= delete`** copy в иерархии — destructor всё равно virtual для delete через base.

## Краткое резюме

Special members — default/copy/move/assign/destructor; move с **(C++11)**. **`=default`** — явная генерация компилятором; **`=default`** virtual destructor для полиморфизма. **`=delete`** — вызов ill-formed at compile time; точечное удаление overloads. **Rule of zero** предпочтителен; иначе rule of five или RAII-обёртки. Move-only и **`thread`** / **`unique_ptr`** — следствие deleted copy.

## Вопросы для самопроверки

1. Что сломается при default copy класса с полем **`std::unique_ptr<int>`**?
2. Зачем **`=delete`** copy лучше, чем private copy без definition?
3. Когда user-declared destructor влияет на неявно генерируемый move constructor?
4. Можно ли **`virtual ~Base() = default;`** у полиморфной базы?
5. Сколько функций входят в «rule of five» если считать move отдельно от copy (copy ctor, copy assign, move ctor, move assign, dtor)?

## Обсуждение с репетитором

**Том:** 1 · **Глава:** `ch07` · **Файл:** `ch07-special-members.md`

**Дальше по курсу:** [Глава 8. Приведения типов](./ch08-casts.md)
