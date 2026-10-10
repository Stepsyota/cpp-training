# Глава 12. Резюме тома и мифы

## Зачем эта глава

Двенадцать глав подряд — много деталей: capture, init-move, mutable, generic lambda, `function`, STL, потоки, dangling. Перед тем как закрыть том 2 и открыть том 3 (потоки и память) или вернуться к своему коду с пулом и очередью, полезно **сжать картину** и снять оставшиеся мифы одним проходом. Эта глава не вводит новый синтаксис; она связывает лямбды с пройденным треком и с томом 1.

## Сквозная модель

Лямбда создаёт **closure object** с `operator()` и полями **capture**. Синтаксис `[](){}` — объявление объекта; вызов — метод. **`[=]` / `[&]`** — default правила копии и ссылки; явный список читается лучше в production. **Init-capture** **(C++14)** — поля с инициализатором и move. **`mutable`** меняет копии внутри non-const `operator()` относительно полей. **Generic lambda** шаблонирует **вызов**, не захват. **`std::function`** стирает тип для API; **шаблон `F`** сохраняет perf. В **потоках** побеждают копия, init-capture и join; не слепой `[&]`. **Lifetime** — главный класс багов.

Предложи себе объяснение вслух: «Лямбда — это …» — если фраза заканчивается на «функция», вернись к главе 1.

## Мифы, которые ещё встречаются

**«Лямбда — это функция»** — объект с `operator()`. **«Компилятор создаёт новую функцию в .text на каждый вызов»** — один closure type, много вызовов одного `operator()`. **«auto лямбда = нет типа»** — тип есть, скрыт. **«std::function бесплатен»** — может аллоцировать и indirection. **«mutable = потокобезопасно»** — нет. **«Захват this = GC удерживает объект»** — нет, сырой указатель. **«C++20 template lambda есть в C++17»** — проверяй `-std=`. **«[=] в thread = безопасно»** — копии безопаснее ссылок, но shared mutable state всё ещё гонка. **«Захват всего через [&] короче = лучше»** — часто хуже для review и UB.

## Чеклист code review лямбды

1. Список захвата: default `[&]` в callable, который **уходит наружу** (return, callback, thread)?
2. **`this`**: объект переживёт все вызовы, включая очередь?
3. Тяжёлые копии в `[=]` на hot path (строки, vector в каждой задаче)?
4. Ссылки на итераторы или сырые указатели после possible reallocation?
5. Возврат лямбды из функции: что захвачено — копии или ссылки?
6. Нужен ли **`std::function`** или достаточно **`template F`** / concrete `auto`?
7. Side effects в predicate при **`execution::par`**?
8. **`mutable`** + shared объект между потоками без mutex?

Распечатай или держи в заметках; через месяц перечитай главы 3, 9, 10.

## Связь с репозиторием training

**03_oracle** — callable и future; захват и state должны быть valid до `get()`. **06_thread_pool** — очередь работ, владение задачей, `[this]` у воркера — на уровне идей из главы 9, без эталона из `solutions/`. **07_bounded_queue**, **08_parallel_map** — callable в другом контексте; parallel map не требует `function`, часто шаблон и конкретная лямбда на куске данных. **THEORY.md** — move и capture; не противоречь «move = метка для перегрузки». **Том 1** — `mutable`, `auto`, `this`, `template`. **Том 3** — mutex, atomic, happens-before поверх этих callables.

## Карта глав для повторения

| Если споткнулся о… | Перечитай |
|--------------------|-----------|
| Синтаксис, IIFE | ch02 |
| `[&]` vs `[=]` | ch03 |
| unique_ptr в задачу | ch04 |
| Счётчик внутри лямбды | ch05 |
| auto параметры | ch06 |
| function в API | ch07 |
| sort / find_if | ch08 |
| thread / async | ch09 |
| return lambda, this | ch10 |

## Мини-сценарий «пройди review»

Код: `pool.submit([&]{ process(item); });` где `item` — ссылка на локальную переменную цикла. Три бага: lifetime `item`, возможно `[&]` на `this`, гонка на `item` без sync. Исправление словами: capture by value/move копию `item`, не держать ссылку на loop variable, sync если shared — без написания эталона пула.

## Плотность vs том 1

Том 1 шире по охвату keywords; том 2 глубже в **одну** ось — callable и closure. Если после тома 2 всё ещё неясно `[&]` — это нормально: перечитай 3→9→10 с паузой и одним упражнением из `03_oracle` или минимальным `thread` + join локально.

## EPUB и перечитывание на Kindle

Абзацы короткие для узкого экрана; код — короткими блоками. Закладки ставь на чеклист (эта глава) и на «Вопросы» в ch03/ch10.

## К собесу: пять коротких ответов

1. Closure type — compiler-generated class + capture members + `operator()`.
2. Capture by reference — не extends lifetime.
3. Init-capture — C++14, move unique_ptr.
4. Generic lambda — templated `operator()`, C++14 auto / C++20 explicit template.
5. `std::function` — type erasure, возможная heap allocation.

## Что читать дальше

По серии: **том 3** — многопоточность углублённо (mutex, condition_variable, memory order). **Том 4** — async, executors, coroutines. Если лямбды в алгоритмах показались скомканными — практика STL отдельно; этот том давал паттерн predicate/comparator.

Перечитать проблемные главы после первого прохода: обычно **3, 9, 10**.

## Одна страница «шпаргалка» (prose)

Capture by value — копия в объекте, безопаснее для escape. Capture by reference — только если lifetime доказан. Init-capture — move и rename. Mutable — менять поля-копии между вызовами. Generic — auto/C++20 template на параметрах. Storage — auto vs function vs template F. Threads — join, не dangling. Review — чеклист выше. Альternatives — functor, fn ptr, bind legacy.

## Благодарность процессу обучения

Ты прошёл multithread без «курса лямбд» — нормально. Том 2 даёт слова на то, что уже делал руками. Если при перечитывании узнаёшь свой bug в ch10 — цель достигнута.

## Финальное упражнение (мысленное)

Напиши на бумаге capture list для: (a) sync sort with external threshold, (b) thread with copy of id, (c) returning callback from factory, (d) pool worker with fields access. Answers: (a) `[t]` or `[&t]` sync, (b) `[id]`, (c) value only, (d) `[this]` + lifetime proof.

## После тома 2

Открой свой multithread project file with lambdas — one pass with checklist from this chapter.

## Статус издания

Этот том доведён до **книжного** повествовательного уровня серии (октябрь 2026): связные абзацы, сценарии, без спойлеров solutions.

## Словарик тома (одним проходом)

**Closure type** — сгенерированный класс. **Capture** — поля или ссылки в нём. **Default capture** — `[=]` или `[&]`. **Init-capture** — `[id = expr]` **(C++14)**. **Mutable** — non-const `operator()` для полей-копий. **Generic lambda** — `auto` или template params на аргументах. **Type erasure** — `std::function`. **IIFE** — вызов лямбды в той же строке. **Dangling capture** — ссылка пережила referent. **Callable** — всё, что можно вызвать с `()`.

Прочитай словарик вслух один раз перед сном после главы 12 — если термины не вызывают паузы, том усвоен на базовом уровне. Если «erasure» или «odr-used» всё ещё пустые — вернись к главам 1 и 7.

## Пройди по главам: одна фраза на главу

1 — closure object. 2 — синтаксис и IIFE. 3 — `[=]` vs `[&]`. 4 — init-capture move. 5 — mutable поля. 6 — generic operator(). 7 — function erasure. 8 — STL predicate. 9 — thread join. 10 — dangling. 11 — functor legacy. 12 — checklist. Если любая цифра пустая — перечитай только её файл за 15 минут.

## Сравнение с томом 3 (ожидания)

Том 3 не переобъяснит `[this]` с нуля — он добавит mutex, condition_variable, atomics, memory order. Том 2 обязателен перед глубоким concurrency, иначе «почему TSan ругается на capture» останется загадкой.

## Благодарность будущему себе

Через полгода при добавлении нового `std::thread` или `submit` в учебный проект открой только ch10 checklist — 60 секунд review дешевле часа gdb. Том 2 задуман как **перечитываемый** справочник по стилю, не как одноразовый tutorial.

## Краткое резюме тома 2

Лямбда — closure; capture задаёт state и lifetime. C++14 init-capture и move; C++20 template lambda. function vs template — trade-off API и perf. STL и потоки — главные потребители; осторожность с `[&]` и `this`. Альтернативы — функтор, function pointer, legacy bind.

## Version table (prose)

C++11: lambdas. C++14: init-capture, generic auto params. C++17: constexpr lambda, [*this]. C++20: template lambda. Mark your `.cpp` standard in CMake.

## Параграф для Kindle (закрепление)

Перечитай ключевую мысль главы без кода: запиши одно предложение от руки. Для этой главы сформулируй, **что** должно остаться в памяти после чтения, и **какую ошибку** ты больше не хочешь повторять в своём `.cpp`. Связь с треком multithread — только теория; код упражнений по-прежнему твой.

## Вопросы для самопроверки

1. Сформулируй одним предложением, что такое closure type.
2. Назови три пункта из чеклиста review.
3. Какой миф про `this` опаснее всего в async?
4. Куда в серии идти после тома 2?
5. Чем том 2 дополняет `multithread/THEORY.md`, а не дублирует?
6. Когда init-capture обязателен?
7. Почему generic lambda не решает проблему разных типов в одном `vector` без erasure?

## Обсуждение с репетитором

**Том:** 2 · **Глава:** `ch12` · **Файл:** `ch12-summary-myths.md`

**Конец тома 2.** Дальше по серии: том 3 — многопоточность (`books/vol-03-concurrency`, по плану серии).
