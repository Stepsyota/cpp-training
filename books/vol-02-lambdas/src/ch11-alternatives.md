# Глава 11. Функтор, bind и указатель на функцию

## Зачем эта тема

Лямбда не единственный callable в C++. **Именованный функтор** читается в больших системах годами; **указатель на функцию** — для C API и vtables; **`std::bind`** — легаси связывания аргументов до лямбд и init-capture. Понимание альтернатив помогает читать старый код Boost/Qt и выбирать инструмент: не «всегда лямбда», а «минимальный ясный callable».

## Именованный функтор

```cpp
struct IsPositive {
    bool operator()(int x) const { return x > 0; }
};
std::count_if(v.begin(), v.end(), IsPositive{});
```

Плюсы: переиспользование в нескольких translation units, явный тип в отладчике и в stack trace, state в полях без синтаксиса `[]`, можно документировать invariant класса. Минусы: бойлерплейт для одноразового predicate из двух строк.

Лямбда — синтаксический сахар над **локальным** unnamed функтором. В публичном заголовке библиотеки `struct LessByField` часто предпочтительнее лямбды в `.h`.

## Stateful policy как class

```cpp
struct RateLimiter {
    int max_per_sec;
    mutable int count = 0;
    bool operator()() const {
        return ++count <= max_per_sec;
    }
};
```

Тот же state, что `[count = 0]() mutable`, но имя типа видно в профайлере.

## Указатель на функцию

```cpp
void log_line(const char* msg);
void run(void (*fp)(const char*)) { fp("ok"); }
```

Нет состояния. Лямбда **без capture** с совместимой сигнатурой может неявно конвертироваться в `void(*)()`. Для state — `void* user_data` + static function в C стиле:

```cpp
void trampoline(void* ctx) {
    static_cast<MyType*>(ctx)->on_event();
}
```

Лямбда с capture в такой API напрямую не ляжет без heap context.

## std::bind (легаси)

```cpp
using namespace std::placeholders;
auto f = std::bind(printf, "%s\n", _1);
```

Создаёт callable с **замороженными** аргументами и placeholder. Проблемы: тяжёлые типы ошибок, путаница с копиями vs references, `bind` на overloaded function, interaction с move. В новом C++ предпочитают лямбду:

```cpp
auto f = [](const char* s) { std::printf("%s\n", s); };
```

`bind` остаётся в коде Boost.Asio старых версий, `std::thread` ранних примеров — уметь **читать**.

## mem_fn и bind к методу

```cpp
auto call = std::bind(&Worker::run, &w, _1);
```

Заменяется `[&w](auto arg) { w.run(arg); }` или `[ptr = &w](auto arg) { ptr->run(arg); }` с проверкой lifetime `w`/`ptr`.

## Boost.Function и poly функции (история)

До `std::function` использовали Boost.Function — та же идея erasure. Лямбды C++11 сделали паттерн mainstream. Читая код 2010-х, видишь `boost::function` + `bind`.

## Overloaded function и bind/lambda

Address of overloaded function требует cast. Лямбда `[](auto... args) { return overloaded(args...); }` **(C++14)** — обход для передачи overload set в API, ожидающий один callable type.

## noexcept и function pointer

Function pointer не хранит `noexcept`; `std::function` может учитывать. Для callback в real-time иногда требуют `noexcept` callable — лямбда с `noexcept` spec подходит.

## function pointer в virtual interface

Классический C++ до лямбд: virtual `on_click()` вместо `std::function`. Лямбды не заменили virtual — другой слой design.

## Когда что выбирать (сценарии)

Локальный predicate в одной функции — **лямбда**. Переиспользуемый policy в заголовке — **struct functor**. C callback без state — **function pointer**. C callback со state — **context + trampoline** или C++ wrapper с `function`. Старый Boost — **читать bind**, новый код — **лямбда** с init-capture.

## Связь с томом 2

Если альтернатива — `std::function`, см. главу 7. Если capture by reference — главы 3 и 10.

## Function object + state в STL

`std::sort` с `std::reference_wrapper` comparators — редко; обычно лямбда или функтор. `std::owner_less` для shared_ptr — named functor example in standard.

## Лямбда вместо macro

Макрос `FOR_EACH` хуже лямбды + `for_each` — лямбда type-checked. Альтернатива macro — не bind, а algorithm + lambda.

## Читаемость legacy: разбор bind expression

Видишь `bind(&Class::method, _1, ref(x))` — переводи мысленно в lambda с capture `[&x]` и call `method`; проверяй lifetime `x`.

## Function object traits

`std::is_invocable_v<Lambda, Args...>` **(C++17)** — traits for generic APIs.

## Типичные заблуждения

«bind устарел = запрещён» — нет, но не пишем в новом коде без причины. «Функтор медленнее лямбды» — обычно одинаковый codegen. «function pointer гибче лямбды» — наоборот для state. «Лямбда заменила все функторы» — нет, именованные типы живы.

## std::invoke **(C++17)**

`std::invoke(f, args...)` unified call for function, member pointer, functor — lambda path is `operator()`. Useful in generic libraries you read, not required to write daily.

## Comparison with Python/Java closures

GC languages extend lifetime; C++ does not. Alternative «always shared_ptr» — C++ idiom sometimes, not language rule.

## When reviewer asks for functor

Public API stability, documentation, or reuse across TUs — say yes to struct.

## Краткое резюме

Лямбда ≈ локальный функтор. Function pointer — без state. `bind` — легаси, замена — лямбда с init-capture. Выбор по читаемости, границе API и legacy.

## Параграф для Kindle (закрепление)

Перечитай ключевую мысль главы без кода: запиши одно предложение от руки. Для этой главы сформулируй, **что** должно остаться в памяти после чтения, и **какую ошибку** ты больше не хочешь повторять в своём `.cpp`. Связь с треком multithread — только теория; код упражнений по-прежнему твой.

## Вопросы для самопроверки

1. Когда struct предпочтительнее лямбды?
2. Может ли лямбда с захватом стать `void(*)()`?
3. Чем `bind` хуже лямбды для нового кода?
4. Зачем C API использует `void*` context?
5. Эквивалент `bind(&W::run, &w, _1)` на лямбде?
6. Почему в .h библиотеки редко экспортируют лямбду как тип?
7. Stateless lambda и free function — в чём разница для ABI?

## Обсуждение с репетитором

**Том:** 2 · **Глава:** `ch11` · **Файл:** `ch11-alternatives.md`

**Дальше по курсу:** [Глава 12. Резюме и мифы](./ch12-summary-myths.md)
