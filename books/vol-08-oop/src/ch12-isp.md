# Глава 12. Interface Segregation

## Зачем эта тема

**Interface Segregation Principle (ISP):** клиенты **не должны** зависеть от интерфейсов, которые они **не используют**. «Жирный» интерфейс заставляет реализации **заглушать** методы (`return false;`, пустые тела) и тянет **лишние** пересборки при изменении ненужных частей API.

В C++ «интерфейс» — часто **абстрактный класс** с чистыми virtual, иногда **concept** **(C++20)**, иногда просто **минимальный** набор free functions.

## God interface

```cpp
struct IDevice {
    virtual void print(Document) = 0;
    virtual void scan() = 0;
    virtual void fax(Document) = 0;
    virtual void staple() = 0;
};
```

Принтер реализует `scan`, `fax`, `staple` как stub или throw — **запах ISP**. Лучше:

```cpp
struct IPrinter { virtual void print(Document) = 0; virtual ~IPrinter() = default; };
struct IScanner { virtual Image scan() = 0; virtual ~IScanner() = default; };
```

Клиент печати зависит только от `IPrinter`.

## ISP и заголовки

#include тяжёлого `idevice.hpp` с 40 методами тянет перекомпиляцию. Мелкие интерфейсы в мелких заголовках — **compile-time ISP**.

## Разделение Reader / Writer

Идея для очереди (концептуально):

- потребителю нужен только **pop**;
- производителю — **push** и **close**.

Один класс `BoundedQueue` может реализовать оба интерфейса, но **клиент** получает ссылку на `IConsumer` или `IProducer` (если выделишь такие абстракции). Тест consumer не видит push — меньше coupling.

В учебном курсе один класс с полным API — OK по размеру; **мысленно** ISP: не заставляй стадию «штамп» вызывать методы упаковки.

## std::function как узкий интерфейс

Воркеру пула нужен `void()` — не весь `ThreadPool`. `submit` принимает **минимальный** callable — ISP на уровне типа.

## ISP vs SRP

SRP — **внутри** модуля одна причина изменения. ISP — **снаружи** клиент не видит лишнего. Толстый интерфейс может быть SRP нарушением внутри (всё про устройство), ISP — как это бьёт **потребителей**.

## Когда не дробить

Один cohesive API `fstream` read+write — файл **и** читается, и пишется осознанно. Дробление на `IRead`/`IWrite` для каждого вызова — шум. ISP когда **реальные** клиенты используют **подмножества**.

## Сценарий: «IWorker» для людей и роботов

```cpp
struct IWorker {
    virtual void eat() = 0;
    virtual void sleep() = 0;
    virtual void work() = 0;
};
struct Robot : IWorker {
    void eat() override {}   // stub
    void sleep() override {}
    void work() override { /* ... */ }
};
```

Робот **вынужден** знать про `eat`/`sleep` — ISP нарушен. Разделение:

```cpp
struct IWorkable { virtual void work() = 0; virtual ~IWorkable() = default; };
struct IHumanNeeds {
    virtual void eat() = 0;
    virtual void sleep() = 0;
    virtual ~IHumanNeeds() = default;
};
```

`Human` implements оба; `Robot` — только `IWorkable`. Функция `assign_task(IWorkable&)` не тянет биологию.

## ISP и заголовки (compile-time coupling)

Fat header `all_interfaces.hpp` с 200 строками virtual — **каждый** `.cpp`, который include его, **пересобирается** при добавлении метода в **чужой** интерфейс. Мелкие заголовки — ISP для **компилятора**. В больших C++ кодовых базах это иногда важнее runtime.

## Множественное наследование **интерфейсов**

```cpp
class BoundedQueue final : public IProducer, public IConsumer {
    // один объект — два «лица» для разных клиентов
};
```

Два **абстрактных** базовых без state — идиома COM/CORBA, нормальна в C++. Diamond problem — когда **два** базовых несут **данные**. ISP часто **мотивирует** второй базовый интерфейс без полей.

## ISP vs «минимальный API» класса

Класс `ThreadPool` **может** иметь `submit`, `wait_idle`, деструктор — полный API для **владельца** пула. ISP спрашивает: **код воркера** внутри `.cpp` пула не должен вызывать `wait_idle` снаружи — это **внутренняя** деталь. Снаружи клиент smithy видит только `submit` — **фактически** узкий интерфейс, даже без отдельного `ISubmitter`.

## Антипаттерн: интерфейс на каждый метод

`IAdd`, `ISub`, `IMul` — карикатура. ISP не требует **атомарных** интерфейсов; требует **смысловых** ролей потребителя. `IPrinter` с `print` — OK; `IPrintLine` и `IPrintChar` — обычно шум.

## concepts (C++20) как ISP compile-time

Concept `Readable` с `read()` — клиент шаблона **не зависит** от `write()`. Это ISP перенесённый на этап компиляции. Том держит C++17; запиши себе: «concepts = ISP для templates».

## Связь с лямбдами (том 2)

Тип лямбды **уникален**; в API передаёшь `std::function<void()>` — **стираешь** всё, кроме `operator()`. ISP: воркер не видит capture, `mutable`, state лямбды — только callable. Это **намеренное** сужение.

## Fat interface в legacy C++

Старый `CApi` с сотней функций в одном `.h` — все клиенты зависят от всего. Обертка C++ иногда **дробит** на несколько thin классов, каждый держит pointer на общий context — ISP на уровне **facade**. Полный рефакторинг DLL редко возможен; постепенное сужение include — реальный путь.

## Тесты и ISP

Unit-тест consumer очереди **не должен** линковать код producer-only методов, если архитектура строгая — на практике один класс = один тест object, но **mock** реализует только `IConsumer` — меньше stub методов «throw not implemented».

## Типичные заблуждения

«ISP = 100 интерфейсов» — нет.

«Один класс = один интерфейс» — класс может implement несколько **узких** интерфейсов.

«public наследование от двух интерфейсов» — нормально для ISP (множественное наследование **интерфейсов** без state).

## К собесу

ISP vs fat interface. Printer/Scanner split. Как function сужает API? ISP vs SRP?

## Практика: нарисуй стрелки include

На листе: три `.cpp` — consumer, producer, full. Какие заголовки **должны** включать каждый, если ISP строгий? Если все включают `bounded_queue.hpp` с полным API — ISP нарушен **на compile-time**, даже если runtime OK. Подумай, как бы ты **разделил** заголовки, не ломая учебный API курса (мысленный эксперимент).

## ISP и DIP

Интерфейс `IRepo` с 20 методами заставляет **FakeRepo** реализовать 20 stub — тесты хрупкие. Узкий `IOrderWriter` с `save` — DIP injection проще, ISP соблюдён. Оба принципа про **уменьшение surface area**.

## Краткое резюме

Клиент не зависит от неиспользуемых методов. Дроби интерфейсы по ролям потребителей. function — узкий callable. Не дроби без реальных подмножеств клиентов. Дальше — DIP.

## Вопросы для самопроверки

1. ISP своими словами?
2. Что не так с IDevice?
3. Reader/Writer для очереди — зачем?
4. ISP и #include?
5. submit и узкий тип?
6. ISP vs SRP?
7. Когда не дробить fstream?

## Обсуждение с репетитором

**Том:** 8 · **Глава:** `ch12` · **Файл:** `ch12-isp.md`

**Дальше по курсу:** [Глава 13. Dependency Inversion](./ch13-dip.md)
