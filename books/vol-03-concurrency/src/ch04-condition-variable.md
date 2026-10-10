# Глава 4. Condition variable: wait, notify, предикат

## Зачем

**Mutex** отвечает на вопрос «кто сейчас внутри критической секции». Но часто поток должен **ждать условие**: «в очереди появилась работа», «в буфере есть место», «все воркеры простаивают». Крутить **`while` с `lock`/`unlock` и sleep** — трата CPU и гонки на проверке. **`std::condition_variable`** (cv) связывает **ожидание** с **mutex**: поток **атомарно** отпускает mutex и засыпает, пока другой не **уведомит** и не изменит shared состояние под тем же mutex.

Без **предиката** в **`wait`** легко получить **lost wakeup (пропущенное пробуждение)**: notify пришёл, пока waiter ещё не спал. Эта глава — протокол **`wait`/`notify`**, зачем **`notify_one` vs `notify_all`**, почему **`notify` под mutex** допустим, и как cv стыкуется с **`unique_lock`**.

## Роли mutex и cv

**`condition_variable`** сам по себе **не защищает данные** — только очередь ожидания. Инвариант «сколько элементов в очереди» по-прежнему под **`std::mutex`**. Типичный worker:

```cpp
std::mutex m;
std::condition_variable cv;
std::queue<Job> jobs;
bool stopped = false;

void worker_loop() {
    std::unique_lock<std::mutex> lock(m);
    while (!stopped || !jobs.empty()) {
        cv.wait(lock, [&] {
            return stopped || !jobs.empty();
        });
        if (stopped && jobs.empty())
            break;
        Job j = std::move(jobs.front());
        jobs.pop();
        lock.unlock();
        j.run();
        lock.lock();
    }
}
```

Упрощённый скелет: реальный код добавляет счётчики занятости, обработку исключений, **`close`** очереди (глава 11). Здесь важна **форма** цикла.

Перед **`wait`** predicate часто ложен — поток блокируется. После **`notify`** пробуждённый поток **снова захватывает** mutex и **снова** проверяет predicate **до** возврата из **`wait`**. Поэтому «проснулся» ≠ «условие истинно» — только **`while`/predicate** даёт гарантию.

## `wait` с предикатом обязателен

Два эквивалентных стиля:

```cpp
cv.wait(lock, predicate);
// то же, что:
while (!predicate()) cv.wait(lock);
```

**Предикат** перепроверяет условие **после** каждого пробуждения. Пробуждение может быть **spurious (ложным)** — без notify; стандарт разрешает. Пробуждение может быть **ранним**: другой поток забрал последний элемент — predicate снова false, нужно снова спать.

**Lost wakeup** без цикла: producer положил задачу, вызвал **`notify_one`**, consumer ещё не вошёл в **`wait`** — notify потерян, consumer спит вечно, если условие уже не выполнено. **`while` + predicate** чинит: либо не спишь, пока очередь пуста, либо после wake видишь пустую очередь и снова **`wait`**.

## `notify_one` и `notify_all`

**`notify_one()`** — разбудить **одного** waiter'а; хорошо для «добавили одну задачу». **`notify_all()`** — всех; нужно при **shutdown**, смене режима, когда **все** должны пересмотреть predicate (например **`stopped = true`**).

После **`notify_all`** при остановке каждый worker проснётся, проверит predicate, часть выйдет, часть снова **`wait`**, пока очередь не опустеет — это нормально. Экономить **`notify_all`** на shutdown — false economy: зависшие waiter'ы дороже.

## Notify под mutex — можно

Частый вопрос: «notify надо вызывать **после** **`unlock`**, иначе deadlock?» **Вызывать notify, удерживая mutex, — корректно по стандарту.** Waiter в **`wait`** **не держит** mutex, пока спит: он отпустил его внутри **`wait`**. Notify будит waiter; тот снова **захватит** mutex перед возвратом из **`wait`**.

Путаница с **join под mutex** (глава 3): там другой поток **заблокирован на lock**, а не спит в cv с отпущенным mutex. **Notify под mutex** не заменяет правило «не join worker, пока держишь mutex, на котором он может ждать **lock**».

Практика: многие всё равно notify после unlock, чтобы waiter меньше конкурировал за mutex с notifier'ом — микрооптимизация, не требование.

```cpp
{
    std::lock_guard lock(m);
    jobs.push(std::move(j));
}
cv.notify_one();  // тоже OK было notify под lock
```

## Thundering herd и лишние пробуждения

**`notify_all`** на каждый **`push`** будит всех waiter'ов; один заберёт задачу, остальные снова **`wait`** — **thundering herd**. На малом числе потоков терпимо; на большом иногда оптимизируют отдельными cv или **`notify_one`**. При shutdown **`notify_all`** неизбежен: каждый должен увидеть **`stopped`**.

Producer не обязан будить, если **ничего не изменилось** для waiter'ов — но лишний **`notify_one`** без waiter'а harmless: пробуждение просто не кому дать.

## `wait_for`, `wait_until`

**`cv.wait_for(lock, timeout, pred)`** — ждать с deadline; вернёт **`std::cv_status`**. Полезно для периодической проверки **`stopped`** без отдельного timer thread. Не забывай predicate — те же правила lost wakeup.

## Одна cv или несколько

Можно **`not_empty`** и **`not_full`** для bounded buffer (глава 11): producer ждёт «не full», consumer — «не empty». Одна cv на всё проще, но больше spurious wake; две cv точнее будят нужных. Обе схемы легальны при правильном predicate.

## Ошибки API

**`condition_variable`** ждёт только **`std::unique_lock<std::mutex>`** (или совместимый lock). **`lock_guard`** не подходит — нет **`unlock`** для **`wait`**. Нельзя **`wait`** на разных mutex с одной cv — UB.

Для **`std::atomic`** + cv в C++20 есть **`wait` на atomic**; в C++17 классический паттерн — bool/int флаг **под mutex**, не «голый atomic + cv» без осторожности.

## Монитор как паттерн

Классический **monitor**: mutex + cv + данные; все методы берут mutex, **`wait`** по predicate. В C++ это не keyword, а дисциплина класса. Наружу не отдавай ссылки на внутреннюю очередь без замка — клиент обойдёт инвариант.

**`wait`** atomically «отпустил mutex + встал в очередь cv»; пробуждение «взял mutex + вернулся из wait» — поэтому predicate проверяется **под** mutex после wake.

## Упражнение «лифт» и `THEORY.md`

`04_elevator` в `beginner/` — эталонный мини-монитор: mutex + cv + **предикат** «можно ехать». В `THEORY.md` lost wakeup объясняется на этом же сюжете: один `notify` до входа waiter'а в `wait` теряется, если условие проверяли **один раз** через `if`. Книга не подменяет README задачи; если твой лифт зависает «иногда» — первый вопрос: **цикл** или **if** вокруг `wait`?

Лифт также учит, что **shared state** (этаж, направление, флаг движения) меняется **только** под mutex; cv лишь **будит**, когда инвариант уже записан.

## `wait`, `notify` и порядок с happens-before

Notifier, изменивший очередь под mutex и вызвавший `notify`, создаёт цепочку: запись в queue **sequenced-before** unlock; waiter после wake снова держит mutex и видит те же поля. Поэтому «notify под mutex» — не deadlock с waiter'ом: спящий поток **отпустил** mutex внутри `wait`.

Отличай от сценария, где waiter **не** в `wait`, а блокируется на `lock()` — тогда notifier, держа mutex и пытаясь `join`, может зациклиться (глава 3).

## Таймауты и «периодический» stop

`wait_for` с предикатом удобен, когда нужно просыпаться раз в N ms проверить **stopped**, не плодя отдельный timer-thread. Предикат всё равно обязателен: по истечении timeout условие может быть ложным — нужно снова спать или выйти по протоколу shutdown.

Не используй timeout как замену **правильному** `notify_all` при остановке пула — это маскирует lost wakeup, а не лечит протокол.

## § К собесу

- Сформулируй **lost wakeup** и роль **predicate**.
- Почему **spurious wakeup** не баг реализации libstdc++?
- Держит ли waiter mutex **во время** блокировки в `wait`?
- Когда **notify_all** обязателен при shutdown?

## Связь с пулом и конвейером

Пул: producer **`submit`** push под mutex, **`notify_one`**. Остановка: **`stopped`**, **`notify_all`**, **`join` без mutex**. Конвейер: consumer **`pop`** ждёт not_empty; producer **`push`** ждёт not_full; **`close`** меняет predicate «брать больше нечего». Книга описывает протокол; твоя реализация в упражнениях должна совпасть с **тестами как спецификацией**.

## Типичные ошибки

**`wait` без predicate** — один **`if (!jobs.empty()) wait`**. **`notify`** до изменения shared state под mutex. Думать, что **`notify`** передаёт данные — только **будит**, данные в очереди. **`wait` с `lock_guard`**. Забыть **`notify_all`** при shutdown — worker'ы спят, **`join`** висит. Проверять условие **без** mutex снаружи **`wait`**. Путать «notify под mutex = deadlock» с join-under-mutex.


## Глубже: invariant и «почему while»

Инвариант очереди: «`size` согласован с deque, `closed` означает запрет push, не отмену элементов». Любая проверка инварианта снаружи mutex — ложь. Predicate в `wait` — это **формализация** «мне можно продолжать только если инвариант для моей роли истинен».

Два producer'а: оба могут пройти predicate «есть место», оба push — mutex сериализует, predicate после wake перепроверит. Без while один producer мог бы ошибочно считать, что место ещё есть после spurious wake.

## Будить всех или одного — engineering note

`notify_one` экономит thundering herd на hot path; `notify_all` — страховка при смене **глобального** режима (shutdown, close). В сомнении при остановке системы — **all**; при каждом push одной задачи — **one**.


## Мост к atomics

Когда условие — одно слово («флаг готов»), иногда достаточно atomic + spin или **(C++20)** `wait` на atomic; cv нужен, когда условие связано с **структурой** под mutex (размер очереди, несколько полей). Не заменяй cv на atomic «для скорости», не доказав протокол.

## `std::condition_variable_any` **(C++17)** — упоминание

Вариант cv для **разных** типов mutex, если когда-нибудь понадобится не `std::mutex`. В учебных задачах репо — обычный `condition_variable` + `std::mutex`. API wait/predicate то же по смыслу.

## Практика на бумаге

Запиши predicate для worker пула и для producer bounded queue (not_full).

Сценарий: notify_one до wait — почему while, не if?

## Краткое резюме

**`condition_variable`** ждёт условие, связанное с **mutex**. **`wait(lock, pred)`** — цикл, защита от spurious wake и lost wakeup. **`notify_one`** для одной задачи, **`notify_all`** для остановки и массового пересмотра. **Notify под mutex** legal: спящий waiter mutex не держит. Используй **`unique_lock`**. Дальше — **atomics**, когда mutex избыточен для одного слова.

## Вопросы для самопроверки

1. Почему `if (ready) cv.wait(lock);` опаснее `while (!ready) cv.wait(lock);`?
2. Что такое spurious wakeup и как predicate его нейтрализует?
3. Держит ли поток mutex во время сна внутри `wait`?
4. Когда при остановке пула логичнее `notify_all`, а не `notify_one`?
5. Почему `lock_guard` несовместим с `condition_variable::wait`?

## Обсуждение с репетитором

**Том:** 3 · **Глава:** `ch04` · **Файл:** `ch04-condition-variable.md`

**Дальше по курсу:** [Глава 5. Atomics и CAS](./ch05-atomics.md)
