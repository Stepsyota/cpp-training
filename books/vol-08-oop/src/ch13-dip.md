# Глава 13. Dependency Inversion

## Зачем эта тема

**Dependency Inversion Principle (DIP):**  
1) модули верхнего уровня не должны зависеть от модулей нижнего уровня — оба зависят от **абстракций**;  
2) абстракции не должны зависеть от деталей; детали зависят от абстракций.

Проще: **политика** (бизнес-логика) не должна **вшивать** `SqliteDatabase` или `TcpSocket`; она говорит с **`IDatabase`**, **`IConnection`**, а конкретика подставляется снаружи (**dependency injection**).

DIP — частый провал на практике: «написал класс, внутри `new`» — тестировать нельзя, сменить БД — вырезать код.

## Плохо: конкретика внутри

```cpp
class OrderService {
    SqliteDatabase db_{"orders.db"};
public:
    void place_order(Order o) {
        db_.insert(o);
    }
};
```

`OrderService` **знает** SQLite, путь к файлу, SQL. Тест без диска — боль.

## Лучше: инверсия

```cpp
struct IOrderRepository {
    virtual void save(const Order&) = 0;
    virtual ~IOrderRepository() = default;
};

class OrderService {
    std::unique_ptr<IOrderRepository> repo_;
public:
    explicit OrderService(std::unique_ptr<IOrderRepository> repo)
        : repo_(std::move(repo)) {}
    void place_order(Order o) { repo_->save(o); }
};

// main — composition root
int main() {
    auto repo = std::make_unique<SqliteOrderRepository>("orders.db");
    OrderService svc(std::move(repo));
}
```

Высокий уровень зависит от **`IOrderRepository`**. SQLite — деталь в `main`.

## Composition root

Место, где **собирается** граф объектов: обычно `main`, фабрика приложения, DI-контейнер (в C++ реже, чем в Java). **Только здесь** `new Sqlite`, `make_unique<ThreadPool>(4)`.

## DIP и тесты

```cpp
struct FakeRepo : IOrderRepository {
    std::vector<Order> saved;
    void save(const Order& o) override { saved.push_back(o); }
};

TEST(OrderService, places) {
    auto repo = std::make_unique<FakeRepo>();
    OrderService svc(std::move(repo));
    // ...
}
```

Без DIP тест цепляет файловую систему.

## DIP без virtual: шаблон

```cpp
template<class Repo>
class OrderServiceT {
    Repo repo_;
public:
    explicit OrderServiceT(Repo r) : repo_(std::move(r)) {}
    void place_order(Order o) { repo_.save(o); }
};
```

Зависимость от **контракта** `Repo::save` compile-time. DIP на этапе компиляции; mock — struct с тем же методом.

## DIP и multithread

Кузница **зависит** от абстракции «исполнитель задач» (`ThreadPool&` или интерфейс `submit`), а не создаёт `std::thread` на каждый заказ внутри бизнес-логики. Деталь «4 воркера» — в `main` при создании пула.

## Ссылка vs unique_ptr

`OrderService(IOrderRepository& repo)` — lifetime снаружи. `unique_ptr` — владение. Оба — injection; выбор по lifetime (том 2, 4).

## Антипаттерн Service Locator

Глобальный `getDatabase()` — **скрытая** зависимость, тесты всё ещё боль. Явный конструктор лучше.

## DIP vs просто «есть интерфейс»

Интерфейс без injection: `class Service { SqliteDb db_; };` implements `IService` — **не** DIP. DIP — **направление** зависимости: политика → абстракция ← деталь.

## Сценарий: логирование «для продакшена и тестов»

Плохо:

```cpp
class Parser {
    void parse(string_view s) {
        spdlog::info("parse {}", s);  // concrete logger
        // ...
    }
};
```

Тесты либо пишут в консоль, либо мокают глобальный spdlog — боль. Лучше:

```cpp
class Parser {
    ILogger& log_;
public:
    explicit Parser(ILogger& log) : log_(log) {}
    void parse(string_view s);
};
```

`main`: `FileLogger` или `SpdlogAdapter`. Тест: `NullLogger` или `vector<string>` sink. **Направление**: Parser → `ILogger` ← concrete.

## Constructor injection vs setter injection

**Constructor** — зависимости **обязательны** для валидного объекта; объект **immutable** после создания (если ссылки/const). **Setter** — опционально, но объект может быть **half-initialized** — запах. В C++ предпочитай constructor + `explicit` для single dependency.

```cpp
class Service {
    IRepo& repo_;  // non-owning; lifetime у composition root
public:
    explicit Service(IRepo& r) : repo_(r) {}
};
```

`unique_ptr<IRepo>` — когда сервис **владеет** репозиторием или lifetime не совпадает с `main`.

## DIP и фабрики

Иногда `main` не должен знать все derived:

```cpp
std::unique_ptr<IOrderRepository> make_repository(const Config& cfg) {
    if (cfg.use_memory) return std::make_unique<InMemoryRepo>();
    return std::make_unique<SqliteRepo>(cfg.db_path);
}
```

Фабрика — **деталь** низкого уровня относительно `OrderService`, но **не** внутри сервиса. Граф: Service → IRepo ← Factory (в infrastructure layer).

## Антипаттерн: глобальный `Database::instance()`

Скрытая зависимость; unit-тест **порядок-зависим** от других тестов, меняющих singleton. В multithread — **гонка** на lazy init без call_once. DIP + explicit wiring в `main` — проще для головы и для TSan.

## DIP без virtual: type erasure

`std::function` для `Repo` с сигнатурой `void save(Order)` — **не** interface class, но injection **работает**: lambda или struct с `save`. Compile-time DIP через шаблон — mock struct в тесте без vtable.

Trade-off: ошибки в mock — **link/compile** на вызове, не override mismatch.

## DIP и ThreadPool (развёрнуто)

Smithy **не** создаёт `ThreadPool(4)` внутри — иначе тест smithy **всегда** многопоточный и медленный. Принимает `ThreadPool&` или `function<void(Job)>` — подставляешь **синхронный** fake, который выполняет job сразу. DIP делает **dom logic** тестируемым без sleep в тестах.

## Layering (высокий vs низкий уровень)

| Уровень | Пример | Зависит от |
|---------|--------|------------|
| Domain | `Order`, правила скидок | абстракций repo, не SQLite |
| Application | `OrderService` | `IOrderRepository` |
| Infrastructure | `SqliteOrderRepository` | SQLite API |
| Composition root | `main` | всё конкретное |

Стрелки **внутрь**: infrastructure реализует интерфейсы domain/application. Не наоборот.

## RAII и DIP

`unique_ptr<IConnection>` в сервисе — владение через абстракцию; **deleter** вызывает virtual dtor `IConnection`. DIP + полиморфное уничтожение — одна цепочка с главой 5.

## Manual DI в main (типичный C++)

```cpp
int main() {
    SqliteOrderRepository repo{"app.db"};
    OrderService orders{repo};
    HttpServer server{orders};
    server.run();
}
```

Нет контейнера — **явный** граф. Читается сверху вниз. Тест:

```cpp
TEST(orders, ...) {
    FakeRepo repo;
    OrderService orders{repo};
}
```

Тот же `OrderService`, другая **деталь** — DIP без фреймворка.

## Ошибка: интерфейс в domain, реализация тянет тяжёлый header

`order_service.hpp` include `sqlite3.h` — domain **зависит** от детали. Правило: header сервиса видит только `iorder_repository.hpp` (virtual или pimpl); `sqlite_repo.cpp` include sqlite. **Направление include** = DIP на этапе сборки.

## Типичные заблуждения

«DIP = только virtual» — templates OK.

«new в main — плохо» — new в composition root норм.

«DIP для 10 строк» — overkill.

## К собесу

DIP двумя правилами. Composition root. Отличие injection от Service Locator. Шаблон vs virtual для DIP.

## Практика: mini composition root

На бумаге нарисуй: `main` → `FakeRepo` → `OrderService` → один вызов `place_order`. Добавь стрелку «зависит от интерфейса». Затем замени `FakeRepo` на `SqliteRepo` — какие **файлы** перекомпилируются, если headers правильные? Только `main.cpp` и sqlite adapter — **идеал** DIP.

## DIP и тесты multithread

Если smithy **жёстко** создаёт `ThreadPool` внутри, интеграционный тест **всегда** многопоточный. Injection `IExecutor` с синхронным fake — unit-тест логики заказа **без** sleep. DIP — не только про БД, но про **любую** тяжёлую деталь (сеть, диск, потоки).

## Таблица: injection styles

| Стиль | Плюс | Минус |
|-------|------|-------|
| Constructor + reference | Явный lifetime снаружи | Граф объектов в main длиннее |
| Constructor + unique_ptr | Ясное владение | Аллокации |
| Template parameter Repo | Нет virtual | Длинные типы, header-only |
| function / lambda | Гибко | Аллокации type erasure |

Выбор — не «один правильный»; DIP про **направление**, не про конкретный механизм C++.

## К собесу — extended

Объясни разницу: «есть interface» vs «зависимость от interface в конструкторе». Назови composition root. Почему Service Locator мешает тестам. Приведи compile-time DIP без virtual.

## Заключение блока SOLID

DIP **замыкает** цепочку: абстракции получают смысл, когда модули уже разрезаны (SRP), расширяются без switch (OCP), подтипы честны (LSP), интерфейсы узки (ISP). В следующей главе — антипаттерны и **вся серия** 0–8 одним взглядом. Перед ch14 перечитай **краткое резюме** ch09–ch13 без основного текста — проверка памяти.

## Краткое резюме

Зависи от абстракций; детали в main. Constructor injection + unique_ptr. Шаблон — compile-time DIP. Пул подставляется снаружи. Дальше — итог серии.

## Вопросы для самопроверки

1. DIP своими словами?
2. Где composition root?
3. Зачем FakeRepo?
4. DIP vs интерфейс на классе с Sqlite внутри?
5. Шаблонный OrderServiceT?
6. Как пул связан с DIP?
7. Почему Service Locator хуже?

## Обсуждение с репетитором

**Том:** 8 · **Глава:** `ch13` · **Файл:** `ch13-dip.md`

**Дальше по курсу:** [Глава 14. Антипаттерны и конец серии](./ch14-summary-series.md)
