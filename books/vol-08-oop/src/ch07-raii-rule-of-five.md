# Глава 7. RAII и Rule of Five

## Зачем эта тема

**RAII** (Resource Acquisition Is Initialization) — фундамент C++, без которого ООП превращается в ручной `malloc`/`free` и забытые `unlock`. Ты уже жил RAII в `lock_guard`, `unique_lock`, `thread` (join в dtor или terminate). Эта глава связывает RAII с **Rule of Zero / Five** из тома 1 — чтобы класс **владел** ресурсом осознанно, а не «случайно скопировался» и удвоил mutex.

Слабое ООП часто проявляется как: «написал деструктор с `delete`, забыл copy constructor» — Rule of Five как чеклист.

## RAII одной фразой

Ресурс захватывается в **конструкторе** объекта-владельца и освобождается в **деструкторе**. Scope объекта = lifetime ресурса. Исключение между acquire и release — guard всё равно вызовет dtor при unwind.

```cpp
void work() {
    std::lock_guard<std::mutex> lk(m);
    // ...
}  // unlock здесь автоматически
```

Mutex — не «ООП в Java смысле», но **объект** `lock_guard` владеет «захватом» mutex.

## Rule of Zero

Если все члены класса корректно управляют своими ресурсами (`string`, `vector`, `unique_ptr`, `mutex` — mutex **не** копируется), **не объявляй** свои copy/move/dtor — компилятор сгенерирует memberwise правила.

```cpp
class Order {
    std::string id_;
    std::vector<LineItem> lines_;
public:
    void add(LineItem x) { lines_.push_back(std::move(x)); }
};
```

Это **идеальный** ООП-тип для бизнес-данных без сырого указателя.

## Rule of Five (шесть special members)

Если ты **вручную** управляешь ресурсом (raw pointer, file descriptor, `new[]`), объявление **любого** из:

- destructor
- copy constructor
- copy assignment
- move constructor
- move assignment

требует **осознанного** решения по **всем остальным** (в C++11+ таблица implicit generation сложная — практика: **либо все =default/delete, либо реализуй полный набор**).

Пример delete copy для unique ownership:

```cpp
class Handle {
    int fd_{-1};
public:
    Handle(int fd) : fd_(fd) {}
    ~Handle() { if (fd_ >= 0) ::close(fd_); }
    Handle(const Handle&) = delete;
    Handle& operator=(const Handle&) = delete;
    Handle(Handle&& o) noexcept : fd_(o.fd_) { o.fd_ = -1; }
    Handle& operator=(Handle&& o) noexcept;
};
```

Лучше: `Handle` на `unique_ptr` с custom deleter — Rule of Zero.

## Копирование и mutex

Класс с `std::mutex` member **не** копируется по умолчанию нормально — copy deleted implicitly в многих случаях. **Не** пиши `= default` copy для класса с mutex. Пул **не** копируется — один экземпляр на программу или явный delete copy.

## Move и очередь задач

`std::function<void()>` или packaged_task в `deque` — **move** в `push` избегает копирования тяжёлого closure. Класс Job с `unique_ptr<Impl>` — move-only. ООП здесь — **владение** задачей переходит в очередь, не копируется.

## Полиморфизм и копирование

Полиморфные иерархии часто **запрещают** slice: copy Base от Derived — плохо. Паттерн: `clone()` virtual, или храни `unique_ptr<Base>`.

## Исключения и noexcept move

`vector` reallocate при move — если move constructor бросает, сложные гарантии. Для performance `noexcept` move на своих типах в контейнерах — тема tom 1; для курса — знай, что **move** в очередь пула должен быть **дешёвым и не бросать** (function может бросать при copy внутри — ещё причина move).

## Связь с special members тома 1

Перечитай `ch07-special-members` тома 1 после этой главы — там таблица implicit generation. Том 8 добавляет **дизайн**: «нужен ли мне вообще raw resource в классе?»

## Сценарий: класс Buffer с new[]

Если написал `~Buffer() { delete[] data_; }` — добавь или delete copy, или deep copy, или замени на `vector<char>`. Иначе double-free при копировании.

## Сценарий: ThreadPool и join в деструкторе

Деструктор пула **обязан** остановить воркеров и **join** — иначе `std::terminate` на joinable `std::thread` (том 3, THEORY). RAII: lifetime пула = lifetime worker threads. **Не** вызывай `join` под тем же mutex, который ждёт воркер — deadlock (AGENTS.md). Это ООП + sync **в одном** типе.

## Rule of Zero в bounded queue

Очередь с `mutex`, `condition_variable`, `deque` — **не** объявляй copy; move может быть deleted implicitly. Клиент держит один экземпляр или `unique_ptr`. Попытка copy queue — ошибка дизайна (два mutex на одну логическую очередь).

## unique_ptr как владение полиморфом

```cpp
class Document {
    std::unique_ptr<IBlock> root_;
public:
    // Rule of Five: default move, delete copy или deep clone virtual
};
```

Владение через smart pointer + virtual ~IBlock — связка RAII, Rule of Five, virtual dtor (главы 5 и 7).

## Антипаттерн: «сырой new в конструкторе, delete в dtor, всё остальное default»

Классическая утечка при exception в конструкторе до присвоения pointer в member — используй `unique_ptr` или RAII guard. Rule of Five **начинается** с вопроса «нужен ли raw owning pointer?»

## Связь SOLID и RAII (мост к части 2)

SRP: класс-владелец ресурса **один** отвечает за release. DIP: **не** создавай `SqliteConnection` внутри — получи `IConnection` снаружи, но **закрой** в dtor или `unique_ptr`. LSP: derived с дополнительным ресурсом в dtor **должен** корректно уничтожаться через base pointer — virtual dtor.

## К собесу

Rule of Zero vs Five; почему `mutex` member делает класс non-copyable; RAII пример кроме памяти (`lock_guard`, `fstream`).

## Типичные заблуждения

«= default на всём всегда» — для raw pointer опасно.

«mutex можно копировать через = default» — нет.

«RAII только про память» — файлы, locks, threads.

«ООП без деструкторов» — в C++ dtor всегда есть.

## Сценарий: self-assignment и copy assignment

Если когда-нибудь пишешь ручной `operator=` для ресурса — проверь `if (this == &other) return *this;` и strong exception guarantee (copy-and-swap идиома). В Rule of Zero этим **не** занимаешься — `vector` и `string` уже правильные.

## =delete move — иногда намеренно

```cpp
class NotMovable {
    NotMovable(NotMovable&&) = delete;
};
```

Редко; чаще delete copy. `mutex` — не movable в старых стандартах; в C++17 `mutex` not movable — тип с mutex member наследует ограничение.

## Мост к SOLID (перед главой 8)

Перед обзором SOLID зафиксируй три связки:

1. **Инвариант + RAII** — класс очереди **единственный** владелец mutex и deque; клиент не release ресурс вручную — это **граница** для SRP.
2. **Move-only пул** — copy deleted → один явный владелец пула в программе; injection пула снаружи (DIP) не путается с копией.
3. **Virtual dtor на интерфейсе** — когда появится `IExecutor` для тестов, RAII `unique_ptr<IExecutor>` и LSP на уничтожении идут **вместе**.

Если эти три пункта ясны — SOLID будет не абстракцией из Java-блогов, а продолжением idioms C++, которые ты уже использовал в lock_guard и thread.

## Краткое резюме

RAII — scope = resource. Rule of Zero с STL-членами. Raw resource → Rule of Five или delete copy/move. Пул move-only jobs. mutex — не копировать. Дальше — SOLID обзор.

## Вопросы для самопроверки

1. Пример RAII не про память?
2. Когда Rule of Zero?
3. Что после user-defined dtor с raw pointer?
4. Почему пул не copyable?
5. Зачем move в submit?
6. Связь lock_guard и ООП?
7. Что опасного в `= default` copy для класса с mutex?

## Обсуждение с репетитором

**Том:** 8 · **Глава:** `ch07` · **Файл:** `ch07-raii-rule-of-five.md`

**Дальше по курсу:** [Глава 8. Обзор SOLID](./ch08-solid-overview.md)
