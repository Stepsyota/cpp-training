# Глава 8. Раздельная компиляция

## Зачем эта тема

Заголовок **`thread_pool.hpp`** с объявлением класса и два **`.cpp`** с методами — три файла в дереве, но **каждый `.cpp`** компилируется **отдельно** в свой **`.o`**. Ошибка **duplicate symbol** или **`undefined reference to ThreadPool::submit`** — про **ODR** (One Definition Rule) и про то, что определение метода забыли в `.cpp` или не добавили `.cpp` в CMake target.

Без модели **translation unit (TU)** CMake выглядит магией: «почему `smithy.cpp` видит order.hpp» — потому что **`target_include_directories`**, а не потому что «все файлы одна программа на compile».

## Translation unit

**TU** — один **`.cpp`** после препроцессора (все `#include` развернуты). Компилятор **не** видит тела функций в другом `.cpp`, только **объявления** из заголовков. Два `.cpp` **не** делят локальные **`static`** функции и **`static`** переменные на уровне файла — unless exported.

Сценарий: в `a.cpp` `static void helper(){}` — в `b.cpp` такого символа нет. Если helper нужен везде — заголовок inline или отдельный `.cpp` с non-static и объявлением в `.hpp`.

## Объявление vs определение

В заголовке:

```cpp
class ThreadPool {
public:
    void submit(/*...*/);
};
```

В **`thread_pool.cpp`**:

```cpp
void ThreadPool::submit(/*...*/) { /*...*/ }
```

Если метод **определён внутри класса в заголовке** — он **inline** по умолчанию; каждый TU, include’ящий заголовок, может получить копию. Для мелких **`size()`** OK; для тяжёлой логики кузницы — **вынеси в `.cpp`**.

## One Definition Rule (ODR)

Глобальная функция или **non-inline** переменная — **ровно одно** определение в программе. Несколько `.cpp` с **`int counter = 0;`** без **`static`/`inline`** → **multiple definition** на link.

**`inline`** на функциях и **`inline` variables (C++17)** в заголовках — ODR-safe sharing. **`static`** в `.cpp` — **internal linkage**, символ не виден другим TU.

Шаблоны: определения обычно **в заголовке**; компилятор инстанцирует в каждом TU, где использовали — **`bounded_queue.hpp`** vs **`thread_pool.cpp`**.

## Паттерны репозитория advanced

**`06_thread_pool`**: **`thread_pool.hpp`** — интерфейс; **`thread_pool.cpp`** — реализация; **`smithy.cpp`** — сценарий; **`test.cpp`** — main и проверки. CMake:

```cmake
add_executable(06_thread_pool
    advanced/01_thread_pool/thread_pool.cpp
    advanced/01_thread_pool/smithy.cpp
    advanced/01_thread_pool/test.cpp
)
```

**`07_pipeline`**: header-only **`bounded_queue.hpp`** — шаблон целиком в заголовке; **`factory.cpp`** + **`test.cpp`**.

**`08_parallel_map`**: логика в **`parallel_map.hpp`**, один **`test.cpp`** — инстанцирование шаблонов в одном TU.

## include guards и #pragma once

```cpp
#ifndef THREAD_POOL_HPP
#define THREAD_POOL_HPP
// ...
#endif
```

**`#pragma once`** — проще, не 100% стандарт, но везде работает. Цель — **один** include заголовка **в одном TU**, не multiply class definitions **в одном** `.cpp`. ODR между TU — другая история.

## undefined reference на метод класса

Компилятор в **`test.o`** видит объявление **`submit`**. Линковщик ищет **`ThreadPool::submit`** в **`thread_pool.o`**. Нет реализации или не линкуешь **`thread_pool.o`** → **undefined reference**. Синтаксис в test «правильный» — лечится **списком исходников** (глава 10).

## Шаблон только в .cpp

Классическая ошибка: шаблон **`template<typename T> void foo(T)`** определён только в `.cpp` без **explicit instantiation** — link error **`undefined reference to void foo<int>(int)`**. В курсе **`parallel_map`** держит шаблон в **`.hpp`**.

## Связь с главой 1

**`-c`** на каждый `.cpp` → `.o` → link. Ошибка фазы — см. шпаргалку в ch01.

## К собесу

Что такое TU. Почему метод в заголовке раздувает binary. **`static`** в `.cpp` и ODR. Откуда **undefined reference** на метод.

## Типичные заблуждения

«Скомпилировал `.hpp`» — компилируют **`.cpp`**. «Два include = две копии класса в программе» — в **одном** TU include once; ODR про **разные** TU и inline. «ODR только для globals» — и для inline, templates, types.


## Карта файлов advanced (без спойлера решений)

**`06`**: hpp + cpp pool, cpp smithy, cpp test — **три TU**. **`07`**: hpp queue, cpp factory, cpp test. **`08`**: hpp map, cpp test. Сопоставь с **`add_executable`** в **`CMakeLists.txt`** — каждый **`*.cpp`** в списке = один **`.o`**.

## Explicit template instantiation (край)

Если бы **`parallel_map`** определили только в **`.cpp`**, понадобилось бы в конце **`template ...`** для используемых **`T`**. Репо избегает этого pattern в **`08`** — header-only шаблон проще для учебника.

## static member ODR

**`static`** data member класса — одно определение в **`.cpp`** (C++17 inline static иной случай). Duplicate on link — частая ошибка при копипасте **`int Foo::count = 0;`** в два **`.cpp`**.


## anonymous namespace vs static

**`namespace { void f(); }`** — internal linkage like **`static`** in C++ — prefer modern style in new code.

## weak symbols (awareness)

**`__attribute__((weak))`** — rare; link order tricks. Not in **`multithread`**.

## Углубление: связь с пройденным курсом multithread

Ты уже реализовал **`01_rocket_launch`** — там важно было **`join`**, не pipeline. С точки зрения toolchain это один **`task.cpp`**, один **`test.cpp`**, один link с **`-pthread`**. **`02_ticket_office`** — тот же каркас; mutex не меняет флаги, но если забудешь **`-std=c++17`**, **`std::mutex`** всё равно есть (C++11), зато другие привычки (bindings, optional в других файлах) поплывут.

**`03_oracle`** — **`async`**; link как у thread. **`04_elevator`** — первый раз, когда Debug с **`-g`** реально нужен для lost wakeup: без символов **`wait`** в gdb — адрес. **`05_highscore`** — atomics; Release **`-O2`** не заменяет CAS, но UB без atomics проявится иначе на Release.

**`06_thread_pool`** — первый target с **тремя** `.cpp` в **`CMakeLists.txt`**. Ошибка «не тот список файлов» — глава 8 и 10. **`07_pipeline`** — два `.cpp` и header-only queue. **`08_parallel_map`** — один test TU, шаблон в hpp; **`Threads`** всё равно на link, потому что **`parallel_map`** создаёт **`std::thread`**.

Когда перечитываешь том 5 после сдачи advanced, отметь галочкой: «эта глава объясняет мой симptom N». Так книга становится справочником, не только линейным чтением.

## Углубление: чтение вывода cmake --build

Первые строки compile — **`Building CXX object`**, имя **`.o`**, путь к **`.cpp`**. Link — **`Linking CXX executable`**, имя **`06_thread_pool`**. Если compile прошёл, а link упал — scroll к **link** stanza, не к первому warning compile.

Warning в **`test.cpp`**, который «не трогаешь» — всё равно **твой** build; CI не разделит «виноват test». Репетитор может сказать «игнорируй test», но toolchain всё равно показывает warning — решай, нужен ли **`‑Wno`** или fix в **`task`**.

## Углубление: дисциплина build directory

Один **`build/`** на ветку git — норма. Два clone репо — два **`build/`**. Не копируй **`build/`** между машинами. **`compile_commands.json`** привязан к absolute paths на машине — на другом ноутбуке regenerate.

## Мини-кейс для самостоятельной проверки toolchain

Возьми **`multithread/beginner/02_ticket_office/`** (или любую сданную задачу). Собери через CMake target **`02_ticket_office`**. Затем собери одной строкой **`g++`** из главы 13 с **`-Wextra -Wpedantic`**. Оба run должны быть зелёными. Если расходятся — выпиши diff флагов из **`compile_commands.json`** и ручной команды. Повтори для **`06_thread_pool`** только через CMake (ручная строка длиннее). Запиши один вывод **`ldd`** на любой binary. Это занимает 15 минут и закрепляет том лучше, чем перечитывание без terminal.

## К собесу (дополнение)

TU, ODR, declaration vs definition. Почему три `.cpp` у `06_thread_pool`.

## Краткое резюме

**TU = `.cpp` + includes**. Объявления в **`.hpp`**, тяжёлые методы часто в **`.cpp`**. **ODR** — одно определение. Link errors — missing **`.cpp`** в target. Дальше — **`.a`/`.so`**.

## Вопросы для самопроверки

1. Что такое translation unit?
2. Почему inline method в header может раздуть exe?
3. Чем **`static`** переменная в `.cpp` помогает с ODR?
4. Откуда **`undefined reference to ThreadPool::...`**?
5. Зачем include guard?

## Обсуждение с репетитором

**Том:** 5 · **Глава:** `ch08` · **Файл:** `ch08-separate-compile.md`

**Дальше по курсу:** [Глава 9. Статика и динамика](./ch09-static-dynamic.md)
