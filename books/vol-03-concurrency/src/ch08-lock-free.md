# Глава 8. Lock-free: идея и осторожность

## Зачем эта тема

Happens-before и atomics из [главы 7](./ch07-happens-before.md) — фундамент; здесь смотрим, куда их применяют в **структурах данных** без mutex.

Mutex прост для головы: один зашёл — остальные ждут. Но блокировка несёт цену: contention, priority inversion, риск deadlock при нескольких замках, и воркер может «заснуть» в kernel, пока держит lock. **Lock-free** структуры обещают прогресс без mutex: потоки двигают общее состояние через atomics и CAS, не выстраиваясь в очередь у одного замка.

Эта глава — **не** рецепт «перепиши очередь на CAS». Это карта местности: что значит lock-free, какие гарантии прогресса бывают, почему ABA ломает наивные алгоритмы, и когда mutex честнее.

## Что значит lock-free

В терминах стандартной классификации (Herlihy & Shavit и последующие учебники) структура **lock-free**, если после **конечного** числа шагов **хотя бы один** поток в системе завершил операцию над структурой, даже если другие подвисли или их медленно вытесняет планировщик. Иначе говоря: система в целом не «застряла навечно из-за одного lock».

**Wait-free** сильнее: **каждый** поток завершает операцию за **конечное** число своих шагов — независимо от скорости других. Wait-free редок в промышленных очередях; lock-free чаще достижим, но всё равно сложен.

**Obstruction-free** слабее: если никто не мешает, поток завершит операцию за конечное время. На практике для сервисов обычно хотят хотя бы lock-free, а не obstruction-free «на удачу».

Lock-free **не** значит «без ожидания вообще»: CAS-цикл может крутиться много раз; spin на atomic — тоже ожидание, только без mutex.

## Почему mutex иногда лучше

Lock-free очередь или стек требует: корректный memory order, безопасное **удаление** узлов (кто освобождает память?), handling ошибок allocator, тестирование под TSan и на слабых ISA. Для учебного пула или bounded queue в репозитории mutex + cv — **правильный** уровень абстракции: понятный инвариант, join/shutdown без hazard pointers.

Переходите к lock-free, когда профиль показал lock как bottleneck **и** у команды есть ресурс доказывать и сопровождать алгоритм. Иначе это оптимизация ради резюме.

## CAS как кирпич

Compare-and-swap: «если atomic == expected, записать desired, иначе обновить expected». Успех — вы изменили глобальное состояние; неудача — кто-то опередил, перечитайте и повторите. Lock-free стек «push» часто выглядит как CAS на голове указателя; «pop» — CAS, который сдвигает голову.

Каждый успех/неудача должны быть вписаны в happens-before протокол (глава 7). «CAS прошёл — значит, я владел узлом» — утверждение, которое нужно **доказать**, а не надеяться. На неудаче CAS часто ставят `memory_order_relaxed` для чтения stale expected; на успехе — `acq_rel`, если через этот CAS вы «забираете» узел, связанный с другими полями.

Повторяющийся CAS-цикл под нагрузкой бьёт по одной cache line (глава 9): lock-free и false sharing — соседи по профилю.

```cpp
// Иллюстрация идеи, не production stack
bool try_pop(Node*& head, Node* expected) {
    Node* desired = expected ? expected->next : nullptr;
    return head.compare_exchange_strong(
        expected, desired,
        std::memory_order_acq_rel,
        std::memory_order_acquire);
}
```

## Проблема ABA (обзор)

**ABA:** поток 1 читает указатель head == A. Поток 2 снимает A, освобождает или кладёт в пул, меняет стек. Поток 3 (или снова 2) кладёт узел так, что head снова == A (тот же **адрес**, другое **содержимое**). Поток 1 делает CAS «A → следующий», думая, что никто не менял стек — инвариант разрушен.

Классические меры: **tagged pointer** (версия в старших битах), **hazard pointers**, **epoch-based reclamation**, **не переиспользовать узлы сразу**. В C++ ещё слой: когда `delete` узла безопасен, если другой поток мог ещё читать поле — UB.

ABA — причина, почему «скопировал lock-free код из блога» часто падает под нагрузкой. Для собеседования достаточно объяснить сценарий A-B-A и что CAS по «голому указателю» недостаточен без версии или отложенного free.

## Memory reclamation и гонки на lifetime

Lock-free **pop** отдаёт узел одному потоку; остальные не должны dereference после «отщипывания». Без схемы reclamation типичны use-after-free и double-free. Mutex-очередь проще: pop под lock, delete после выхода из секции, пока никто не держит указатель на элемент.

`std::shared_ptr` с atomic счётчиком — не волшебная lock-free очередь; contention на control block может быть хуже mutex. Архитектурные trade-offs, не dogma.

## Is «lock-free queue» in standard library?

В C++17 нет готовой lock-free `std::queue` в core. Есть `std::atomic` и иногда lock-free помеченные типы (`atomic_is_lock_free`). Контейнеры из `<concurrency>` **(C++26)** — за пределами базы репозитория. Промышленный код тянет Boost.Lockfree, folly, или свой модуль с code review уровня «доказательство».

Для учебника вывод простой: **mutex + cv** в репозитории — не «стыдный» выбор, а соответствующий уровень.

## Progress и starvation

Lock-free гарантирует, что **кто-то** продвинется, но **не** что продвинется **ваш** поток. При высокой contention CAS-цикл одного воркера может крутиться долго — starvation в практическом смысле. Wait-free устраняет это для операций, но ценой сложности.

Fair scheduling и work-stealing — отдельные темы (упомянем в томе 3 позже); lock-free deque для steal — уже graduate level.

## Helpful vs harmful — интуиция contention

Иногда различают **helpful** lock-free (CAS другого потока **помогает** общему прогрессу) и сценарии, где один thread starved. Для сервиса важнее tail latency под пиковой нагрузкой, чем красивое слово «lock-free» в README. Измеряйте p99 времени операции с mutex и без — на короткой критической секции mutex часто побеждает.

## Очередь Michael-Scott и «только для чтения главы 8»

Классическая M&S lock-free queue — два указателя head/tail, CAS на head при dequeue, CAS на tail при enqueue. Учебная ценность: увидеть, где release/acquire на указателях и **где** reclamation. Реализовывать её в упражнении репозитория не требуется; достаточно понимать, почему «голый указатель + CAS» без epoch/hazard — не production.

Bounded queue на mutex из следующих глав **не** lock-free, но **корректна** и предсказуема для pipeline — не стыдно выбрать её осознанно.

## Тестирование и sanitizer

Lock-free баги **редко** воспроизводятся. Нужны: stress-тесты, `-fsanitize=thread`, иногда model checking. На Fedora Clang/GCC TSan ловит data race на неatomic полях узла, если вы «забыли» atomic. Он **не** заменяет доказательство ABA.

## К собеседованию

Спросят: чем lock-free от wait-free; что такое ABA; зачем tagged pointer; почему pop lock-free stack опасен без reclamation. Хороший ответ признаёт, где mutex уместен в production.

## Типичные заблуждения

«Lock-free быстрее всегда» — contention и CAS retry могут быть медленнее короткого critical section. «Atomic = lock-free» — нет, `atomic<int>` ++ под spin — не структура данных. «ABA только на старых CPU» — нет, это логика указателей и reuse. «ABA решается double-checked locking» — DCL без правильной публикации — отдельный кошмар, не лекарство ABA.

Перед lock-free в своём проекте: схема памяти на бумаге, план reclamation, stress + TSan, code review. Иначе mutex честнее по стоимости сопровождения.

## Почему учебный репозиторий остаётся на mutex

`06_thread_pool`, `07_bounded_queue` сознательно на **mutex + cv**: доказуемый инвариант, понятный shutdown, join без hazard pointers. Lock-free M&S queue — отличный **читательский** материал, плохой «первый PR» без опыта reclamation. Книга не спойлерит эталоны из `solutions/`; достаточно знать, что **корректность** там проверяется тестами как спецификация, а не «lock-free ради моды».

Если профилировщик показал mutex как bottleneck **после** укорочения секции и layout (глава 9), тогда имеет смысл читать lock-free literature — не раньше.

## Progress guarantees и SLA

Lock-free обещает **системный** прогресс, не fairness. Для latency-sensitive сервисов иногда важнее bounded wait на mutex с OS-примитивами, чем CAS storm. Аргумент «lock-free = быстро» без p99 — marketing.

## Связь с atomics главы 5–7

Каждый успешный CAS — точка, где нужно вписать **release/acquire** или seq_cst в диаграмму HB. «Lock-free» не отменяет главы 6–7; она **усиливает** требование к доказательству.

## § К собесу (углубление)

- Назови три класса **progress** (wait-free, lock-free, obstruction-free).
- Почему **free** узла после pop опасен без epoch/hazard?
- Чем **ABA** связана с **memory reuse**?
- Когда промышленный код выбирает **mutex queue** осознанно?

## Когда spin, когда block

Lock-free часто крутит CAS в user space; mutex при contention уходит в futex sleep. При **низкой** contention CAS дешёв; при **высокой** все крутят один atomic — cache line bouncing (см. главу 9) + retry. Hybrid: spin N раз, потом block — политика runtime библиотек, не C++ core.

Для пула воркеров блокировка на cv при пустой очереди **экономит** CPU; spin-wait на «есть job» без cv сжигает квоты на Ryzen так же, как лишние потоки ОС.


## Глубже: Michael-Scott и границы учебника

Очередь M&S — два CAS на head/tail, **helping** других потоков продвинуть tail. Понимать **зачем** helping — полезно для чтения papers; писать с нуля — только с reclamation. Репозиторий `training` **не требует** этого уровня для зачёта.

## Work-stealing deque (упоминание)

Пул fork-join иногда использует steal у idle worker'а — lock-free deque с одним owner end и steal end. Это глава graduate; mutex queue в 06/07 — baseline.


## Мост к false sharing

CAS hot loop бьёт по одной cache line — глава 9. Lock-free без padding может быть **медленнее** mutex на короткой секции из-за contention line + retry.

## Когда статья про lock-free queue не для production

Признаки: нет раздела reclamation; «просто CAS на pointer»; нет TSan/stress; автор не упоминает ABA. Читать такие материалы полезно **как fiction для понимания CAS**, не как copy-paste.

В индustry mutex queue с короткой секцией часто **быстрее** и **дешевле** в сопровождении, чем lock-free с багами в reclamation.

## Практика на бумаге

Опиши ABA тремя предложениями для собеса.

Назови две причины остаться на mutex queue в учебном проекте.

## Краткое резюме

Lock-free — гарантия **системного** прогресса без mutex, цена — CAS, memory order, reclamation, ABA. Wait-free строже, реже. Для большинства прикладного кода mutex + чёткий инвариант выигрывает по стоимости сопровождения. Если лезете в lock-free — рисуйте happens-before, планируйте lifetime узлов, гоняйте TSan.

## Вопросы для самопроверки

1. Чем lock-free отличается от wait-free простыми словами?
2. Опишите сценарий ABA на стеке из трёх потоков.
3. Почему «CAS успешен» не означает «я единственный владелец памяти узла»?
4. Когда mutex для очереди разумен, даже если «хочется performance»?
5. Что TSan проверит в lock-free коде, а что останется на совести алгоритма?
6. Почему reclamation — часть алгоритма, а не «деталь allocator»?

## Обсуждение с репетитором

**Том:** 3 · **Глава:** `ch08` · **Файл:** `ch08-lock-free.md`

**Дальше по курсу:** [Глава 9. False sharing и кэш-линии](./ch09-false-sharing.md)
