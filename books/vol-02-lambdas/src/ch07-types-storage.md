# Глава 7. Тип замыкания, decltype и std::function

## Зачем эта тема

Переменная `auto f = [](int x) { return x; };` скрывает **имя типа**, но тип **есть**: уникальный **closure type**. От этого зависят размер на стеке, возможность положить два разных closure в один `vector` без erasure, сообщения об ошибках при несовместимых присваиваниях и использование `decltype` в метапрограммировании.

`std::function` — удобная обёртка с **единым** типом в заголовке API, но с ценой. Эта глава учит выбирать: оставить конкретный closure, шаблон `F`, или `function` — осознанно, а не «всегда function в публичном интерфейсе».

## Имя типа closure

```cpp
auto lam = [](double d) { return d * 2; };
```

Тип `lam` — implementation-defined, часто что-то вроде имени функции с суффиксом `$_0`. Нельзя написать этот тип вручную в **портативном** коде. Варианты:

- оставить `auto` (или `decltype` при возврате из функции);
- передать в `template<typename F> void use(F&& f)`;
- обернуть в `std::function<double(double)>`.

Попытка `void foo(??? lam)` без шаблона и без `function` — тупик: имя типа неизвестно автору исходника.

## decltype для лямбды

```cpp
auto f = [](int i) { return i; };
decltype(f) g = f;
```

`decltype(f)` — точный closure type; `g` — копия того же вида. Полезно в generic code, когда нужно вернуть «такой же callable, как вход», или хранить в tuple с точным типом.

`decltype([](int i){ return i; })` в unevaluated context даёт тип closure **без** создания объекта — правила тонкие; на практике чаще `auto` + trailing return `-> decltype(...)`.

## invoke_result и traits

Для callable `F` и аргументов `Args...` тип результата вызова — **`std::invoke_result_t<F, Args...>`** (C++17). С лямбдой в `auto` это помогает объявить `std::vector` результатов map-подобной операции без дублирования сигнатуры — тема перекликается с parallel map в треке, но без разбора твоего заголовка.

## sizeof и пустые лямбды

```cpp
auto a = []{};
auto b = [x = 1]{};
```

`sizeof(b)` ≥ `sizeof(int)` (поле `x`); `sizeof(a)` часто 1 или 0. Лямбда с большим захватом (`std::string`, `std::vector`, несколько полей) раздувает каждый объект в контейнере. Если в очереди тысячи задач с тяжёлым `[=]`, память уходит на **копии в каждом closure**, не на `function` alone.

## std::function — type erasure

```cpp
std::function<int(int)> f = [](int x) { return x + 1; };
f = [](int x) { return x * 2; };  // другой closure, тот же function
```

Внутри: small buffer optimization (SBO) или указатель на кучу + type erasure vtable (implementation-defined). **Не** использовать `std::function` в tight loop без замера — indirection и возможная аллокация на каждое присваивание новой лямбды.

Ограничения: callable должен быть **CopyConstructible** для классической `function` (C++11 нюансы с move); лямбда только с move-only capture может не лечь в `function` или требует move-only обёрток в новых стандартах — проверяй для своего stdlib.

## function vs template parameter

```cpp
template<typename F>
void run_twice(F f) {
    f();
    f();
}
```

Предпочтительно для performance-critical кода: `F` может быть конcretной лямбдой, **devirtualization** и inline возможны. Минус: код в заголовке или explicit instantiation; каждый новый тип `F` — новая instanciation в бинарнике (code bloat при злоупотреблении).

Публичный API библиотеки иногда стабилизирует тип:

```cpp
using Task = std::function<void()>;
void submit(Task t);
```

Концептуально очередь работ в упражнении с пулом близка: callable с пустой сигнатурой; эталон из `solutions/` не разбираем — только идея erasure vs шаблонная очередь.

## Хранение в контейнерах

`std::vector<???>` не может хранить две разные лямбды без общего типа. Варианты: `vector<std::function<void()>>`, `vector` конкретного closure (все элементы одного lambda-expression в цикле — один тип), или `variant` of callables (C++17). Выбор — trade-off память/perf/API.

## Указатель на функцию снова

Лямбда **без захвата** и с подходящей сигнатурой может **неявно** конвертироваться в указатель на function. С захватом — нет: state потеряется. C API `void (*)(void)` — только stateless lambda или free function.

## Copy assign между разными closure types

```cpp
auto a = [](int x) { return x; };
auto b = [](int x) { return x + 1; };
// a = b;  // ошибка: разные типы
```

Присваивание возможно только между **одним** closure type. Сменить поведение — новый объект или `std::function` erasure.

## exception_specification и лямбда

`noexcept` на лямбде propagates в тип `operator()`. `std::function` может не принять callable с incompatible exception spec в старых реализациях — edge case при обёртывании.

## Optimization: inlining через template

Когда `run_twice(F f)` в заголовке и `F` — конкретная лямбда, компилятор часто inline'ит тело в место вызова. `std::function` добавляет indirect call barrier — LTO иногда восстанавливает, но не стоит полагаться.

## Лямбда в return type функции

```cpp
auto make_add(int k) {
    return [k](int x) { return k + x; };
}
```

Return type — closure type, уникальный для этой функции; разные вызовы `make_add` дают **совместимые** closure types только если компилятор объединяет (обычно один тип на функцию-фабрику). Клиент держит `auto` — не пишет имя типа.

## typeid и lambda

`typeid(f)` для `auto f = []{}` — имя closure type implementation-defined; для сравнения типов двух лямбд используй `decltype` traits, не `typeid` строки между translation units.

## ABI и passing lambda to C

Closure passed as template parameter — C++ only. Export through `extern "C"` — только stateless или function pointer + context.

## Рекомендация для заголовков библиотеки

Публичный header: `using Callback = std::function<void(int)>;` или template `template<class F> void register(F&&);` — второй для header-only performance libs. Не expose `auto` lambda type in public struct field without erasure.

## Packaged erasure size

Some stdlib use 32-byte SBO — one pointer + small buffer; large capture heap.

## Типичные заблуждения

«auto лямбда = нет типа» — тип есть, имя скрыто. «function всегда медленный» — не всегда, SBO спасает мелкие closure. «Две function с одинаковой сигнатурой — один тип» — да, это цель erasure. «decltype лямбды можно написать в .h для экспорта» — нет, без `auto`/шаблона.

## std::move_only_function **(C++23 preview)**

Стандарт добавляет move-only erasure; на C++17 training repo — `function` или template. Если прочитал про move_only_function в интернете — не путай с C++17 baseline.

## Лambda size in cache line

Fat capture (vector, string) — closure не влезает в cache; для hot comparator копируй только int key, не whole object.

## API design sentence

«Принимаем `template<class F> F&&`» — perf. «Принимаем `function<void()>`» — stability. Document choice in header comment.

## Краткое резюме

Closure type уникален; `auto` и `template F` сохраняют его. `std::function` — erasure для стабильного API. `sizeof` растёт с захватом. Выбор — perf vs граница модуля vs удобство контейнера.

## Параграф для Kindle (закрепление)

Перечитай ключевую мысль главы без кода: запиши одно предложение от руки. Для этой главы сформулируй, **что** должно остаться в памяти после чтения, и **какую ошибку** ты больше не хочешь повторять в своём `.cpp`. Связь с треком multithread — только теория; код упражнений по-прежнему твой.

## Вопросы для самопроверки

1. Можно ли написать имя типа лямбды вручную в portable коде?
2. Зачем `template<typename F>` вместо `function`?
3. Почему `function` может аллоцировать?
4. Что даёт `decltype(f)`?
5. Когда лямбда конвертируется в указатель на функцию?
6. Как хранить «разные» лямбды в одном `vector`?
7. Что такое SBO в контексте `std::function`?

## Обсуждение с репетитором

**Том:** 2 · **Глава:** `ch07` · **Файл:** `ch07-types-storage.md`

**Дальше по курсу:** [Глава 8. Лямбды и алгоритмы STL](./ch08-stl-algorithms.md)
