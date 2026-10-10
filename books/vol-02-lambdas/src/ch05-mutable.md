# Глава 5. mutable в лямбде

## Зачем эта тема

По умолчанию `operator()` у лямбды — **const member function**: как const-метод класса, он не должен менять **поля** closure, захваченные по значению. Но иногда нужен **счётчик вызовов**, одноразовый флаг в predicate, кэш или «ленивое» поле внутри «логически const» callable. Keyword **`mutable`** на лямбде снимает const с `operator()`, позволяя менять **копии** захваченных по значению переменных между вызовами.

Это тот же **`mutable`**, что в томе 1 у полей класса в const-методах — только применённый к **сгенерированному** closure type и стоящий в другой позиции грамматики (после `)` параметров). Путаница с **mutex** в mutable полях класса (том 1) и с **mutable лямбдой** в многопоточности — отдельная осторожность: mutable не даёт синхронизации и не делает поля atomic.

## Синтаксис и const по умолчанию

```cpp
int outside = 0;
auto f = [n = 0]() mutable {
    return ++n;
};
f(); f();
// outside не менялся; n внутри closure == 2
```

Без `mutable` строка `++n` в const `operator()` была бы ошибкой компиляции: «cannot assign to capture in const lambda» (формулировка зависит от компилятора).

С **`mutable`** сгенерированный `operator()` становится **не-const** с точки зрения полей closure. Снаружи объект лямбды по-прежнему можно вызывать на const объекте лямбды в смысле «вызов не требует mutating the variable f» — const-ness относится к **полям внутри**, как у const vs non-const method.

## mutable и захват по ссылке

```cpp
int x = 0;
auto g = [&x]() mutable {
    ++x;  // меняет внешний x через ссылку
};
```

`mutable` на лямбде **не** превращает ссылку в копию: через `&x` по-прежнему меняется **внешний** объект. `mutable` здесь избыточен для компиляции `++x`, но иногда его оставляют по привычке — не ошибка.

`mutable` в основном про поля, которые являются **копиями** в closure и должны меняться между вызовами без изменения внешних переменных.

## Зачем это в STL-подобном коде

Предикат, который «запоминает», что уже видел элемент (учебный, не production):

```cpp
auto once = [seen = false]() mutable {
    if (seen) return false;
    seen = true;
    return true;
};
```

В реальном коде состояние чаще выносят наружу (`int&`, atomic) или используют алгоритм без памяти; пример показывает **механику** mutable.

Другой сценарий: closure как **мини-state machine** в тесте — считать вызовы mock-callback без глобальной переменной.

## mutable vs обычный функтор

```cpp
struct Counter {
    int n = 0;
    int operator()() { return ++n; }  // не const operator()
};
```

Лямбда с `mutable` генерирует не-const `operator()`; без `mutable` — const, если нет особых правил capture. Эквивалентность с функтором полная: keyword просто выбирает const-ность сгенерированного метода.

## mutable и const member function снаружи

Вызов `f()` на объекте лямбды не требует, чтобы переменная `f` была non-const: `operator()` const на closure object по умолчанию означает «не менять capture members»; `mutable` меняет **внутреннюю** const-ность метода. Снаружи `const auto f = [...] mutable {}` всё ещё можно вызывать — как const object с non-const method... На практике лямбды редко объявляют const на переменной; проще `auto`.

## Логическая const vs физическая (параллель с томом 1)

Predicate для `find_if` «логически» только читает контейнер, но может держать mutable счётчик «сколько раз вызвали» для отладки — `mutable` + capture by value. Для production такой side effect в predicate — code smell; алгоритм не гарантирует число вызовов для всех iterator categories.

## Комбинация mutable и init-capture

```cpp
auto acc = [sum = 0]() mutable {
    return ++sum;
};
```

Каждый вызов на **одном** объекте `acc` увеличивает поле. Две копии `acc` — два независимых счётчика. Это как два экземпляра struct Counter.

## Связь с const correctness

Снаружи клиент может держать `const auto callback = [...] mutable {...}` — вызовы разрешены, mutable внутри. Это редкий стиль; чаще `auto` без const на closure.

## Многопоточность

Два потока вызывают **одну и ту же** лямбду с `mutable` и общим полем (или ссылкой на общий `int`) без mutex — **data race**. `mutable` не про потоки; том 3 разберёт atomics. Для счётчика в parallel algorithm используют `std::atomic` или внешний sync, не `mutable int` в shared closure.

Если каждый поток имеет **свою копию** closure (копия задачи из очереди), mutable поля не shared — гонки нет между потоками, только внутри одного объекта при повторных вызовах в одном потоке.

## Связь с томом 1

Keyword **`mutable`** на полях class — глава 4 тома 1 (кэш в const-методе). Здесь тот же keyword в позиции **lambda specifiers**. Оба случая — «логически не меняем смысл, физически трогаем служебное поле».

## К собесу

«Зачем mutable в лямбде?» — const `operator()` по умолчанию; нужно менять capture-by-value. «Меняет ли mutable захват by reference?» — нет, ссылка как ссылка. «Thread-safe?» — нет.

## std::generate и mutable state

```cpp
int seed = 0;
std::generate(v.begin(), v.end(), [seed]() mutable { return ++seed; });
```

`generate` копирует callable или вызывает на одном объекте — зависит от implementation; предполагай один объект generator, mutable seed работает. Не полагайся на generate для parallel без sync.

## Различие mutable closure vs mutable field класса

В классе `mutable int n` в const method — поле. В лямбде `mutable` — весь `operator()`. Keyword одно, механика разная позиция в grammar — том 1 + том 2 вместе.

## Thread-local vs mutable lambda

`thread_local` in lambda body different from mutable member — don't confuse storage duration with mutable.

## Типичные заблуждения

«mutable делает лямбду thread-safe» — нет. «Без mutable нельзя менять захваченный int» — можно, если захват по ссылке `&`. «mutable нужен для любого side effect» — side effect на globals не требует mutable. «mutable = как mutable поле класса» — по духу да, синтаксис другой.

## Predicate «ровно один true» anti-pattern

Использовать mutable bool в predicate для `remove_if` опасно: algorithm может вызывать predicate произвольное число раз для элементов. State in predicate — только если algorithm guarantees single pass и ты прочитал standard.

## const correctness в lambda-heavy code

Большие лямбды с mutable полями усложняют reasoning. Если state растёт — вынеси struct.

## Том 1 cross-link

Перечитай параграф про mutable cache в const method — parallel с mutable lambda field.

## Краткое резюме

`mutable` снимает const с `operator()` лямбды; поля-копии можно менять между вызовами. Ссылки ведут себя как ссылки. Не замена синхронизации. Аналог не-const `operator()` у функтора.

## Параграф для Kindle (закрепление)

Перечитай ключевую мысль главы без кода: запиши одно предложение от руки. Для этой главы сформулируй, **что** должно остаться в памяти после чтения, и **какую ошибку** ты больше не хочешь повторять в своём `.cpp`. Связь с треком multithread — только теория; код упражнений по-прежнему твой.

## Вопросы для самопроверки

1. Что сломается без `mutable` при `++n` для захваченного по значению `n`?
2. Нужен ли `mutable` для `++x` при захвате `[&x]`?
3. Какой аналог mutable у обычного struct-функтора?
4. Безопасно ли вызывать mutable-лямбду из двух потоков на одном объекте?
5. Связь с `mutable` полем в const-методе класса?
6. Меняет ли `mutable` внешнюю переменную при capture by value?
7. Const-ness какого метода задаёт `mutable`?

## Обсуждение с репетитором

**Том:** 2 · **Глава:** `ch05` · **Файл:** `ch05-mutable.md`

**Дальше по курсу:** [Глава 6. Generic lambda](./ch06-generic-lambdas.md)
