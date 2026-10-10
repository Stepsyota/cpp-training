# Глава 11. Liskov Substitution

## Зачем эта тема

**Liskov Substitution Principle (LSP):** объекты подтипа должны **подставляться** вместо объектов базового типа **без нарушения** корректности программы. Клиент, написанный для `Base`, не должен **знать**, что под капотом `Derived`, и всё равно работать.

Если это не так — наследование было **ошибкой is-a**. LSP — **тест** на правомерность public наследования. Слабое ООП часто проявляется как «унаследовался, а потом `dynamic_cast` на конкретный derived-тип» — запах нарушенного контракта.

## Контракт метода

У каждого метода негласный контракт:

- **предусловия** (что должно быть true до вызова);
- **постусловия** (что true после);
- **инварианты** класса.

**Derived** может **ослабить** предусловия и **усилить** постусловия (в классической формулировке Барбары Лисков для контрактного программирования). На практике для C++: **не удивляй** клиента.

Пример плохого усиления предусловия:

```cpp
struct Base {
    virtual void set_volume(int v);  // v в [0, 100]
};
struct QuietBase : Base {
    void set_volume(int v) override {
        if (v > 50) throw std::invalid_argument("too loud");
    }
};
```

Клиент Base рассчитывает на 80 — QuietBase **ломает** ожидание.

## Rectangle и Square — классика

```cpp
struct Rectangle {
    virtual void set_width(int w);
    virtual void set_height(int h);
    virtual int area() const;
};
struct Square : Rectangle {
    void set_width(int w) override { side_ = w; }
    void set_height(int h) override { side_ = h; }
};
```

Код, который делает `r.set_width(5); r.set_height(10);` ожидает area 50. Для Square получится side 10 — **не** прямоугольник 5×10. **Square не substitutable для Rectangle** в общем смысле — композиция или отдельные типы.

## Полиморфизм и LSP

```cpp
void double_area(Shape& s) {
    // предполагает, что area() согласован с геометрией
}
```

Любой `Shape` с корректным `area()` — LSP OK. Shape с `area()` return 0 всегда — нарушение смысла, не компилятора.

## Исключения

Если `Base::save()` не бросает, а `Derived::save()` бросает `IOException` — клиент с `noexcept` контекстом может сломаться. Согласованность exception specification важна (в C++17 `noexcept` частично).

## LSP и thread-safety

Если `Base` документирует «все методы thread-safe», а `Derived` требует внешний lock — LSP для многопоточного клиента **нарушен**. Контракт должен включать **sync**.

## Не путать с «любой derived в vector<Base*>»

Хранить `vector<unique_ptr<Shape>>` — OK, если LSP держится. Если один derived — «особенный» — вынеси из иерархии.

## Альтернатива наследованию

Два типа `Rectangle` и `Square` без is-a; общий free function `area(const Rectangle&)` только для Rectangle. Или `std::variant<Rectangle, Square>`.

## Связь с курсом

В multithread редко is-a иерархии. LSP важен, когда **ты** проектируешь `IJobSource`, `IExecutor` для тестов — mock должен **подставляться** без сюрпризов.

## Сценарий: «ReadOnlyFile» наследует «File»

```cpp
struct IFile {
    virtual void write(span<const char> data) = 0;
    virtual std::vector<char> read(size_t n) = 0;
    virtual ~IFile() = default;
};
struct ReadOnlyFile : IFile {
    void write(span<const char>) override {
        throw std::runtime_error("read only");
    }
    std::vector<char> read(size_t n) override { /* ... */ }
};
```

Клиент, работающий с `IFile&`, вызывает `write` — для `ReadOnlyFile` **исключение**. Формально подтип есть, **контракт** «можно записать» нарушен. LSP ломается. Лучше: отдельный `IReadable` без `write`; или `optional`-результат вместо throw в hot path.

## Инварианты и постусловия (практика C++)

Для `Stack` базовый класс обещает: после `push`, `size()` вырос на 1. Derived `BoundedStack` с лимитом **может** бросать при переполнении — это **усиление** предусловия на caller («не push сверх cap») или явный `try_push`. Документируй, что подтип **сужает** допустимые последовательности вызовов — клиент Base может не знать.

## LSP и noexcept

Если базовый `swap(Other&)` documented as `noexcept`, derived не должен бросать из `swap` — иначе generic код с `std::sort` или контейнеры могут **нарушить** свои гарантии. LSP на уровне **exception safety**, не только логики.

## Антипаттерн: «проверяю тип и кастую»

```cpp
void process(Shape& s) {
    if (auto* sq = dynamic_cast<Square*>(&s)) {
        // особая логика — Square не substitutable
    }
    s.area();
}
```

Частый признак: иерархия **для галочки**. Альтернативы: `visit` на `variant`, отдельные overloads, Strategy без общего базового «сломанного» контракта.

## Mock и LSP (тесты)

`FakeThreadPool` для unit-теста smithy должен:

- принимать `submit` как настоящий пул (или документированно синхронно выполнять job);
- не «терять» задачи молча;
- `wait_idle` отражать реальное простаивание.

Если mock **проще** реальности настолько, что тест зелёный, а прод падает — LSP между mock и реализацией **формально** OK, **семантически** нет. Договаривай контракт интерфейса явно.

## LSP vs композиция (решение для Square)

Вместо `Square : Rectangle`:

```cpp
struct Square { int side; };
struct Rectangle { int w, h; };
int area(const Square& s) { return s.side * s.side; }
int area(const Rectangle& r) { return r.w * r.h; }
```

Нет подстановки — нет LSP-проблемы. Общий `draw(const Square&)` / `draw(const Rectangle&)` — overload, не иерархия.

## Связь с virtual destructor

Подтип, уничтожаемый через `Base*`, **обязан** корректно уничтожаться — это часть **контракта lifetime**. Отсутствие virtual ~Base — нарушение LSP в смысле «подставил Derived, программа UB».

## История Rectangle/Square на собеседовании

Интерviewer часто ждёт не «Square bad», а **формулировку контракта**: «Rectangle позволяет независимо менять width и height». Square **нарушает** неизменяемость этой семантики. Альтернатива — **не общий базовый тип** для независимых сторон. Свяжи с главой 6: композиция или отдельные типы.

## Precondition strengthening — пример с pop

Если `BaseQueue::pop` **никогда** не бросает, а `BlockingQueue::pop` ждёт бесконечно — клиент Base с timeout логикой **сломается**. Документируй blocking vs non-blocking **в имени типа** (`try_pop` vs `pop`), не только в комментарии.

## Типичные заблуждения

«Наследование = LSP автоматически» — нет.

«dynamic_cast — нормальный дизайн» — часто признак нарушения LSP.

«LSP только про virtual» — про любую подстановку подтипа.

## К собесу

Объясни LSP. Rectangle/Square. Что нельзя делать в override с предусловиями? Thread-safety в контракте?

## Практика: «контракт на бумаге»

Для воображаемого `IStorage` выпиши три постусловия: `save` сохраняет байты; `load` возвращает то же; `remove` идемпотентен. Придумай `CachingStorage`, который **не** пишет на диск сразу — какие постусловия **ломаются** для клиента, ожидающего durability? Это упражнение на LSP без кода — только слова.

## LSP и ISP

Fat `IStorage` с `save/load/remove/defragment/backup` — mock в тесте **не** может нарушить только backup, если клиент вызывает всё. Узкий `IReadable` + `IWritable` — mock проще, LSP проверять легче. Принципы **усиливают** друг друга.

## Краткое резюме

Подтип не ломает ожидания клиента базы. Плохой is-a — Square/Rectangle. Контракт = пред/пост + инвариант + exceptions + sync. Дальше — ISP.

## Вопросы для самопроверки

1. LSP своими словами?
2. Почему Square спорен для Rectangle?
3. Пример усиления предусловия?
4. Роль thread-safety в LSP?
5. Когда dynamic_cast — запах?
6. Альтернатива is-a для Square?
7. Mock и LSP?

## Обсуждение с репетитором

**Том:** 8 · **Глава:** `ch11` · **Файл:** `ch11-lsp.md`

**Дальше по курсу:** [Глава 12. Interface Segregation](./ch12-isp.md)
