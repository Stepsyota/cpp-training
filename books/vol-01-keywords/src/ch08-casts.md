# Глава 8. Приведения типов

## Зачем эта тема

В C из учебника приведение записывают одной парой скобок: `(double)n` или `(char*)buf`. Компилятор C++ тоже понимает такой синтаксис — это **C-style cast** — но язык с **C++98** предложил четыре **именованных keyword**: `static_cast`, `dynamic_cast`, `const_cast` и `reinterpret_cast`. Каждое слово сужает, *какой* вид преобразования ты задумал, и тем самым отвечает на вопрос ревьюера: «ты меняешь число, иерархию классов, const или биты в памяти?».

Без этой главы легко смешать **безопасный upcast** с **опасным downcast**, снять `const` «на минутку» и получить неопределённое поведение, или переписать float в int через указатель и удивиться результатам под UBSan. Ты уже видел `static_cast` в чужом коде рядом с `enum class` (глава 3) и `const` (глава 4); здесь мы собираем casts в одну картину и отмечаем, что **не keyword**, но часто заменяет `reinterpret_cast`: **(C++20)** `std::bit_cast` из `<bit>`.

Примеры ниже — **C++17**, если не сказано иначе. На Fedora проверяй `g++ -std=c++17 -Wall -Wextra -pedantic`.

## Почему C-style cast прячет намерение

C-style cast — не пятый keyword, а синтаксический сахар. Компилятор по правилам стандарта **перебирает** const_cast, static_cast и reinterpret_cast (и иногда комбинирует const_cast со static_cast), пока не найдёт допустимое преобразование. Ты в исходнике видишь одни скобки, а машина могла снять const или переинтерпретировать указатель.

```cpp
void demo(const int& x) {
    int* p = (int*)&x;   // часто const_cast + static_cast
    *p = 0;              // UB, если x действительно const
}
```

В новом коде вместо `(T)e` пишут конкретный cast. Исключение — взаимодействие с макросами C или очень старыми заголовками; даже там, где можно, лучше явное имя.

## `static_cast` — «обычная» семантика типов

Keyword **`static_cast`** покрывает преобразования, которые компилятор **может проверить на этапе компиляции** (с оговорками для указателей в иерархии). Это главный рабочий инструмент.

### Числа, enum и явные конструкторы

Сужение `double` → `int`, расширение целых, приведение к **(C++11)** `enum class` через underlying type — всё сюда. Компилятор предупредит о потере данных, если включить соответствующие флаги.

```cpp
enum class Color : unsigned char { Red, Green, Blue };

int n = 42;
double d = static_cast<double>(n);
auto c = static_cast<Color>(2);  // только если 2 — допустимое значение
std::string s = static_cast<std::string>("tmp");
```

Последняя строка — явный вызов конструктора: иногда так отключают **narrowing** в списках инициализации или делают намерение видимым.

### Указатели и ссылки в иерархии

**Upcast** (от производного к базовому) через `static_cast` законен и не требует virtual. **Downcast** (от базы к производному) компилятор пропустит **без проверки runtime типа** — ответственность на тебе.

```cpp
struct Base { virtual ~Base() = default; };
struct Derived : Base { void tick() {} };

Derived d;
Base* b = static_cast<Base*>(&d);
Derived* back = static_cast<Derived*>(b);  // OK только если b реально указывает на Derived
```

Если `b` пришёл из «чужого» места и на самом деле указывает только на `Base`, `static_cast<Derived*>` даст указатель, но разыменование — UB. Для проверки нужен `dynamic_cast` (ниже).

### `void*` и обратно

Указатель на объект можно перевести в `void*` и **обратно в тот же тип** через `static_cast`, если исходный тип был тем же (не «любой» указатель подряд).

```cpp
int n = 7;
void* vp = static_cast<void*>(&n);
int* np = static_cast<int*>(vp);
```

Это не «сериализация произвольной памяти», а согласованная пара преобразований в рамках одного объекта.

### Чего `static_cast` не делает

Он **не** снимает `const`/`volatile` — для этого `const_cast`. Он **не** произвольно переинтерпретирует биты одного типа как другой (float как int) — для trivially copyable одинакового размера в **(C++20)** предпочтительнее `std::bit_cast`.

## `dynamic_cast` — runtime тип и RTTI

Keyword **`dynamic_cast`** работает только там, где включена **run-time type information (RTTI)**: в базовом классе должен быть **полиморфный** тип, обычно достаточно одного **virtual** метода (часто виртуальный деструктор).

### Указатели: неудача — `nullptr`

```cpp
Base* b = fetch();  // полиморфная база
if (auto* d = dynamic_cast<Derived*>(b)) {
    d->tick();
} else {
    // другой подтип или nullptr
}
```

Это идиоматичный безопасный downcast: ветка с `d` выполняется только при успехе.

### Ссылки: неудача — исключение

Для ссылок нет «пустого» значения, поэтому неудачный `dynamic_cast` бросает **`std::bad_cast`**.

```cpp
void use(Base& b) {
    try {
        Derived& d = dynamic_cast<Derived&>(b);
        d.tick();
    } catch (const std::bad_cast&) {
        // тип не Derived (и не подтип с нужной layout)
    }
}
```

### Стоимость и альтернативы

`dynamic_cast` ходит по таблице типов. В горячем цикле иногда заменяют дизайн: **variant**, tag enum, visitor — это уже архитектура (том 8), не keywords. Отключение RTTI (`-fno-rtti`) ломает `dynamic_cast` — помни про флаги сборки.

## `const_cast` — только cv-qualifiers

Keyword **`const_cast`** добавляет или убирает **`const`** и **`volatile`**. Он **не** меняет тип «с int на double» и **не** лечит неверную иерархию.

Типичный легальный случай — legacy API, который принимает `char*`, а у тебя `const char*` или `std::string::data()` **(C++17)** с const строкой, которую API обещает не трогать:

```cpp
void legacy(char* s);

void forward(std::string& s) {
    legacy(const_cast<char*>(s.data()));
}
```

Если через `const_cast` ты **реально изменишь** объект, который изначально был const (строковый литерал, `const` переменная, `const&` на read-only данные), стандарт говорит об **undefined behavior**. Keyword не «даёт право» — он только снимает метку с типа.

`volatile` в embedded и signal handlers снимают тем же cast; в обычном прикладном коде `volatile` почти не трогают (глава 4).

## `reinterpret_cast` — низкий уровень и риск

Keyword **`reinterpret_cast`** трактует **битовое представление** адреса или целого как другой тип указателя/ссылки/целого. Это инструмент драйверов, interop с C, иногда hash от адреса.

```cpp
int n = 0;
std::uintptr_t u = reinterpret_cast<std::uintptr_t>(&n);
int* p = reinterpret_cast<int*>(u);
```

Много «хитрых» приёмов (чтение `float` через `int*`, приведение несвязанных указателей, обход alignment) попадают под **strict aliasing** и **alignment** — UB без платформенных гарантий. После экспериментов имеет смысл гонять **UBSan** (том 7).

Используй `reinterpret_cast`, когда понимаешь контракт платформы и нет безопасной альтернативы. Для «перечитать те же биты другим типом» у trivially copyable одинакового размера — **(C++20)**:

```cpp
#include <bit>
static_assert(sizeof(float) == sizeof(std::uint32_t));
float f = std::bit_cast<float>(std::uint32_t{0x3f800000u});
```

`bit_cast` — функция стандартной библиотеки, не keyword, но по смыслу часть той же главы учебника.

## **(C++20)** и **(C++23)** рядом с casts

**`std::bit_cast`** — определённое поведение для подходящих типов; не заменяет указатели и полиморфизм.

**`std::start_lifetime_as`** **(C++23)** — продвинутая тема **object lifetime** при placement и буферах; не keyword. Упоминаем, чтобы не путать с «любым» `reinterpret_cast` при сериализации struct в byte array: там отдельные правила layout, alignment и **(C++20)** `std::endian`.

Implicit conversions и **list-initialization** иногда делают то же, что ты хотел написать cast'ом; explicit constructors и **`explicit`** на конструкторе (глава 6) — соседняя линия обороны от случайных преобразований.

## Как выбирать cast за thirty секунд

Спроси себя: меняю ли я только **числовую сетку** или **иерархию без проверки**? → `static_cast`. Нужна ли **проверка подтипа** у полиморфной базы? → `dynamic_cast`. Только **const/volatile**? → `const_cast`. Нужна ли **переинтерпретация адреса/бит** на свой страх? → `reinterpret_cast`, иначе **(C++20)** `bit_cast`. Если ответ «не уверен» — возможно, проблема в дизайне API, а не в выборе keyword.

## Связь с другими главами

К **`const`** и **`volatile`** (глава 4) привязан единственный cast — `const_cast`. К **`virtual`** (глава 6) — условие работы `dynamic_cast`. Шаблоны (главы 9–10) иногда требуют `static_cast` в telereturn или при работе с зависимыми типами; не подменяй ими `dynamic_cast` «чтобы собралось».

## Типичные ошибки

Downcast через `static_cast`, когда runtime тип неизвестен — тихая мина: тесты с одним подтипом зелёные, прод с другим — падение. `dynamic_cast` на тип **без** virtual — ошибка компиляции; попытка обойти через `static_cast` не добавляет проверки. C-style cast к string literal через const_cast и запись в буфер — классический UB. `reinterpret_cast` между несвязанными указателями «для быстрой сериализации» без учёта alignment и strict aliasing. Путать `static_cast<void*>` с «можно забыть исходный тип» — обратно можно только в **тот же** тип. Ждать, что `bit_cast` работает с указателями — нет, только trivially copyable значения.

## Краткое резюме

Четыре keywords — `static_cast`, `dynamic_cast`, `const_cast`, `reinterpret_cast` — разделяют намерения явнее, чем `(T)e`. `static_cast` — числа, enum, up/down cast без runtime проверки, `void*` туда-обратно. `dynamic_cast` — безопасный downcast/cross-cast при полиморфной базе; указатель или исключение. `const_cast` — только cv; изменение truly const — UB. `reinterpret_cast` — последний resort; для битов значений смотри **(C++20)** `std::bit_cast`. C-style cast в новом коде избегай.

## Вопросы для самопроверки

1. Почему `dynamic_cast` требует хотя бы одного virtual в иерархии?
2. Что вернёт `dynamic_cast<Derived*>(b)`, если `b` указывает на объект другого подтипа?
3. Когда `std::bit_cast` предпочтительнее `reinterpret_cast`?
4. Почему C-style cast опаснее явного `static_cast` при чтении кода?
5. Можно ли безопасно сделать изменяемым string literal через `const_cast`?

## Обсуждение с репетитором

**Том:** 1 · **Глава:** `ch08` · **Файл:** `ch08-casts.md`

**Дальше по курсу:** [Глава 9. template, concept, requires](./ch09-template-keywords.md)
