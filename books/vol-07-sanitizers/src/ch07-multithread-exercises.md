# Глава 7. Упражнения multithread

## Зачем эта тема

Том 3 объяснил гонки; том 7 даёт **проверку** на своём коде. Ниже — **какие** упражнения гонять с TSan и что ожидать, **без** эталонных реализаций из `solutions/`.

## Сборка одного упражнения с TSan

Из `multithread`, после обычного configure:

```bash
cd multithread
cmake -S . -B build-tsan -DCMAKE_BUILD_TYPE=Debug
cmake --build build-tsan --target 02_ticket_office \
  -- VERBOSE=1
```

Проще для разового прогона — ручная строка из папки beginner (глава 9 CMake):

```bash
cd multithread/beginner/02_ticket_office
g++ -std=c++17 -pthread -g -O1 -fsanitize=thread -fno-omit-frame-pointer \
    -Wall -o t task.cpp test.cpp && ./t
```

Если тест **зелёный** без TSan и **красный** с TSan на намеренно сломанном `++` без mutex — инструмент работает.

## 02 ticket_office

Сценарий: два потока, общий счётчик. Без `lock_guard` TSan должен показать **conflicting access** на поле счётчика. После mutex — чисто (на этом тесте; не доказывает всю программу).

## 05 highscore

CAS loop на atomic — **корректный** путь без mutex. Сломанный «load; if; store» на обычном `int` — TSan. Правильный `atomic` + CAS — TSan молчит на race этого поля; ошибки логики CAS TSan не ловит.

## 03 oracle, 04 elevator

Меньше shared mutable state; TSan полезен при ошибках lifetime с `[&]` или гонке на флаге без atomic. `cv` без предиката — чаще **логический** lost wakeup, не всегда TSan.

## 06–08 advanced

Пул и очередь: гонки на `deque`, `stopped`, `busy` без mutex — TSan. **Deadlock** TSan **не** диагностирует как «deadlock» — программа висит; GDB (том 6). **Закрыли очередь и потеряли jobs** — логика, не TSan.

## Stress

Увеличь итерации в локальной копии теста или запускай `check.sh` циклом **только** на TSan-сборке. Том 3: тест `08` делится нацело — TSan + странные результаты намекают на дыру в разбиении.

## check.sh

Скрипт хозяина по умолчанию без sanitizer. Отдельный прогон TSan — **твоя** привычка перед «сдал модуль», не меняем CI без запроса.

## beginner vs advanced time budget

TSan all 8 targets — long. Prioritize **changed** targets since last green check.sh.

## 04 elevator predicate

Review: `wait(pred)` — no race on cv internals if used correctly; flag outside may race.

## 06 wait_idle

Uses cv + mutex — if busy wrong, test fails; TSan on busy if unsynced.

## 07 factory.cpp

Don't need to TSan-edit factory if only header changed — rebuild target 07.

## Parallel map empty input

Edge case — no threads — TSan quiet — OK.

## n_threads > size

min logic — wrong code may still race if shared counter for chunks wrong.

## Document local workflow

Notebook: «build-tsan path, flags, date» — future you on new laptop (tom 5).

## Pair with tom 6

TSan report line → breakpoint file:line in GDB on **non-tsan** build optional — addresses differ; use line only.

## Типичные заблуждения

«Зелёный TSan раз = навсегда» — гонки недетерминированы; stress помогает. «TSan на 06 заменяет code review» — нет. «Копировать solutions чтобы TSan прошёл» — смысл тренажёра теряется.

## Принцип: свой код, не solutions

Эталоны **`multithread/solutions/`** обходят обучение. TSan на **твоём** `task.cpp` / `thread_pool.hpp` — цель. Намеренно сломай на минуту — убедись, что инструмент **реагирует**.

## 01 rocket_launch

Мало shared mutable state. TSan — если добавил global flag без sync. Join discipline — **terminate**, не TSan.

## 02 ticket_office (эталонный кейс)

Два потока, один счётчик. Без **lock_guard** — **conflicting access** на counter. С mutex — clean **on this test**. Stress: больше итераций в **локальной копии** test, не обязательно commit.

## 03 oracle

**async** + future — другой sync path. Race если shared state в oracle path без protection. Lifetime `[&]` — ASan/UBSan иногда.

## 04 elevator

**condition_variable** + pred. **Lost wakeup** — logic. Race на shared **bool** flag without atomic — TSan.

## 05 highscore

Broken load-if-store on plain `int` — TSan. **atomic** + CAS — no race on atomic; wrong max — logic.

## 06 thread_pool

Race on **jobs**, **stopped**, **busy** without mutex. **Deadlock** if join under same mutex worker holds — GDB. **Notify under lock** — OK (tom 3). Don't spoiler pool code from solutions.

## 07 bounded queue

**cap_**, **closed**, **q** under mutex. Pop when `closed && empty` — logic formula. **move** from queue — if double pop without sync — TSan.

## 08 parallel_map

Disjoint output ranges — no lock on **out** if ranges disjoint. Wrong slice formula — wrong results, **may** pass tests (tom 3: tail). TSan if shared index wrong. Stress + TSan.

## check.sh и CI

Default **check.sh** — no sanitizer. Habit: TSan build before «module done». Tom 5: CI **-Werror** separate from TSan job idea. **cicd/** in repo — tom 5 link; optional future sanitizer stage **not** changed by book.

## Локальная копия для экспериментов

Копируй папку упражнения в `/tmp`, ломай там — не портить git tree.

## § К собесу

02/05 canonical TSan; 06-08 shared structures; stress; no solutions; GDB hang.

<!-- vol7-pass5-ch07 -->

## Дополнительное закрепление

Advanced: shared поля под mutex? — без solutions. TSan не проверяет формулу `pop`.

**check.sh** first; TSan second.

02 smoke TSan; 06–08 когда трогал код.

## Краткое резюме

02/05 — эталонные учебные кейсы для TSan. 06–08 — shared структуры под mutex. Stress и отдельный build dir. GDB для hang. Дальше — комбинации флагов.

## Таблица упражнений и инструментов

| Упражнение | TSan ожидание | ASan | GDB |
|------------|---------------|------|-----|
| 02 | race без mutex | редко | редко |
| 05 | race на broken int | — | — |
| 06 | race на queue fields | UAF редко | deadlock |
| 07 | race на q | — | hang |
| 08 | race если shared | — | — |

## Время прогона

TSan 06 может быть **минуты** — нормально. Планируй кофе, не отменяй sanitizer из impatience.

## git diff перед TSan

Знай **что** менял — гоняй targets, зависящие от файлов diff.

<!-- vol7-pass6 -->
## Строка для памяти (Kindle)

Перед сном одним взглядом: **compile+link**, **-g -O1**, **TSan+pthread**, **два build dir**, **два стека в отчёте**, **solutions не спойлерить**. Завтра — один TSan-прогон на 02 или на последнем изменённом advanced target.

## Вопросы для самопроверки

1. Как собрать 02 с TSan одной командой?
2. Что TSan должен увидеть в ticket без mutex?
3. Почему 05 на atomic не «гонка» при правильном CAS?
4. Кто ловит deadlock?
5. Зачем stress для 08?

<!-- vol7-pass4-ch07 -->
## Глава-мост к практике

Эта глава **специально** без кода из `solutions/`: только **какие** targets, **какие** симптомы, **какие** инструменты. Реализацию проверяешь ты; репетитор спрашивает «что показал TSan на строке X», не «вставь mutex сюда».

Для **06–08** TSan может быть **долгим** — запускай вечером, не перед быстрым commit. Для **02** — секунды; используй 02 как **smoke** «TSan toolchain жив».

Связь **cicd/**: отдельный трек в repo; sanitizer-идеи те же — отдельный build tree, tom 5. Не путай `multithread/check.sh` с CI forge, если трогаешь оба.

## Обсуждение с репетитором

**Том:** 7 · **Глава:** `ch07` · **Файл:** `ch07-multithread-exercises.md`

**Дальше по курсу:** [Глава 8. Комбинации санитайзеров](./ch08-combining-sanitizers.md)
