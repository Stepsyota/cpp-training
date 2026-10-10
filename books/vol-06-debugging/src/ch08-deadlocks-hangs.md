# Глава 8. Зависания и deadlock

## Зачем эта тема

Тест «висит» без вывода — не segfault. GDB на Ctrl+C + **`thread apply all bt`** часто показывает **цикл ожидания**: mutex, condition_variable, `future.get()` навсегда. Эта глава связывает картинку стека с ошибками из тома 3–4, без спойлера эталонного кода.

## Hang — это не «тип ошибки компилятора»

Зависание в CI или в **`check.sh`** раздражает тем, что CPU может быть занят на сто процентов или, наоборот, почти idle — оба варианта не дают строки «Segmentation fault». Пользователь ждёт завершения, а процесс живёт в kernel wait или крутит spin. GDB не ускоряет диагноз, но **останавливает** иллюзию «ничего не происходит»: после Ctrl+C ты видишь **конкретные** функции ожидания. Это уже половина пути к fix.

Запиши привычку: как только тест «висит» дольше ожидаемого, **не** перезапускай вслепую десять раз — один раз снимок стеков. Повторные run без анализа иногда «магически» проходят из‑за timing — ты так и не учишься.

## Mutex deadlock

Два потока держат разные mutex и ждут друг друга — классика. Один mutex, **двойной lock** в одном потоке без recursive mutex — второй `lock` в том же потоке зависнет. В bt: `__pthread_mutex_lock` → твой `lock_guard`.

Упражнение кузница: **join воркеров под тем же mutex**, что ждёт воркер в `wait_idle` — воркер не может уменьшить `busy`, main не отпускает lock. Стеки покажут оба в `pthread_mutex_lock`.

## Таймауты и «висит вечно»

Учебные тесты редко ставят **`wait_for`** с таймаутом — намеренно, чтобы баг hang был заметен. В production сервис добавил бы timeout и метрику; в курсе GDB **заменяет** timeout диагностикой: ты видишь, кто не проснулся. Если когда‑нибудь добавишь timeout в свою копию для эксперiment, hang превратится в «ложный fail» — не путай с успешным fix.

## condition_variable без notify

Поток в `__condvar_wait`. Другие потоки не в wait — проверь, был ли `notify` под правильным mutex, был ли предикат ложен из-за lost wakeup (том 3). GDB не заменяет разбор предиката, но показывает **кто спит**.

## future.get навсегда

Main в `future::wait` / `get`, worker не вызвал `set_value` — broken promise или поток умер. bt main — в libstdc++ future, worker — в compute или уже завершился. Связь с томом 4.

## spin без прогресса

Бесконечный цикл без yield — CPU 100%, bt повторяется. Отличай от block на mutex: в spin нет wait в libc.

## Зависший тест check.sh

Запусти один exe под gdb, дождись зависания, Ctrl+C:

```
(gdb) thread apply all bt
```

Запиши: сколько потоков, где wait. Сопоставь с полями пула/очереди через `print` в frame воркера (имена полей — твои в task, не из solutions).

## Документирование hang для себя

После **`thread apply all bt`** выпиши в блокнот: число потоков, две строки «main делает …», «worker N делает …». Через неделю без записи забудешь, был ли это **`lock`** или **`wait`**. **`THEORY.md`** в репозитории — правильное место для таких выводов (см. **AGENTS.md**).

## Когда GDB бессилен

Гонка без deadlock: программа **завершается** с неверным результатом — GDB не остановится. Нужен TSan или логика. «Иногда висит» — может быть livelock или starvation; реже в учебнике.

## Типичные заблуждения

«Висит = segfault» — нет. «all bt всегда явный deadlock» — иногда один поток ждёт ввода. «notify под mutex ломает gdb» — нет.

## Краткое резюме

Ctrl+C → `thread apply all bt`. Ищи mutex lock, cond wait, future wait. Сопоставь с join под mutex и cv предикатом. Гонки — том 7. Дальше — core.

## Алгоритм первых пяти минут при hang

1. Не убивай процесс сразу — открой GDB или уже в GDB: **Ctrl+C**.
2. **`thread apply all bt`** — сохрани вывод.
3. Отметь потоки в **`__lll_lock_wait`**, **`pthread_cond_wait`**, **`futex`**, **`future::wait`**.
4. Переключись на main thread **`thread 1`** (не доверяй номеру — смотри bt на `main`).
5. **`print`** ключевые флаги **в frame твоего класса**, не в libc.

## double lock и recursive_mutex

**`std::mutex`** не reentrant. Второй **`lock`** в том же потоке → hang. bt покажет **тот же** thread дважды в lock path. **`std::recursive_mutex`** в курсе обычно не нужен — fix логики, не тип mutex.

## join под mutex — картина в bt

Main: **`pthread_mutex_lock`** → твой **`wait_idle`** / destructor. Worker: **`lock`** → **`condition_variable::wait`**. Worker не может **`--busy`**, main не отпускает mutex — классика из THEORY.md. GDB **подтверждает** гипотезу; fix — дизайн (join без того же mutex), не команда GDB.

## lost wakeup vs deadlock

**lost wakeup** — программа **может** завершиться с неверным результатом **без** вечного hang. **deadlock** — прогресса нет. В bt при lost wakeup не всегда «все в wait»; иногда один поток завершился, другой ждёт forever. Различай с tom 3.

## future и promise

Main в **`get`**, worker не дошёл до **`set_value`** — worker в compute или упал. bt worker — исключение? **`catch throw`**. Broken promise — runtime error text.

## spin vs block

**spin**: bt показывает твой **`while`** без libc wait. CPU 100% в top. **block**: wait в kernel. Разное лечение.

## livelock

Редко в учебнике: потоки «работают», но прогресса нет — bt не статичен. GDB с Ctrl+C снова — frames меняются. TSan не главный инструмент; логика.

## Когда закрыть GDB и включить TSan

Счётчик «иногда» неверен, hang нет — GDB может не помочь. TSan (tom 7). Hang с mutex — GDB достаточно часто.

## timeout in tests

Some tests wait forever by design on bug — GDB confirms infinite wait location.

## try_lock diagnostic

If code used **`try_lock`**, bt differs — rare in course.

## condition_variable spurious wakeup

Wakeup without notify possible; predicate must handle — GDB shows repeated wait loops.

## thread pool stopped flag

Print **`stopped`** on hang during destructor scenario — ordering bug.

## factory close flag

**`07`**: **`closed`** true while threads blocked on full queue — backpressure scenario.

## document bt snippet

When asking tutor, paste bt only — no need full locals.

## gdb interrupt twice

Rare hang in gdb itself — kill terminal last resort.

## attach to hung process

**`gdb -p`** if forgot to start under gdb — same **`all bt`**.

## wait with timeout **(C++20)**

Not default in C++17 course; **`wait_for`** would change hang story — out of scope.

## checklist before blaming cv

Mutex held? Predicate? **`stopped`**? Queue **`closed`**? Fields — yours from task, theory from tom 3.

## Дополнительный сценарий: check.sh остановился на 06

Не убивай **`check.sh`** сразу: найди PID процесса зависшего exe (**`pgrep -f 06_thread_pool`**) или запускай один target под GDB с самого начала. **`check.sh`** полезен для «есть hang», GDB — для «где именно». Разделение экономит нервы.

## Отличие «молчит» от «медленно»

Bounded queue с sleep в factory может быть **медленной**, но не deadlock. Смотри, растут ли счётчики в test output. Hang — **нулевой** прогресс дольше разумного timeout. GDB нужен для второго класса.

## Практическое закрепление (5–10 минут)

Открой Debug-сборку любого **зелёного** target из `multithread/build/`. Запусти GDB, поставь **`break main`**, **`run`**, выполни **`next`** пять раз, **`bt`**, **`info locals`**, **`quit`**. Повтори на другом target — muscle memory важнее заучивания глав. Если что-то не совпало с текстом главы, проверь cwd и путь к exe: nine из десяти «GDB сломан» — неверный путь или сборка без **`-g`**.

## Связь с главой тома 3 без спойлера кода

Вернись мысленно к инварианту упражнения, которое отлаживаешь: что должно быть истинно перед выходом из **`wait`**, когда очередь считается пустой, когда **`submit`** должен молчать. GDB показывает **момент**, когда инвариант уже нарушен или ещё не достигнут. Исправление остаётся в голове и в **`task.cpp`**, не в команде **`continue`**.

## § Коротко к собесу (повтор)

Сформулируй вслух одним предложением главную мысль этой главы и назови **одну** команду GDB, без которой тема не работает. Если не можешь — перечитай «Краткое резюме» и вернись сюда завтра; spaced repetition лучше зубрёжки за вечер.

## Вопросы для самопроверки

1. Первая команда при зависшем `06_thread_pool`?
2. Как в bt узнать condvar wait?
3. Почему join под mutex опасен для wait_idle?
4. Чем spin отличается от block в bt?
5. Почему неверный счётчик без hang — не кейс GDB?

## Обсуждение с репетитором

**Том:** 6 · **Глава:** `ch08` · **Файл:** `ch08-deadlocks-hangs.md`

**Дальше по курсу:** [Глава 9. Core dump](./ch09-core-dumps.md)
