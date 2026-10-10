# Глава 10. Open/Closed

## Зачем эта тема

**Open/Closed Principle (OCP):** программные сущности должны быть **открыты для расширения**, **закрыты для модификации**. Новое поведение добавляется **новым кодом** (класс, функция, template instantiation), а не правкой проверенного `switch` в центре системы.

На собеседованиях OCP связывают с **Strategy** и **virtual**. В C++17 расширение бывает через **шаблоны** и **`std::function`** — OCP не равен «только наследование».

## Антипример: растущий switch

```cpp
enum class Op { Add, Sub, Mul };

int eval(Op op, int a, int b) {
    switch (op) {
    case Op::Add: return a + b;
    case Op::Sub: return a - b;
    case Op::Mul: return a * b;
    }
    throw std::logic_error("unknown");
}
```

Каждая новая операция **Div** — **правка** `eval` и перекомпиляция всех пользователей. Риск сломать Add/Sub. Тесты гоняют всё заново.

## Расширение через иерархию

```cpp
struct BinaryOp {
    virtual int apply(int a, int b) const = 0;
    virtual ~BinaryOp() = default;
};
struct Add : BinaryOp {
    int apply(int a, int b) const override { return a + b; }
};

int eval(const BinaryOp& op, int a, int b) {
    return op.apply(a, b);
}
```

Новый `Mul` — **новый файл** `mul.hpp`, `eval` **не** меняется. OCP на уровне `eval`.

## Расширение через std::function

```cpp
using Job = std::function<void()>;
void ThreadPool::submit(Job job);
```

Новая работа — новая лямбда. Пул **закрыт** для модификации при добавлении типов задач. Это OCP, который ты уже **живёшь** в курсе, без слова «Strategy».

## Расширение через шаблон

```cpp
template<class F, class Range>
auto parallel_map(F f, const Range& input, int n_threads);
```

Любой новый `F` — новый call site, **не** правка `parallel_map`. OCP compile-time. Цена: код в заголовке, инстанцирования.

## OCP не «никогда не трогать файл»

Багфикс в `ThreadPool::submit` — **модификация**. OCP про **добавление фич** без ломания стабильного контракта. Если submit был неверный — чини.

## OCP и тесты

Стабильный `eval(const BinaryOp&)` — regression тесты на Add остаются зелёными при добавлении Mul. Switch — каждый case в одной функции — выше связность тестов.

## Когда switch допустим

Закрытый enum из 3 значений, не растёт, внутренний helper — switch OK. OCP важен на **границах** модулей и плагинов.

## OCP vs YAGNI

Не строй иерархию `Shape` на 10 классов, если приложение рисует один круг. OCP платят **сложностью** абстракции — вводи, когда **уже** видишь второй и третий вариант поведения.

## Связь с pipeline

Новая стадия конвейера — новый поток/функция `pack`, новая очередь в wiring `factory.cpp` (идея), а не правка `BoundedQueue::push` под «упаковку».

## Сценарий: «тип сообщения» в сетевом демоне

Плохо — один класс `Dispatcher` с растущим switch:

```cpp
void dispatch(Message m) {
    switch (m.kind) {
    case Kind::Ping: /* ... */
    case Kind::Login: /* ... */
    // каждый новый Kind — правка здесь + merge conflict
    }
}
```

Лучше — **таблица обработчиков** или полиморфный `MessageHandler`:

```cpp
struct Handler {
    virtual void on(Message m) = 0;
    virtual ~Handler() = default;
};
void dispatch(Message m, span<Handler*> chain);
```

Новый `Kind::Metrics` — новый класс `MetricsHandler`, регистрация в `main`. `dispatch` **стабилен** — OCP на границе протокола.

В C++17 без virtual — `std::unordered_map<Kind, std::function<void(Message&)>>`, заполняемая при старте. OCP сохраняется, цена — map lookup вместо vtable.

## OCP и Rule of Five

Расширяемая иерархия `Handler` с `unique_ptr<Handler>` в векторе требует **virtual ~Handler** (глава 5). OCP через наследование **платит** runtime polymorphism и правила lifetime. Шаблон `dispatch<Handlers...>` (C++20 variadic) — OCP compile-time без vtable, но пересборка при новом типе.

Выбор инструмента — не «кто умнее», а **где** граница стабильности: библиотека (stable ABI реже) vs приложение (header-only OK).

## Антипаттерн: «наследуюсь от std::function»

Не делай `class MyJob : public std::function<void()>`. Наследование от STL **ломает** инварианты и не даёт OCP — клиент всё равно видит полный interface function. OCP здесь — **принимать** `std::function<void()>` как есть; новые jobs — новые лямбды на call site.

## OCP в parallel_map (идея курса)

Сигнатура шаблона **закрыта** для модификации при новом `F`: ты не добавляешь `case` «умножить на два» внутрь `parallel_map`. Новый алгоритм — новая лямбда `f`. Стабильный контракт: «разбей диапазон, примени `f`, собери результат». Это OCP **без** virtual — важный ответ на собеседовании.

## Когда OCP **не** стоит применять

- Прототип на один вечер;
- enum из двух значений, закрытый навсегда;
- hot path, где virtual unacceptable — тогда explicit switch **внутри** одного `.cpp` может быть осознанным компромиссом.

OCP про **изменяемые** границы. Если продукт **никогда** не добавит третий Kind — абстракция рано.

## OCP + тесты (кейс regression)

Команда добавляет `Mul` к `BinaryOp`. С virtual: старые тесты `Add`, `Sub` **не перекомпилировались** логически — только линковка нового translation unit. С switch: diff в `eval` — ревьюер смотрит **весь** switch на регрессии. OCP снижает **когнитивную** нагрузку review, не только строки кода.

## Visitor vs switch (намёк за рамками тома)

Классический **Visitor** добавляет **новую операцию** над иерархией без правки каждого Node — но **правит** все Visitor классы при **новом типе** Node. OCP «открыт для расширения» **асимметричен**: virtual hierarchy хорош для **новых типов**, Visitor — для **новых операций**. Запиши trade-off; на собеседовании это уровень «понимает, а не зубрит».

## std::variant + std::visit (C++17)

Закрытый набор `Circle | Rectangle` — visit добавляет операцию **одним** switch в одном месте; новый тип — **правка variant** и всех visit. OCP снова асимметричен. Выбор между variant и virtual — **частота** добавления типов vs операций.

## Registry idiom (C++17)

```cpp
using Handler = std::function<void(Message&)>;
std::unordered_map<Kind, Handler> handlers;

void dispatch(Message& m) {
    handlers.at(m.kind)(m);
}
```

Регистрация в `main`: `handlers[Kind::Ping] = [](Message& m){ /* ... */ };`. Новый Kind — одна строка регистрации + новая лямбда/функция. `dispatch` **стабилен** — OCP. Ошибка `at` при unknown Kind — fail fast.

## Типичные заблуждения

«OCP = virtual всегда» — function/template тоже.

«Закрыт = immutable файл» — нет.

«Switch всегда зло» — внутри модуля часто норм.

## К собесу

Объясни OCP. Покажи switch vs virtual vs template на одном примере. Как `submit` в пуле открыт для расширения?

## Практика: один switch в голове

Вспомни любой `if/else` или `switch` по «типу команды» из игры, CLI или учебного проекта. Запиши **три** способа добавить новую ветку без правки центра (virtual, map+function, template). Выбери один **для твоего** масштаба — обоснуй одним предложением (YAGNI vs рост). Обсуди с репетитором, если сомневаешься.

## OCP и SRP вместе

Switch на 20 `Kind` — часто **и** OCP нарушен, **и** SRP (разные домены в одной функции). Рефакторинг: сначала **вынеси** каждый case в функцию/класс (SRP), затем **регистрация** handlers (OCP). Не наоборот — абстракция без границ не спасёт.

## Краткое резюме

Расширяй новым кодом, не центральным switch. virtual, function, template — инструменты. Пул и parallel_map — примеры из курса. Не over-abstract рано. Дальше — LSP.

## Вопросы для самопроверки

1. Сформулируй OCP.
2. Почему switch на Op нарушает OCP?
3. Три способа OCP в C++17?
4. Как submit связан с OCP?
5. Когда switch приемлем?
6. OCP vs багфикс?
7. YAGNI vs OCP?

## Обсуждение с репетитором

**Том:** 8 · **Глава:** `ch10` · **Файл:** `ch10-ocp.md`

**Дальше по курсу:** [Глава 11. Liskov Substitution](./ch11-lsp.md)
