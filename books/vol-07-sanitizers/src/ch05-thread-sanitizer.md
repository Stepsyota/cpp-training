# Глава 5. ThreadSanitizer

## Зачем эта тема

**Data race** — два потока обращаются к одному адресу, хотя бы одна запись, нет happens-before между ними — **UB** (том 3). **ThreadSanitizer (TSan)** отслеживает синхронизацию и **репортит race** с двумя стеками. Это главный инструмент курса `multithread/` после code review.

## Включение

```bash
g++ -std=c++17 -pthread -g -O1 -fsanitize=thread -fno-omit-frame-pointer \
    task.cpp test.cpp -o t && ./t
```

**`-pthread` обязателен** на compile и link. Без него — странные ошибки или неполная картина.

Clang на Fedora historically сильен в TSan; GCC тоже поддерживает TSan на x86_64 в новых версиях — глава 10.

## Что TSan видит

Два `fetch_add` на одном **non-atomic** `int` — race.

`vector::push_back` из двух потоков без mutex — race на внутренних полях.

Один поток пишет `bool ready`, другой читает plain `bool` без sync — race (нужен atomic или mutex).

## Что TSan не гарантирует

**Deadlock** — висит, отчёта «deadlock» нет (GDB).

**Неверный инвариант** под mutex (выкинули задачи из очереди) — логика.

**Гонка не воспроизвелась** на коротком тесте — недетерминизм.

**Правильные atomics** с **неверным memory order** — TSan может не пожаловаться; нужен разбор модели (том 3).

## Overhead

TSan сильно замедляет и ест память (shadow для каждого byte access tracking). Не используй TSan-сборку как «быстрый» бинарник для бенчмарков.

## Связь с томом 3 глава 14

Чеклист тома 3 заканчивается советом прогнать TSan — здесь механика.

## vector reallocation race

Two threads `push_back` same vector — internal capacity/data races — TSan on libstdc++ internals + your line push_back call.

## atomic vs mutex

TSan understands both. **Lock order** inversion — deadlock, not TSan.

## thread_local

Per-thread storage — no cross-thread race on **different** thread_local instances. Race if pointer to thread_local escaped wrong — rare.

## fork

TSan + fork — special rules; not in course exercises.

## External thread pool

If you add third-party pool without TSan — problems — stick to std::thread in course.

## Report size

Huge reports on many races — fix one sync bug may clear many — cascade.

## Happens-before edge cases

**Release seq** before consumer read — tom 3; TSan models pthread.

## Training: oracle async

If shared counter in task without sync — TSan; async doesn't magically sync.

## Типичные заблуждения

«atomic всегда без race» — wrong order всё ещё баг логики. «mutex всегда виден TSan» — если забыли lock, TSan видит. «TSan в CI на каждый push» — тяжело, но желательно на ночных job.

## Data race в терминах стандарта

**Data race** — два доступа к одному объекту, хотя бы один write, без **happens-before** (том 3). Это **UB**. TSan строит **happens-before graph** из mutex, atomic, thread create/join и репортит конфликт.

Plain `int counter` + два `std::thread` с `++` — textbook; **02 ticket_office** без mutex.

## Обязательные флаги

```bash
g++ -std=c++17 -pthread -g -O1 -fsanitize=thread     -fno-omit-frame-pointer task.cpp test.cpp -o t
```

**`-pthread`** на compile **и** link. Забыл — weird link errors или incomplete instrumentation.

## Что TSan видит в advanced

- `deque` / queue без mutex — races на control block.
- `bool stopped` plain — race с worker.
- `busy` counter без sync — race.
- **Правильный mutex** на всех полях — TSan чист на **race**; логика всё ещё может быть wrong.

## Что TSan не гарантирует

**Deadlock** — hang, `thread apply all bt` (tom 6).

**Lost wakeup** без race — логика cv.

**Forgotten join** — `std::terminate` от деструктора `thread`, не TSan report.

**Memory order** bugs на atomics — TSan может **не** пожаловаться; нужен tom 3.

## Overhead и память

TSan **slow**. Не бенчмаркуй **08_parallel_map** на TSan binary. RSS растёт — shadow per access tracking.

## atomics и TSan

**Correct** `atomic` ops on same atomic — **no data race** on that object. Wrong algorithm (CAS loop bug) — logic, TSan silent.

## Связь с tom 3 ch14

Чеклист «прогнать TSan» — здесь механика. Preview в tom 3 → полный tom 7.

## § К собесу

TSan = happens-before; pthread; not deadlock; not logic; `-O1`; separate from ASan.

<!-- vol7-pass5-ch05 -->

## Дополнительное закрепление

Data race definition — без подглядывания tom 3. `atomic` убирает race **на этом** atomic.

TSan slow на **06** — планируй время.

`join` создаёт happens-before — TSan использует.

## Краткое резюме

`-fsanitize=thread` + `-pthread` + `-g` + `-O1`. Ловит data race. Не deadlock, не логику. Дальше — читать отчёт.

## Сценарий 02 без спойлера решения

Два потока, shared counter, тест считает sold tickets. TSan: conflicting accesses on **same address** as counter object. Fix category: **mutual exclusion** or **atomic** — репетитор не требует paste кода, только **класс** fix.

## std::atomic still needs correct usage

`atomic<int>` with race on **different** variable — TSan on that variable. Two atomics — no race on same location if only one atomic object.

## join and happens-before

`thread::join` synchronizes-with thread completion — accesses after join hb — TSan uses that edge.

<!-- vol7-pass6 -->
## Строка для памяти (Kindle)

Перед сном одним взглядом: **compile+link**, **-g -O1**, **TSan+pthread**, **два build dir**, **два стека в отчёте**, **solutions не спойлерить**. Завтра — один TSan-прогон на 02 или на последнем изменённом advanced target.

## Вопросы для самопроверки

1. Обязательные флаги для TSan на Linux?
2. Пример race из 02?
3. Ловит ли TSan forgotten join?
4. Почему не бенчмаркить TSan-build?
5. Связь с UB из стандарта?

<!-- vol7-pass4-ch05 -->
## TSan как экзамен на tom 3

Если можешь связать **определение data race** с строками отчёта TSan — tom 3 усвоен practically. Если отчёт «магия» — вернись к happens-before, mutex, atomic (tom 3 ch5–7).

TSan **не** освобождает от `./check.sh`. Функциональные тесты ловят **spec** («должно быть N билетов»); TSan ловит **UB параллелизма**. Оба нужны.

Запомни: **`-pthread`** — не «для std::thread красиво», а часть **контракта** TSan runtime с libpthread на glibc Linux.

## Обсуждение с репетитором

**Том:** 7 · **Глава:** `ch05` · **Файл:** `ch05-thread-sanitizer.md`

**Дальше по курсу:** [Глава 6. Отчёт TSan](./ch06-tsan-report.md)
