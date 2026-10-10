# Глава 7. Happens-before и synchronizes-with

## Зачем эта тема

Memory order из [предыдущей главы](./ch06-memory-orders.md) — не магические слова, а способ **построить отношение happens-before** между действиями в разных потоках. Стандарт C++ описывает, какие результаты чтений **допустимы**, а какие — нет, даже если «на практике» вы их не видели. Без happens-before data race на обычной переменной остаётся UB; с ним вы можете доказать, что читатель видит опубликованные данные.

Эта глава связывает модель памяти с тем, что ты уже делал руками: **mutex** unlock/lock и **atomic** release/acquire — два частых источника synchronizes-with.

## Happens-before в одном потоке

Внутри одного потока порядок программы (sequenced-before) уже даёт happens-before: если строка A выполняется раньше B, то A happens-before B. Компилятор не может «переставить» так, чтобы сломать **однопоточную** семантику вашего кода — иначе это был бы другой язык.

Между потоками sequenced-before **не** переносится автоматически. Два `std::thread` могут одновременно трогать память; без синхронизации «кто первый» для обычных переменных не определён.

## Synchronizes-with и межпоточный мост

**Synchronizes-with** — особое ребро между atomic-операциями (или между unlock и lock mutex), которое **добавляет** happens-before между потоками. Типичная картина:

- Поток W: записал данные, затем `store(flag, release)`.
- Поток R: `load(flag, acquire)` вернул true, затем прочитал данные.

Load с acquire **synchronizes-with** store с release на **той же** atomic `flag`. Отсюда: запись в `data` в W **happens-before** чтению `data` в R (если чтение данных sequenced-after acquire-load). Это и есть «публикация» без mutex.

**Seq_cst** load/store тоже создают synchronizes-with в рамках глобального порядка seq_cst — проще для головы, дороже на железе.

## Цепочки через несколько потоков

Happens-before **транзитивен**: если A happens-before B и B happens-before C, то A happens-before C. Комбинируется с sequenced-before внутри потока: операция до release в W sequenced-before release; release synchronizes-with acquire; acquire sequenced-before чтение в R — полная цепь.

Без транзитивности модель была бы бесполезной: synchronizes-with давал бы только «соседние» пары, а не смысл «всё в секции до unlock видно после lock». Цепочка может идти через несколько потоков:

Поток 1 публикует в atomic A (release). Поток 2 делает acquire на A, пишет в буфер, делает release на B. Поток 3 acquire на B и читает буфер. Если протокол выдержан, поток 3 видит работу потока 2, а через него — эффекты, которые поток 1 «передал» потоку 2.

Ошибка проектирования: разорвать цепь relaxed-операцией там, где следующий участник **ожидает** видеть связанные данные. Relaxed на промежуточном «счётчике сообщений» может быть OK, relaxed на **единственном** флаге готовности — нет.

```cpp
// Упрощённая идея «передал эстафету»
// T1: payload = ...; stage1_done.store(true, release);
// T2: while (!stage1_done.load(acquire)) {}
//     postprocess(); stage2_done.store(true, release);
// T3: while (!stage2_done.load(acquire)) {}
//     consume(payload);
```

## Mutex: unlock synchronizes-with lock

`std::mutex` — не только «взаимное исключение», но и **носитель happens-before**. Если поток W держал mutex, изменил структуру под замком и сделал **unlock**, а поток R затем **lock** на том же mutex и читает структуру, то unlock W **synchronizes-with** lock R. Все записи в критической секции W happens-before чтениям в секции R **при условии**, что вы не вытащили данные наружу без замка и не оставили указатели на внутренности, которые другой поток меняет без согласования.

Отсюда практическое правило из упражнения с кассой: `++` под `lock_guard` — не только «никто не вклинится», но и **видимость** нового значения для следующего lock. Без mutex два `++` на обычном `int` — data race и UB, даже если «иногда сходится».

**Важно:** synchronizes-with даёт порядок **между** критическими секциями, а не внутри вашей логики «между unlock и следующей строкой в том же потоке» — внутри одного потока по-прежнему sequenced-before.

## Condition variable и happens-before

`condition_variable::wait` атомарно отпускает mutex и засыпает; при пробуждении снова захватывает mutex. Пробуждение после `notify` **не** заменяет предикат: вы должны проверить условие под mutex, потому что spurious wakeup и гонки notify возможны. С точки з models: notify под mutex (как в учебных паттернах) согласован с тем, что wait вернулся с замком — изменения, сделанные notifier-ом под mutex до notify, видны waiter-у после повторного lock.

Lost wakeup (wait без предиката после раннего notify) — это **логическая** ошибка протокола, а не «cv сломан». Happens-before не спасёт, если вы спите, не проверив, что условие уже истинно.

## Atomic RMW и CAS

Успешный `compare_exchange` с подходящим success order создаёт synchronizes-with относительно предыдущего release другого потока (в типичных алгоритмах «очередь через CAS»). Неудачный CAS с relaxed failure order **не** обязан давать вам новое знание о связанных данных — только о значении atomic. Поэтому в lock-free коде failure/success orders выбирают осмысленно, а не «везде relaxed».

## Scoped lock и hand-over-hand

`std::scoped_lock` на нескольких mutex **(C++17)** не меняет модель: каждый unlock→lock по-прежнему synchronizes-with. Важно **не** держать два замка дольше нужного — deadlock между воркерами пула и клиентом редок, но возможен, если callback из job снова лезет в тот же mutex в другом порядке.

Hand-over-hand: поток A отпустил mutex после записи в очередь, поток B взял mutex и прочитал — happens-before между записью элемента и чтением **если** элемент не «утёк» указателем наружу без правил lifetime.

## Data race vs happens-before

**Data race** — два неatomic доступа к одной памяти, хотя бы один — запись, без happens-before между ними. Итог — UB. Atomic с правильным протоколом **устраняет** гонку на **самой** atomic; соседние поля всё ещё должны попадать под release/acquire или mutex.

«Я один раз записал, другой читает без sync, но int маленький» — всё равно race. «Я использую atomic для int» — гонки на этом int нет, но согласование **других** переменных — отдельный вопрос.

## Coherence и «не два значения сразу»

Для одной atomic переменной стандарт гарантирует **coherence**: все потоки согласуются на одной последовательности записей этой atomic, и load не «вернёт назад» к более старому значению без логики. Это **не** распространяется на соседние неatomic поля: там без happens-before возможны «рваные» наблюдения.

Поэтому «я прочитал atomic версию 5» не означает «структура версии 5 консистентна», если консистентность обеспечивалась только mutex в другом потоке, а вы читаете поля без lock.

## Порядок инициализации static

Локальные static в функции **(magic statics)** инициализируются потокобезопасно один раз **(C++11)** — это отдельный пункт стандарта, не ваш release на флаге. Глобальные static до `main` — другой порядок; для многопоточности до `main` лучше не полагаться на сложные зависимости между translation units.

В пуле и очередях важнее: **конструктор** объекта пула завершился до того, как воркеры начали `[this]` — обычный sequenced-before в одном потоке до `thread` start.

## Собрать цепочку для mutex-очереди (мысленное упражнение)

Producer: `lock` → `push(job)` → `unlock` → `notify`. Worker: `wait` с predicate → `pop` под lock → `unlock` → `job()`. Запись в deque **happens-before** pop, потому что unlock producer'а (после push) synchronizes-with lock worker'а (перед pop) **когда** worker берёт mutex после wake. Выполнение `job()` **после** unlock — намеренно вне mutex; side effects job **не** должны ломать инвариант очереди без нового lock.

Если job пишет в shared state без собственной синхронизации — это **новая** зона ответственности API submit.

## `join` как точка синхронизации

`thread::join()` создаёт synchronizes-with между завершением callable и возвратом из `join` в другом потоке. После join локальные результаты worker'а, записанные в **безопасно опубликованную** память (например, заполненный до join массив, к которому worker больше не пишет), видны joiner'у. Передача указателя на stack worker'а — по-прежнему UB.

В `08_parallel_map` join всех потоков **перед** чтением `out` — часть корректности: пока join не прошёл, читать результат нельзя.

## Coherence vs «согласованность структуры»

Повторим: coherence на одной atomic **не** распространяется на соседние поля. Mutex на «всю структуру» публикует **все** поля под замком одним unlock→lock. Смешивать «mutex на head, relaxed tail» без доказанного протокола — типичная ошибка «я же синхронизировал что-то».

## § К собесу (углубление)

- Транзитивность **happens-before**: приведи пример из трёх потоков.
- Может ли **acquire** load без matching **release** создать HB для plain data?
- Как **cv** + mutex вписываются в HB (unlock перед wait, lock после wake)?
- Чем **data race** отличается от «race condition» в разговорной речи?

## Связь с async и future (граница тома 4)

`future::get()` и разрушение связанных объектов вводят synchronizes-with между потоком, который установил результат, и потоком, который get-ит. Детали executors и coroutines — в томе 4; здесь достаточно: **get/join** — точки, где можно считать, что side effects callable видны.

## Рисунок в голове: две нитки и мост

Представь две вертикальные «ленты» — поток W и поток R. Горизонтальные стрелки sequenced-before только внутри ленты. Мост **synchronizes-with** — только на atomic (release→acquire) или mutex (unlock→lock). Чтобы утверждение «R видит запись W в `data`» было законным, должна быть цепь: запись в `data` sequenced-before release в W, release synchronizes-with acquire в R, acquire sequenced-before чтение `data` в R. Если `data` — неatomic, любой обход моста без такой цепи — data race.

Полезное упражнение с репетитором: взять свой код с mutex и подписать unlock/lock; затем мысленно заменить mutex на флаг и проверить, не пропали ли поля вне atomic.

## Типичные заблуждения

«Volatile или sleep даёт happens-before» — нет. «Два mutex на разные поля одной структуры без инварианта» — два независимых моста; составной инвариант может быть сломан между lock. «Acquire на load, который никогда не synchronizes-with чужим release» — вы просто крутите цикл, не имея права читать данные.

В упражнении с лифтом (`condition_variable`) predicated wait как раз связывает «условие истинно» с тем, что notifier записал под mutex до `notify`. Без этого моста вы снова в мире spurious wakeup и lost wakeup, а не в мире «cv сам синхронизирует поля».


## Глубже: publication safety и указатели

Паттерн: allocate node, init fields, `atomic_store(ptr, node, release)`; reader `load(acquire)` и dereference — **если** reader не delete node, пока другой может load. Lifetime и HB — **оба** нужны. Lock-free без reclamation plan — UB через UAF, не через «слабый order».

## Sequential consistency как default policy команды

Coding guideline: «atomics без второго аргумента order; ослабление только с review и комментарием протокола» — разумный default для прикладного C++17 до появления in-house expert.


## Мост к lock-free и debugging

Lock-free — те же HB-стрелки, только их больше и они на CAS. TSan (глава 14) строит HB граф для **отчётов**; он не заменяет вашу диаграмму для atomics ordering.

## Разбор «почему mutex публикует всё под замком»

Unlock **happens-before** lock не «переносит» только одно поле — переносится **все** side effects в critical section W, которые sequenced-before unlock, **если** R не читает данные через указатели, которые W оставил наружу без правил. Если W положил в queue указатель на stack — HB не спасает lifetime.

Atomic release на **одном** флаге публикует только то, что sequenced-before release **и** что reader читает **после** acquire по протоколу «флаг → данные». Поля вне этого протокола — отдельная история.

## Практика на бумаге

Нарисуй unlock→lock между W и R на одном mutex.

Может ли load relaxed на флаге создать HB для plain buffer?

## Краткое резюме

Happens-before — частичный порядок событий; synchronizes-with связывает atomic release/acquire, seq_cst операции и mutex unlock/lock между потоками. Цепочки транзитивны. Mutex и cv в учебных задачах — способ **гарантировать** видимость без ручного рисования флагов. Atomic без пары release/acquire (или seq_cst) не публикует произвольную память.

## Вопросы для самопроверки

1. Чем sequenced-before в одном потоке отличается от happens-before между потоками?
2. Почему unlock в W и lock в R на одном mutex создают synchronizes-with?
3. Зачем после acquire-load читать `data` только **после** load в исходном коде (sequenced-after)?
4. Может ли relaxed atomic участвовать в synchronizes-with для передачи буфера?
5. Почему wait на cv без предиката ломает протокол даже при «правильных» notify?
6. Нарисуй цепь W→R для mutex: unlock, lock, чтение поля под вторым lock.

## Обсуждение с репетитором

**Том:** 3 · **Глава:** `ch07` · **Файл:** `ch07-happens-before.md`

**Дальше по курсу:** [Глава 8. Lock-free: идея и осторожность](./ch08-lock-free.md)
