# Глава 5. Atomics: load, store, RMW, compare_exchange

## Зачем

**Mutex** сериализует **любой** код в секции — даже чтение счётчика блокирует других. Для **одного** слова памяти и простых операций (**increment**, **max**, флаг «готово») CPU умеет **atomic instructions** — без захвата OS mutex. **`std::atomic<T>`** в C++ даёт синтаксис и **memory ordering** (default **`memory_order_seq_cst`**, детали в главе 6).

Atomics **не заменяют** mutex для сложных инвариантов: «`head`/`tail`/`vector` согласованы» одним **`atomic`** не закрыть. Эта глава — когда atomic уместен, **`load`/`store`**, **`fetch_add`**, **`compare_exchange`** (CAS), и почему «весь пул на atomics» — ложный путь.

## Объект `std::atomic`

```cpp
#include <atomic>

std::atomic<int> counter{0};

void bump() {
    counter.fetch_add(1, std::memory_order_relaxed);
}
```

**`fetch_add`** — **read-modify-write (RMW)** атомарно: load + add + store как одна операция с точки зрения data race. Для **`counter++`** на **`atomic`** используй **`fetch_add(1)`** или **`++counter`** (перегрузки для atomics).

**`T`** должно быть **trivially copyable** и подходящего размера; **`std::atomic<std::string>`** — нет. Для флага часто **`std::atomic<bool>`**; для указателя публикации — **`std::atomic<Widget*>`** с осторожностью lifetime.

**`a.is_lock_free()`** — спросить реализацию, гарантирует ли atomic операции без внутреннего mutex (для **`atomic<double>`** на старых платформах иногда нет). На x86 **`int`** и указатели обычно lock-free.

**`std::atomic_flag`** — минимальный atomic; **`test_and_set`**, **`clear`**. Используют для spinlock в низкоуровневом коде; для прикладного C++ чаще **`mutex`**.

## Load и store

**`counter.load()`** и **`counter.store(42)`** — явные операции. **`counter = 42`** для atomic — store. Concurrent **`load`** без **`store`** с другой стороны — не data race на atomic объект; стандарт определяет **data races on atomics** отдельно — используй API atomics, не **`reinterpret_cast`** на байты.

По умолчанию ordering **`seq_cst`** — самый строгий, дороже на некоторых архитектурах; для чистого счётчика статистики иногда **`relaxed`** (глава 6), для publish-handoff — **release/acquire**.

## `compare_exchange` — CAS

**Compare-and-swap**: «если значение всё ещё **expected**, запиши **desired**; иначе обнови **expected** текущим». Основа lock-free алгоритмов и паттерна «обновить max только если больше»:

```cpp
std::atomic<int> best{0};

void try_record(int score) {
    int cur = best.load(std::memory_order_relaxed);
    while (score > cur &&
           !best.compare_exchange_weak(cur, score,
               std::memory_order_relaxed)) {
        // cur обновлён конкурентом, повтор
    }
}
```

**`compare_exchange_weak`** может **ложно провалиться** на spurious — в цикле норма. **`compare_exchange_strong`** — для одной попытки, когда повтор не нужен.

CAS **не** гарантирует, что «мы победили навсегда» без версии объекта: **ABA problem** (глава 8) — когда значение вернулось к старому, но структура другая. Для учебного **max score** на `int` достаточно.

В **`compare_exchange`**, **`expected`** — **in-out**: при неудаче туда пишется актуальное значение. Типичный баг — завести **`int expected = score`** вне цикла и не обновлять после fail, бесконечно биться о устаревшее.

```cpp
int expected = best.load();
while (!best.compare_exchange_weak(expected, score)) {
    if (score <= expected) break; // уже есть лучший
}
```

## Atomic vs mutex для счётчика

| Сценарий | Подход |
|----------|--------|
| Только `++` на `int` | `atomic` или mutex — оба OK |
| Несколько полей + инвариант | mutex |
| `vector`, queue | mutex или специализированные структуры |
| «Проверить и если пусто — взять» | mutex + cv, не один atomic |

**Mutex на каждый `++`** работает, но дороже при contention. **Atomic без смысла** на большой critical path не ускорит.

## Не замена mutex

Ошибка: « **`std::atomic<bool> busy`** вместо mutex в очереди задач » — **`push`/`pop`** трогают **`std::deque`**; нужен один mutex (или lock-free queue — другой уровень, глава 8). Atomic **`busy`** **дополняет** mutex для **`wait_idle`**: «все воркеры простаивают» — отдельный счётчик под mutex или atomic с аккуратным протоколом.

**`volatile std::atomic`** — бессмыслица для синхронизации; **`volatile`** не про threads.

## Free functions

**`std::atomic_load(&a)`**, **`std::atomic_fetch_add(&a, 1)`** — для C interop и generic кода. В C++ коде чаще методы объекта.

## Инициализация и padding

**`std::atomic<int> x{0};`** — value-initialization. **False sharing** (глава 9): два atomic в одной cache line — разные ядра инвалидируют кэш; иногда **`alignas(64)`** между hot atomics.

## Публикация указателя (идея)

Один поток строит **`Widget`** в куче, другой читает через **`atomic<Widget*>`**: store **release** после инициализации полей, load **acquire** перед dereference — формально «увидел указатель → увидел содержимое». Без ordering или mutex — тот же класс багов, что **`ready`/`data`**. Детали ordering — глава 6; здесь запомни: atomic на указателе **не** освобождает от правил lifetime **`delete`**.

## Fedora / GCC / Clang

Atomics в libstdc++/libc++ на x86_64 используют **`LOCK`** инструкции или compare-exchange. С **` -std=c++17`** доступен полный набор для fundamentals. Проверка гонок на «полу-atomic» (**`int` без atomic**) — TSan, не «работает на Ryzen».

## Упражнение highscore и CAS без спойлера

`05_highscore` просит обновлять **максимум** без mutex, через atomic и compare-exchange. Идея книги: **read-modify-write** на plain `int` — гонка; на `std::atomic` — defined behavior при использовании API atomics. Цикл «прочитал cur, если score больше — CAS, иначе выход» — стандартный паттерн; **ordering** по умолчанию `seq_cst` для учебной задачи достаточен (ослабление — только с пониманием главы 6).

Не путай «рекord один `int`» с «атомарно обновить **структуру** лидерборда»: второе требует mutex или сложного lock-free протокола.

## `atomic` и типы: что можно, что нельзя

`std::atomic<T>` требует **trivially copyable** `T` подходящего размера; пользовательские struct с padding и указателями — зона экспертов. `atomic<bool>` может быть lock-free или нет — проверяй `is_lock_free()` в эксперименте, не в hot path production без нужды.

`std::atomic_ref` **(C++20)** позволяет атомарно обращаться к **существующему** объекту — за пределами C++17 базы репо; упоминаем для чтения чужого кода 20+.

## Ordering «по умолчанию» и когда лезть в главу 6

Пока ты не читаешь главу 6 осознанно, **не** ослабляй order «для скорости». `fetch_add(1)` без второго аргумента — seq_cst; для счётчика посещений без публикации связанных данных иногда достаточно `relaxed`, но highscore и handoff флагов — другой сценарий.

`atomic` **не** делает safe concurrent доступ к **другим** полям класса: `atomic head` + plain `Node* next` без протокола — всё ещё гонка на `next`.

## § К собесу

- Чем **RMW** отличается от пары load/store с mutex?
- Зачем **compare_exchange_weak** в цикле, а не один strong?
- Почему **ABA** не проблема для `int` max, но проблема для lock-free stack?
- Можно ли заменить mutex очереди одним `atomic<bool>` busy?

## Связь с упражнениями курса

**Highscore / max без mutex** — типичный CAS loop: два потокa пишут рекord, побеждает больший, без lost update на **`int`**. Это **не** спойлер реализации: идея «читай, сравни, CAS, повтор» — стандартный паттерн. Пул и очередь остаются на **mutex + cv** — там инвариант шире одного слова.

## Типичные ошибки

Думать, что **`atomic`** на поле A делает безопасным concurrent доступ к соседнему полю B без синхронизации. CAS **без цикла** при конкуренции — lost updates. **`compare_exchange`** с неправильным **`expected`** (копия не обновляется в цикле). Заменить mutex на atomic «для скорости» в **`std::map`**. Использовать **обычный `int`** «потому что x86 сильный» — UB по стандарту, TSan ругается. Путать **atomic** с **thread-safe** для всего класса.


## Глубже: размер, alignment и `is_lock_free`

На некоторых платформах `atomic<double>` или большие struct реализованы **внутренним mutex** — `is_lock_free()` false. Для hot path это сюрприз. Проверяй один раз при старте или в unit-тесте окружения Fedora, не в каждой операции.

Bit-fields и atomics — отдельная боль; в учебном коде не смешивай.

## Счётчик ссылок vs data race

`std::shared_ptr` использует atomic control block — это **не** делает **ваш** объект thread-safe. Два `shared_ptr` на один объект и concurrent mutation полей — по-прежнему ваш mutex.


## Мост к memory order

Default seq_cst — ваш друг до первого профиля. Глава 6 объясняет, когда **release/acquire** описывает handoff, а **relaxed** — только счётчик без связанных данных. Highscore в курсе — CAS; ordering по умолчанию достаточен для learning.

## Memory order и highscore (без спойлера реализации)

Для одного atomic max CAS с default order **корректен** для learning: seq_cst на success/failure прощает новичку отсутствие release/acquire на соседних полях — их нет. Если добавишь «рекord + timestamp» в двух полях — снова mutex или протокол release на паре.

## Практика на бумаге

Словами CAS-цикл max: что в `expected` после failed CAS?

Почему atomic на head очереди не заменяет mutex на deque?

## Краткое резюме

**`std::atomic`** — без data race на самом atomic при использовании API; RMW **`fetch_add`**, **`compare_exchange`** для lock-free паттернов на одном слове. **Ordering** по умолчанию **`seq_cst`**; ослабление — глава 6. **Mutex** нужен для составных структур и протоколов очереди. CAS в цикле — max, стеки; не панацея от ABA и сложных инвариантов. Дальше — **memory orders** и happens-before формально.

## Вопросы для самопроверки

1. Почему `++` на обычном `int` из двух потоков — UB, а `fetch_add` на `atomic<int>` — нет?
2. Зачем в CAS-цикле обновлять локальную копию `expected` после неудачи?
3. Можно ли защитить `push_back` в `vector` одним `atomic<size_t>`?
4. Чем `compare_exchange_weak` отличается от `strong` в типичном retry-loop?
5. Заменяет ли `atomic<bool> ready` пару mutex + cv для bounded queue?

## Обсуждение с репетитором

**Том:** 3 · **Глава:** `ch05` · **Файл:** `ch05-atomics.md`

**Дальше по курсу:** [Глава 6. Memory order](./ch06-memory-orders.md)
