# Глава 6. Композиция и наследование

## Зачем эта тема

В учебниках ООП наследование рисуют стрелками на диаграммах — и мозг цепляется: «расширить = унаследовать». В зрелом C++ чаще говорят: **prefer composition over inheritance**. Не потому что наследование «плохое», а потому что **has-a** гибче, не ломает LSP и не тащит лишний интерфейс.

Глава даёт **алгоритм выбора** и разбирает, как твой **ThreadPool** и **pipeline** уже следуют композиции, даже если ты не называл это SOLID.

## Has-a: поле внутри класса

```cpp
class EmailService {
    Logger log_;           // has-a
    std::string smtp_host_;
public:
    void send(const Message& m);
};
```

`EmailService` **не** наследует `Logger` — он **использует** logger. Можно заменить реализацию логгера, тестировать с mock logger через конструктор (DIP, глава 13).

## Is-a: public наследование

```cpp
class Circle : public Shape { };
```

Клиент с `Shape&` должен корректно работать с `Circle` — контракт LSP. Если Circle меняет семантику `setWidth` — ловушка Rectangle/Square (глава 11).

**Вопрос перед is-a:** «Подставлю ли я Derived вместо Base **везде** без сюрпризов?» Нет → не public наследование.

## Делегирование

Метод класса перенаправляет вызов члену:

```cpp
class Server {
    ThreadPool pool_;
public:
    void submit(Job j) { pool_.submit(std::move(j)); }
    void wait_idle() { pool_.wait_idle(); }
};
```

Снаружи «сервер умеет submit» — но реализация **внутри** пула. Это не наследование `Server : ThreadPool`, а **композиция + тонкая обёртка**.

## Пул потоков как композиция примитивов

Концептуально (твой код, не эталон):

- `std::vector<std::thread> workers_` — owns потоки;
- `std::deque<Job> jobs_` — очередь;
- `std::mutex m_`;
- `std::condition_variable cv_`;
- флаги `stopped_`, счётчик `busy_`.

Ни одно из этого **не** is-a «пул». Пул **собирает** примитивы и публикует API. Если бы ты сделал `class ThreadPool : std::deque<Job>` — клиент мог бы вызвать `clear()` на очереди и сломать воркеров.

## Pipeline: стадии и очереди

Конвейер: стадии **имеют** bounded queue между собой. Стадия **не** наследует очередь. Причины изменения разные: формат `Item` vs политика блокировки очереди (SRP, глава 9).

## Когда наследование всё же уместно

- Иерархия **смысловых** подтипов с общим интерфейсом: рендереры, парсеры, стратегии скидок.
- Template Method: база вызывает `virtual hook()` в derived — но осторожно с хрупкой базой.

## std::variant вместо иерархии Shape*

```cpp
using Shape = std::variant<Circle, Rectangle>;
```

Закрытый набор типов без vtable. Добавление типа — правка variant и visit — OCP сложнее, но нет slicing и heap.

## Law of Demeter (смежная идея)

«Не разговаривай с незнакомцами»: метод не должен цепочкой `a.getB().getC().mutate()`. Лучше `a.doWhatINeed()`. Уменьшает coupling — дух инкапсуляции и SRP.

## Упражнение: переписать в голове

Возьми гипотетический `class Game : Network, Graphics, Audio`. Три базы — хрупко. Альтернатива: поля `NetworkClient net_;`, `Renderer rend_;`, `AudioEngine audio_;`. Game **координирует**, не **является** сетью.

## Сценарий: «наследуем ThreadPool, добавляем метрики»

Плохо: `class MonitoredPool : public ThreadPool` — клиент может вызвать унаследованный API и **обойти** счётчики, или LSP ломается, если override меняет семантику `submit`. Лучше: поле `ThreadPool pool_;` + методы `submit` с инкрементом метрик **внутри** обёртки. Has-a сохраняет контракт пула.

## Композиция и тестирование (DIP)

Подставь **fake** executor вместо пула в smithy — композиция на уровне wiring, не override virtual в derived pool. Глава 13 вернётся к этому с injection.

## «Пустая» иерархия ради полиморфизма

Иногда создают `AbstractThreadPool` с одним concrete — YAGNI. Пока **один** тип пула — concrete class + composition внутри достаточно. Вторая реализация (process pool vs thread pool) — тогда interface.

## К собесу

«Composition over inheritance» — пример has-a vs is-a; pipeline и пул; когда variant вместо Shape*.

## Pipeline ещё раз (SRP + composition)

Стадия «штамп» **имеет** входную очередь и выходную — две композиции. Она **не** наследует `BoundedQueue`, потому что «StampStation is-a Queue» бессмысленно. Изменение формата `Item` — правки стадии; изменение `cap_` — правки очереди. Два класса — два review, два набора тестов — **здоровая** архитектура курса.

## Наследование для policy (advanced намёк)

В template metaprogramming `vector<T, Allocator>` **композиция** allocator; иногда policy наследуют через CRTP — это **не** runtime ООП тома 8. Не смешивай: CRTP — compile-time, virtual — runtime.

## Типичные заблуждения

«Наследование = код reuse» — композиция reuse без подтипа.

«private наследание = композиция» — почти; лучше явное поле.

«Глубокая иерархия = хороший дизайн» — часто нет.

## Сценарий: Decorator vs inheritance

«Хочу ThreadPool с логированием каждого submit». Decorator: класс с полем `ThreadPool& inner_` и методом `submit`, который log + delegate. **Не** `LoggedPool : ThreadPool` — иначе клиент может вызвать базовый `submit` без лога через upcast. Комposition **контролирует** forwarding.

## Агрегация vs композиция (термины)

**Композиция** — часть не живёт дольше целого (pool owns threads). **Агрегация** — ссылка на внешний объект (`Smithy` использует `ThreadPool&`). В C++ lifetime выражай типом: `unique_ptr` vs `&` + comment/DIP.

## Краткое резюме

Has-a полями; is-a только для подтипа. Пул и очередь — композиция sync и контейнеров. Делегирование вместо public наследования от реализации. variant — альтернатива иерархии. Дальше — RAII.

## Вопросы для самопроверки

1. Алгоритм: наследование или поле?
2. Почему пул не наследует deque?
3. Что такое делегирование на примере submit?
4. Когда variant лучше Shape*?
5. Что нарушает Demeter?
6. Пример плохого множественного is-a?
7. Как pipeline разделяет ответственность?

## Обсуждение с репетитором

**Том:** 8 · **Глава:** `ch06` · **Файл:** `ch06-composition-vs-inheritance.md`

**Дальше по курсу:** [Глава 7. RAII и Rule of Five](./ch07-raii-rule-of-five.md)
