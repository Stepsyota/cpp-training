# Глава 4. UndefinedBehaviorSanitizer

## Зачем эта тема

Не всякий UB — обращение к «чужой» памяти. Сдвиг на 64 бита, signed overflow, null dereference, неверный `reinterpret_cast` — **UndefinedBehaviorSanitizer (UBSan)** ловит во время выполнения с коротким сообщением.

## Включение

```bash
g++ -std=c++17 -g -O1 -fsanitize=undefined -fno-sanitize-recover=undefined \
    file.cpp -o app
```

**`-fno-sanitize-recover=undefined`** — не продолжать после первого UB (удобно в CI). Без него может быть warning и программа идёт дальше — опасно для обучения.

Часто комбинируют с ASan:

```bash
-fsanitize=address,undefined
```

(на link те же флаги.)

## Примеры UB для курса

- `int x = INT_MAX; x+1` signed overflow.
- `1 << 40` для 32-bit int shift.
- Разыменование `nullptr`.
- Нарушение `dynamic_cast` / vptr (редко в multithread).
- **Strict aliasing** — `float` через `int*` (том 1, casts).

UBSan **не заменяет** здравый смысл типов; он ловит то, что **выполнилось** на этом прогоне.

## UBSan и потоки

UB в одном потоке — UBSan. Data race — **не** UBSan, а TSan. Оба могут понадобиться на разных сборках.

## float и -ffast-math

С `-ffast-math` компилятор считает, что NaN/inf не возникают — UBSan и реальность расходятся. В учебном коде не включай fast-math без причины (том 5).

## Подвиды UBSan (имена)

GCC/Clang: `undefined`, optional `integer`, `nullability`, `alignment`, `bounds` — версии differ. `-fsanitize=undefined` bundle — check compiler doc on Fedora.

## enum class и cast

`static_cast` to invalid enum — may UBSan at runtime if value used in switch without default.

## Division

Integer `/ 0` — UBSan. Float — not UB same way.

## bool in arithmetic

Legacy C++ patterns — rare UBSan; prefer explicit.

## constexpr vs runtime UBSan

Compile-time failure in `constexpr` context — no runtime UBSan needed. Runtime path — UBSan catches.

## multithread

Race on int — TSan. Signed overflow in one thread math — UBSan. Both builds if paranoia on numeric code in 08 fn.

## Recover in dev

Some teams leave recover on locally — course prefers **abort** to learn stop-at-first-UB.

## Типичные заблуждения

«UBSan ловит все UB стандарта» — только instrumented subset. «Компилятор с -Wall = нет UB» — нет. «UBSan вместо TSan» — разные вещи.

## UB без «сломанной» памяти

Не всякий UB — обращение к чужому адресу. **Signed integer overflow**, **shift exponent ≥ width**, **misaligned atomic**, **invalid enum** value, **null deref** — UBSan вставляет проверки **вокруг** операции. Отчёт короче ASan, но тоже с `#0` в твоём коде.

## Строгий режим recover

```bash
-fsanitize=undefined -fno-sanitize-recover=undefined
```

Без **recover** UBSan может напечатать warning и **продолжить** — для обучения плохо: программа уже в UB-territory. В CI-идее (том 5) recover off = fail fast.

## Комбинация address,undefined

```bash
g++ -std=c++17 -g -O1 -fsanitize=address,undefined     -fno-sanitize-recover=undefined file.cpp -o app
```

Один прогон: память + часть UB. **Link** с тем же списком. TSan **не** добавляй (глава 8).

## Shift и overflow — примеры

```cpp
int x = 1;
int y = x << 33;   // shift UB на 32-bit int; на int32 — UBSan
int z = INT_MAX; z + 1;  // signed overflow
```

На amd64 `int` часто 32-bit — shift 33 ловится. Проверь на своей машине с UBSan.

## Null and vptr

`*static_cast<int*>(nullptr) = 1` — UBSan. Виртуальный вызов через destroyed object — сложнее; в multithread редко.

## Strict aliasing

Чтение `float` через `int*` — UB (том 1 casts). UBSan `-fsanitize=undefined` включает **-fsanitize=alignment**, **bounds** subsets на разных версиях — смотри `help=1` для GCC/Clang (глава 10).

## UBSan и потоки

UB в **одном** потоке — UBSan. **Data race** на plain `int` — **TSan**, не UBSan. Оба build dir на одном проекте — норма.

## -ffast-math

Tom 5: `-ffast-math` ломает предположения о NaN/inf — UBSan и реальность расходятся. В учебном коде без fast-math.

## Float divide by zero

UBSan ловит **integer** divide by zero; float divide by zero — IEEE, не UBSan topic.

## Когда UBSan молчит

UB, которое **не** instrumented (например некоторые forms of ODR violation) — link time или `-Wall` не помогут. **Time travel** UB — не ловится.

## § К собесу

UBSan subset; recover off; не TSan; часто с ASan; shift/overflow/null — типичные примеры.

<!-- vol7-pass5-ch04 -->

## Дополнительное закрепление

Эксперiment shift в `/tmp` с UBSan. **build-asan** = address+undefined одним configure.

`-fno-sanitize-recover=undefined` — stop on first UB в обучении.

Tom 1 casts: prefer safe casts; UBSan ловит часть нарушений.

## Краткое резюме

`-fsanitize=undefined`, recover off для строгости. Shift, overflow, null, aliasing. Часто вместе с ASan. Гонки — TSan. Дальше — TSan.

## Таблица «кто ловит»

| Симптом | ASan | UBSan | TSan |
|---------|------|-------|------|
| Out of bounds | да | иногда bounds | нет |
| Signed overflow | нет | да | нет |
| Data race | иногда corruption | нет | да |
| null deref | часто | да | n/a |

## -fsanitize-recover=all

Не используй в обучении без причины — маскирует цепочку UB.

## tom 1 casts

`reinterpret_cast` footguns — UBSan may fire; prefer `static_cast` discipline from tom 1.

<!-- vol7-pass6 -->
## Строка для памяти (Kindle)

Перед сном одним взглядом: **compile+link**, **-g -O1**, **TSan+pthread**, **два build dir**, **два стека в отчёте**, **solutions не спойлерить**. Завтра — один TSan-прогон на 02 или на последнем изменённом advanced target.

## Вопросы для самопроверки

1. Флаги для строгого UBSan?
2. Поймает ли UBSan `++` на гонящемся `int`?
3. Зачем `address,undefined` вместе?
4. Пример shift UB?
5. Влияние `-ffast-math`?

<!-- vol7-pass4-ch04 -->
## UBSan в жизни multithread

Большинство упражнений **не** завязаны на signed overflow — UBSan всё равно полезен в **build-asan** рядом с ASan: один прогон, два класса багов. Если в `08` функция map делает с shift/hash с int — следи за width.

**-fno-sanitize-recover=undefined** — приучи себя к **stop on first UB**. Продолжать после UBSan warning — тренировать нервы не нужно; программа уже в UB.

На CI tom 5: compile job с `-Werror` и run job с UBSan recover off — разные стадии; tom 7 связывает **runtime** стадию с sanitizer flags из главы 9.

## Обсуждение с репетитором

**Том:** 7 · **Глава:** `ch04` · **Файл:** `ch04-ubsan.md`

**Дальше по курсу:** [Глава 5. ThreadSanitizer](./ch05-thread-sanitizer.md)
