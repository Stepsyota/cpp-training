# Глава 10. Lifetime, dangling и std::ref

## Зачем эта тема

Лямбда — объект с полями-ссылками, указателями и копиями. Компилятор **не** проверяет, что referent переживёт каждый вызов `operator()`, если ссылка захвачена извне. **Dangling reference** и **dangling this** — частые баги в асинхронном и callback-коде. Эта глава собирает сценарии из глав 3–9 в чеклист для review и для чтения чужих PR.

Думай о closure как о **мини-struct**: поле `T&` не продлевает жизнь `T`, поле `T` — продлевает копию.

## Висячая ссылка из `[&]`

```cpp
std::function<void()> make_bad() {
    int x = 0;
    return [&x]() { ++x; };
}
```

`std::function` копирует closure при return; внутри — ссылка на стек уже уничтоженного фрейма `make_bad`. Вызов после return — UB. Исправление: `[x]`, `[x = x]`, `shared_ptr` на state (если оправдано), или не возвращать так.

Тот же баг без `function`: вернуть closure type с `[&]` на локальное — UB при вызове у caller.

## this и асинхронность

```cpp
class Host {
public:
    void start() {
        std::thread([this]() { work(); }).detach();
    }
    ~Host() { /* поток может ещё бежать */ }
};
```

`[this]` не продлевает жизнь `Host`. Нужен join в деструкторе, флаг остановки и пробуждение cv, или `[self = *this]` **(C++17)** если копия объекта осмысленна и поток безопасно работает с копией.

На уровне идей пула: деструктор ставит stopped, будит воркеров, **join без** mutex, который держит воркер — ты это уже проходил на практике.

## Захват параметра функции по ссылке в async

```cpp
void foo(const std::string& msg) {
    std::async(std::launch::async, [&]() { log(msg); });
}
```

`msg` — ссылка на объект, lifetime которого может закончиться при выходе из `foo` **до** выполнения async. Нужна копия: `[msg]` или `[m = msg]`.

## Захват iterator и pointer

```cpp
std::vector<int> v{1, 2, 3};
auto it = v.begin();
auto f = [it]() { return *it; };
v.push_back(4);
f();
```

Лямбда хранит **копию итератора**, не «привязку к vector». После reallocation — UB при разыменовании. То же для `T*` на элемент vector.

## std::ref и reference_wrapper

```cpp
int n = 0;
auto f = [r = std::ref(n)]() { r.get() = 1; };
```

`reference_wrapper` в capture — способ положить «ссылку» в контекст, где нужна **копируемая** обёртка (tuple, legacy `bind`). Lifetime referent **всё равно** на тебе; `ref` не RAII и не shared ownership.

`std::cref` для const ref в похожих сценариях.

## Лямбда в init-statement if (C++17)

```cpp
if (auto pred = [&](int x) { return x > limit; }; pred(1)) { }
```

`limit` должен жить в той же области; scope `pred` ограничен if — хорошо для локальных predicate без утечки наружу.

## dangling при std::function и move

При move-assign новой лямбды в `function` старая state уничтожается; убедись, что никто не держит указатель внутрь старого closure.

## Санитайзеры и статический анализ

ASan/UBSan (том 7) иногда ловят use-after-free на захваченных ссылках; многие случаи — тихий UB. Clang static analyzer и warnings на lifetime — помощь, но дисциплина capture надёжнее.

## optional и variant в capture

Захват `[opt = optional<T>{}]` by value безопасен при return. Захват `[&opt]` на optional в stack frame — тот же класс проблем. **`std::variant` of callables** хранит альтернативы by value — lifetime внутри variant OK; ссылки внутри variant rare.

## weak_ptr в capture (обзор)

`[wp = std::weak_ptr{sp}]` в async: в теле `lock()` — если объект умер, не dereference. Это **не** автоматически «правильный async», но лучше сырого `[this]` на `shared_from_this` объект. Overhead `shared_ptr` — trade-off.

## Лямбда, хранящаяся в member класса

```cpp
struct Holder {
    std::function<void()> cb;
    void set() { cb = [&]() { use(*this); }; }  // опасно
};
```

Если `cb` переживает вызов `set` и вызывается после уничтожения `Holder` — UB. Member callback почти всегда требует `weak_ptr`, `shared_ptr` на self, или отмены при destroy.

## Практика review: вопросы к каждой лямбде

Кто владелец referent? Доживёт ли до последнего вызова? Есть ли reallocation контейнера под pointer/iterator? Это `[this]` на короткоживущем объекте?

## Loop variable и async (классика)

```cpp
for (auto& item : items)
    pool.submit([&]{ handle(item); });  // UB: item меняется
```

Fix conceptually: capture `item` by value в init `[it = item]` или copy pointer после C++20 ranges algorithms sync. Ты мог видеть это в интернете — здесь формулировка без кода пула.

## string_view в capture

`string_view` не владеет данными; `[sv = std::string_view{buf}]` или `[&sv]` — пока `buf` жив. Async + view на stack buffer — UB после return.

## ASan use-after-scope

Compile test with `-fsanitize=address` on small dangling example — visceral learning tom 7 preview.

## Типичные заблуждения

«Компилятор предупредит о dangling» — редко для capture by reference. «shared_ptr в capture всегда правильный» — может быть избыточен; иногда достаточно копии значения. «const ref параметр безопасен в async» — параметр живёт до конца **синхронного** вызова функции, не до async job.

## optional local and callback

Same as reference: `[&opt]` escaping — UB.

## Capturing this in member init list

Rare corner: don't bind `[this]` before object fully constructed in constructor body — use after construction complete or avoid.

## Culture of team review

Ask «where is last call site of this lambda?» — if unknown, no `[&]` on locals.

## Краткое резюме

`[&]` и `this` — проверяй lifetime до последнего вызова. Не возвращай лямбды с ссылками на локальные. Итераторы и указатели в capture — инвалидация. `std::ref` не продлевает жизнь.

## Таблица решений (prose, без HTML)

Если callable **остаётся внутри функции** и вызывается до return — допустим `[&]` на locals и параметрах. Если callable **кладётся в очередь, thread, function** — только value, init-capture, `[*this]` при осмысленной копии, или `shared_ptr` state. Если нужен **mutable счётчик** в одном потоке — mutable lambda OK. Если **несколько потоков** на одном closure — mutex или atomic, не mutable int. Если сомневаешься — value capture; цена копии часто меньше цены UB.

Эта таблица в голове заменяет десяток StackOverflow ответов с `[&]`.

## Параграф для Kindle (закрепление)

Перечитай ключевую мысль главы без кода: запиши одно предложение от руки. Для этой главы сформулируй, **что** должно остаться в памяти после чтения, и **какую ошибку** ты больше не хочешь повторять в своём `.cpp`. Связь с треком multithread — только теория; код упражнений по-прежнему твой.

## Вопросы для самопроверки

1. Почему `return [&x]{}` опасен?
2. Как `[*this]` **(C++17)** меняет картину?
3. Что инвалидирует итератор в capture?
4. Зачем `std::ref` в capture?
5. Где заканчивается жизнь `const std::string&` параметра при fire-and-forget async?
6. Безопасен ли `[&]` на член класса в методе?
7. Чем опасен detach + `[this]`?

## Обсуждение с репетитором

**Том:** 2 · **Глава:** `ch10` · **Файл:** `ch10-lifetime-pitfalls.md`

**Дальше по курсу:** [Глава 11. Альтернативы лямбде](./ch11-alternatives.md)
