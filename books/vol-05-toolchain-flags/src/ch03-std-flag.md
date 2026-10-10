# Глава 3. Стандарт языка

## Зачем эта тема

Без явного **`-std=c++17`** компилятор может остаться в режиме по умолчанию (на старых системах — GNU++14 или ниже). Тогда `std::optional`, structured bindings, `if constexpr` из тома 1 **не включены** или ведут себя иначе, чем в тексте книги. Репозиторий `training` **фиксирует C++17** в CMake — том 5 обязан показать **тот же контракт** в флагах командной строки и в IDE через `compile_commands.json`.

Стандарт языка — это не «галочка для pedantic». Это согласование **всех** `.cpp` в target, макроса **`__cplusplus`**, доступных overload в STL и правил ODR для inline в заголовках. Одна папка упражнения, собранная «как получится», а весь трек через CMake с 17 — классический источник «у меня в task работает, check.sh падает» (на самом деле наоборот, если task без `-std=`).

## Флаг -std=

Командная строка:

```bash
g++ -std=c++17 -Wall -c file.cpp
```

Распространённые значения: **`c++17`**, **`c++20`**, **`c++23`**, **`gnu++17`**. Префикс **`gnu`** включает **расширения GNU** поверх ISO. В репо и в серии для **нормативного** кода предпочитай **`c++17`** без `gnu`, если не нужно POSIX-расширение осознанно.

CMake в `multithread/CMakeLists.txt` дублирует смысл флага:

```cmake
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
```

**`CMAKE_CXX_STANDARD_REQUIRED ON`** — если 17 недоступен, configure **падает**, а не откатывается тихо на 14. **`CMAKE_CXX_EXTENSIONS OFF`** — не переключаться на `gnu++17` без запроса. Эти три строки стоит уметь процитировать так же, как `find_package(Threads)`.

## Макрос __cplusplus

В исходнике:

```cpp
#if __cplusplus >= 201703L
// ветка для C++17 и новее
#endif
```

Значение **`__cplusplus`** зависит от **`-std=`** и от компилятора. Исторически **MSVC** сообщал заниженное число до **`/Zc:__cplusplus`** — глава 14. Не путай «код компилируется» с «макрос говорит 201703».

Feature test macros из заголовка **`<version>`** **(C++20)** точнее для отдельных фич (`__cpp_coroutines`), чем одна цифра `__cplusplus`. На `-std=c++17` coroutine keywords в исходнике **не обязаны** собираться — пометка **(C++20)** в книге значит «подними стандарт и проверь GCC status».

## Пометки C++20 и C++23 в серии

Текст вроде «**(C++20)** `co_await`» означает: нужен **и** `-std=c++20` (или выше), **и** поддержка в **твоей** версии GCC/Clang. Флаг один не гарантирует modules, `import std`, полный `std::format` — у каждой фичи свой статус в таблице компилятора.

Для `multithread/` база — **17**. Эксперименты с jthread, latch, coroutines — локально другой build directory и явный `-std=c++20`, без правки сданных упражнений на 20 без запроса (см. `AGENTS.md`).

## Переход на C++20 локально

```bash
g++ -std=c++20 -pthread -Wall -Wextra demo.cpp -o demo
```

Coroutines требуют стандарт **и** флаги/библиотеки runtime. Если видишь «coroutines disabled» — это не «книга ошиблась», а граница toolchain.

Проверка макроса в маленьком файле:

```cpp
#include <iostream>
int main() {
    std::cout << __cplusplus << '\n';
}
```

С `-std=c++17` на GCC ожидай **201703** (не путать с годом учебника).

## -std на compile и согласованность TU

**`-std=`** влияет на **каждый** `.cpp` в executable. Теоретически можно смешать `.o` от разных стандартов, если ABI совместим, но **inline** и шаблоны в общих заголовках должны видеть **один** набор правил. Проще и правильнее: **один стандарт на target** — как в CMake репо.

Если один `.cpp` собран как C++14, а заголовок использует `if constexpr` — ошибка в этом TU, даже если остальной проект на 17.

## gnu++17 vs c++17 на практике

**`gnu++17`** может включить поведение, которого нет в ISO: некоторые builtins, расширения в системных заголовках. **`-Wpedantic`** (глава 4) помогает ловить отклонения, когда ты целишься в переносимый **`c++17`**.

На Fedora для учебного кода репозитория — **`c++17`** + **`CMAKE_CXX_EXTENSIONS OFF`**.

## Связь с томом 1

Keywords **`concept`**, **`co_await`**, **`import`** относятся к другим стандартам. Том 1 предупреждал: смотри флаг. Том 5 даёт **механизм**: CMake, `g++`, запись в `compile_commands.json` для clangd.

Structured bindings и `std::optional` — **C++17**; без `-std=c++17` ты можешь случайно остаться в мире, где их нет, и решить, что «STL сломана».

## Сценарий: «optional не найден»

Симптом: `#include <optional>` есть, компилятор пишет, что optional не member of std. Частые причины:

- сборка как **C++14** по умолчанию;
- опечатка в `-std` (`c++1z` устарело — используй **`c++17`**);
- clangd смотрит другой стандарт без json (глава 11).

Fix: выровнять CMake, ручную строку и IDE.

## К собесу

Процитировать три строки CMake для стандарта из репо. Объяснить разницу **`c++17`** и **`gnu++17`**. Сказать, зачем **`STANDARD_REQUIRED ON`**. Не утверждать, что `-std=c++20` автоматически включает все фичи C++20.

## Типичные заблуждения

«На Fedora по умолчанию всегда C++17» — **проверь** `g++ -dM -E -x c++ /dev/null | grep __cplusplus` без `-std`. «`gnu++17` = `c++17`» — extensions могут менять код. «Можно `#define __cplusplus`» — не подделываешь язык, только себя. «CMake 17 сам по себе без STANDARD» — может не выставить то, что ты думаешь, смотри cache.


## CMake cache и стандарт

После configure посмотри:

```bash
grep CXX_STANDARD multithread/build/CMakeCache.txt
```

Должно быть **`CMAKE_CXX_STANDARD:STRING=17`**. Если вручную менял **`CMakeLists.txt`** с 17 на 20 — без re-configure cache может врать в старых log, пока не перегенерируешь.

**`CMAKE_CXX_STANDARD`** — property у **targets** в новых стилях; в репо глобальный **`set`** до **`add_executable`** — OK для маленького проекта. В больших кодовых базах **`target_compile_features(06_thread_pool PRIVATE cxx_std_17)`** — альтернатива; для **`multithread/CMakeLists.txt`** достаточно трёх **`set`**, уже стоящих в файле.

## -std в compile_commands и clangd

Если clangd показывает C++14 syntax errors на structured bindings — открой json и найди **`task.cpp`**: в **`arguments`** должен быть **`-std=c++17`**. Если там нет — configure без **`CMAKE_CXX_STANDARD`** или json от другого проекта.

## Эксперимент: gnu extensions

Создай **`snippet.cpp`** с valid **`c++17`** и собери дважды:

```bash
g++ -std=c++17 -pedantic -c snippet.cpp
g++ -std=gnu++17 -pedantic -c snippet.cpp
```

На некоторых версиях второй вариант **молчит**, где первый **ругается** на extension — редко в STL-only коде, но объясняет **`CMAKE_CXX_EXTENSIONS OFF`**.

## Связь с упражнениями 03 и 05

**`03_oracle`** и **`async`** — C++11+, но репо на **17**. **`05_highscore`** с **`std::atomic`** — не путать с **`volatile`**. Стандарт задаёт **memory model** atomics (том 3); **`-std=c++17`** не заменяет правильные **`memory_order`**, но включает нужные заголовки и правила языка.


## -std и precompiled headers (PCH)

PCH ускоряют compile больших проектов; **`multithread`** маленький — PCH не нужен. Если увидишь **`/Fp`** (MSVC) или **`‑include pch.h`** — это compile-time speed, не стандарт языка.

## Upgrade path репозитория на C++20

Гипотетически: сменить **`CMAKE_CXX_STANDARD`** на 20, проверить **`jthread`** exercises — **не делай** без запроса. Том 5 фиксирует **17** как контракт **`AGENTS.md`**.

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

Три строки CMake для C++17 из репо. Разница `gnu++17` и `c++17`. Что проверяет `__cplusplus`.

## Согласованность: CMake, g++, clangd

Три потребителя должны видеть **один** стандарт:

1. **`multithread/CMakeLists.txt`** — **`CMAKE_CXX_STANDARD 17`**.
2. Ручная строка **`g++ -std=c++17`** — глава 13.
3. Запись в **`compile_commands.json`** для **`task.cpp`** — глава 11.

Если любой из трёх уехал в **14** или **gnu++17** без **`EXTENSIONS OFF`**, ты тратишь время на phantom errors в IDE или на «почему structured binding не работает». Перед сессией с репетитором по toolchain — 30 секунд: grep **`__cplusplus`** в маленьком snippet или grep **`-std`** в json.

## Краткое резюме

Репо: **C++17**, `-std=c++17`, CMake **`CXX_STANDARD 17`**, extensions off. Для 20/23 — явный флаг + версия компилятора + status page. **`__cplusplus`** — для `#if`, не для споров вслух. Дальше — предупреждения **`-Wall`** как в `CMakeLists.txt`.

## Вопросы для самопроверки

1. Какие три строки в `multithread/CMakeLists.txt` фиксируют стандарт?
2. Чем **`gnu++17`** может отличаться от **`c++17`** в поведении программы?
3. Зачем **`CMAKE_CXX_STANDARD_REQUIRED ON`**?
4. Соберётся ли **`co_await`** только с **`-std=c++17`** на типичном GCC?
5. Как в коде проверить, что активен режим не ниже C++17?

## Обсуждение с репетитором

**Том:** 5 · **Глава:** `ch03` · **Файл:** `ch03-std-flag.md`

**Дальше по курсу:** [Глава 4. Предупреждения](./ch04-warnings.md)
