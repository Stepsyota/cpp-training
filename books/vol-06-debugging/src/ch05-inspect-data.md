# Глава 5. Смотреть данные

## Зачем эта тема

Breakpoint без **`print`** — ты знаешь «где», но не «что». GDB умеет показывать locals, поля struct, элементы vector (с оговорками), формат hex/dec.

Идеальный **`print`** на диагностике — read-only: **`size`**, флаги **`stopped`**, **`closed`**, без вызова **`pop`**. В C++ чтение shared без mutex формально может быть data race; под GDB на oстановке ты делаешь эвристику «что видим», а fix проверяешь тестами и sanitizer (tom 7).

## print

```
(gdb) print counter
(gdb) p counter
(gdb) print/x ptr
```

**`/x`** hex, **`/d`** decimal, **`/t`** binary. **`print *ptr`** — разыменование (если ptr валиден).

```
(gdb) print job_queue.size()
```

Вызов метода в print работает для простых случаев, если компилятор сгенерировал debug info для inline — на `-O0` обычно OK.

## Автодисплей

```
(gdb) display counter
```

После каждой остановки GDB покажет значение. **`undisplay 1`** убрать.

## info locals / info args

```
(gdb) info locals
(gdb) info args
```

Быстрый снимок стека текущего frame.

## Типы

```
(gdb) whatis counter
(gdb) ptype ThreadPool
```

Понять, pointer это или value, размер struct.

## Массивы и vector

```
(gdb) print arr[0]@10
```

Десять элементов от `arr[0]`. Для `std::vector` без pretty-printer:

```
(gdb) print v._M_impl._M_start[0]@n
```

Имена полей libstdc++ **могут меняться** между версиями — хрупко; глава 12.

## Строки

```
(gdb) print s.c_str()
(gdb) print (char*)s.data()
```

Для `std::string` с debug symbols иногда достаточно `print s`.

## const и оптимизация

На Release **`print var`** может сказать **optimized out** — переменной «нет» в runtime representation. Debug build — первый выбор.

## Типичные заблуждения

«print вызывает функцию без побочных эффектов» — может, если вызвал метод с lock. «locals всегда актуальны в threads» — другой поток меняет shared state.

## Краткое резюме

`print`, `display`, `info locals`, `ptype`. Форматы `/x`. STL — осторожно или pretty-printers. Дальше — backtrace.

## Форматы print подробнее

**`print/x`** hex для указателей; **`/t`** binary для флагов; **`/c`** char. **`print (int)ch`** — cast в выражении. **`print sizeof(T)`** — иногда через **`ptype`**.

## display и undisplay

Несколько **`display`** для **`busy`**, **`stopped`**, **`jobs.size()`** (если выражение валидно). Нумерация **`undisplay`**. **`info display`**.

## print и side effects

**`print q.pop()`** **вызовет** pop — испортит состояние очереди. Безопаснее **`print q.size()`** или поля под mutex **осознанно** (понимая, что другой thread concurrent). Правило: **не вызывай** mutating methods в print «для интереса».

## Доступ к полям через this

В методе класса **`print this->stopped`** работает на breakpoint внутри member function. Для **`[this]`** lambda — frame может быть operator(); смотри captured fields через compiler-specific names или break в теле lambda.

## Массивы и указатели

**`print *ptr@n`** — n элементов. Если ptr invalid — segfault **под** GDB, получишь stop. **`x/10wx addr`** — examine memory (низкий уровень).

## std::string и optional

С pretty (глава 12) **`print s`** читаемо. **`std::optional`** **(C++17)** — зависит от printers; иначе **`print opt.has_value()`**.

## atomic и highscore

**`05_highscore`**: **`print record.load()`** — если видишь atomic API в debug info. CAS loop — breakpoint на compare failure, **`print expected`**.

## frame и locals вместе

Сначала **`bt`**, выбери frame **`frame 3`**, **`info locals`**. Так находишь аргументы **`submit`** в caller.

## § К собесу

«Можно ли в GDB вызвать функцию?» — **`call`** / **`print func()`** да, с осторожностью к side effects и threads.

## set print elements

**`set print elements 0`** — unlimited array elements in print; осторожно на huge vector.

## set print object

**`set print object on`** — derived type for pointers to base — OOP tom 8 задел.

## examine queue state under lock

Если break **внутри** `lock_guard` scope, print queue fields **без** доп lock. Break **вне** — print может race read — только для heuristic.

## typed watch

**`watch -location x`** — GDB 10+ syntax variants; classic **`watch x`**.

## compare two runs

**display`** counter at same breakpoint run1 vs run2 — flaky values hint race.

## floating point print

**`print/x`** for bit pattern; NaN debugging — UBSan sometimes first.

## output formats summary

Hex pointers, decimal counters, char for **`char`**. **`print (void*)p`** casts.

## static locals

**`info locals`** includes static with persistence — surprise values between **`run`**.

## global mutable

**`print g_counter`** on break anywhere if symbol global — all threads see; interpret carefully.

## struct padding

**`ptype struct`** shows layout; alignment surprises rare in course.

## Практическое закрепление (5–10 минут)

Открой Debug-сборку любого **зелёного** target из `multithread/build/`. Запусти GDB, поставь **`break main`**, **`run`**, выполни **`next`** пять раз, **`bt`**, **`info locals`**, **`quit`**. Повтори на другом target — muscle memory важнее заучивания глав. Если что-то не совпало с текстом главы, проверь cwd и путь к exe: nine из десяти «GDB сломан» — неверный путь или сборка без **`-g`**.

## Связь с главой тома 3 без спойлера кода

Вернись мысленно к инварианту упражнения, которое отлаживаешь: что должно быть истинно перед выходом из **`wait`**, когда очередь считается пустой, когда **`submit`** должен молчать. GDB показывает **момент**, когда инвариант уже нарушен или ещё не достигнут. Исправление остаётся в голове и в **`task.cpp`**, не в команде **`continue`**.

## § Коротко к собесу (повтор)

Сформулируй вслух одним предложением главную мысль этой главы и назови **одну** команду GDB, без которой тема не работает. Если не можешь — перечитай «Краткое резюме» и вернись сюда завтра; spaced repetition лучше зубрёжки за вечер.

## Вопросы для самопроверки

1. Как каждый stop видеть значение `busy`?
2. Что покажет `info locals` на breakpoint в `main`?
3. Почему `print v[0]` для vector может не сработать?
4. Что значит optimized out?
5. Безопасно ли `print` вызывать `pop()`?

## Обсуждение с репетитором

**Том:** 6 · **Глава:** `ch05` · **Файл:** `ch05-inspect-data.md`

**Дальше по курсу:** [Глава 6. Стек вызовов](./ch06-backtrace.md)
