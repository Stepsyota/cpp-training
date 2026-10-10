# Глава 3. Точки останова

## Зачем эта тема

Останов «на интересной строке» — основа отладки. Без breakpoints ты либо бежишь до конца, либо ловишь только crash. В многопоточном коде breakpoint в воркере срабатывает **много раз** — нужны условия и отключение лишнего.

## Стратегия «узкая точка»

Не ставь breakpoint на строку, которую проходят миллион раз в секунду, без **`condition`** или **`ignore`** — сеанс станет непереносимым. В пуле лучше break на **`submit`** при **`stopped==true`**, на **`pop`** при **`closed`**, чем на каждой итерации цикла worker. Думай как о фильтре логов: только события, которые **меняют** гипотезу.

## Поставить breakpoint

```
(gdb) break main
(gdb) break ticket_office.cpp:25
(gdb) break ThreadPool::submit
```

Сокращение **`b`**. По функции GDB ищет символ; для шаблонов и overload может спросить номер — выбери нужный.

Список:

```
(gdb) info breakpoints
```

Номер **`disable 2`**, **`enable 2`**, **`delete 2`**.

## Условный breakpoint

```
(gdb) break bounded_queue.hpp:40 if closed == true
```

Срабатывает только когда условие истинно. Для счётчика в цикле:

```
(gdb) break task.cpp:18 if i == 999
```

Уменьшает шум в hot loop.

## Временный breakpoint

**`tbreak`** — один раз, потом сам удаляется. Удобно для «остановись при первом входе в submit».

## Watchpoint (на переменную)

```
(gdb) watch counter
```

Останов при **изменении** памяти. Медленнее breakpoints; на глобальных в многопоточности шумно — лучше conditional break.

## continue

После остановки:

```
(gdb) continue
```

**`c`** — бежать до следующего breakpoint или конца.

## Breakpoint на исключении (C++)

```
(gdb) catch throw
```

Останов при throw (нужна поддержка libstdc++ debug symbols). Полезно, если тест падает с исключением из пула.

## Практика на курсе

`02_ticket_office`: break на строке с `++` без mutex — смотри значение счётчика до/после. `06`: break в теле воркера — сколько раз заходим, пуста ли очередь (осторожно: другие потоки меняют state).

## Типичные заблуждения

«breakpoint = все потоки стоят» — не обязательно. «watch на mutex» — ловит любой lock. «break на номере строки без пересборки» — номера устарели.

## Краткое резюме

`break` по функции/файлу:строка, `condition`, `tbreak`, `watch`, `continue`. Управляй списком через `info breakpoints`. Дальше — шагание.

## Номера breakpoint и команды к ним

**`info breakpoints`** показывает Num, Type, Disp, Enb, Address, What. **`commands Num`** — задать список команд при срабатывании (без echo «Continuing»):

```
(gdb) commands 1
> silent
> info threads
> continue
> end
```

**`silent`** убирает лишний шум. Осторожно: бесконечный **`continue`** в hot path замедляет до ползущего.

## ignore и temporary

**`ignore 1 50`** — следующие 50 попаданий на breakpoint 1 игнорировать. Удобно, когда воркер крутится в цикле тысячи раз. **`tbreak`** — одно срабатывание.

## Breakpoint на shared library

Пока не central для static exe multithread; если линкуешь `.so`, **`break foo`** после load или **`set breakpoint pending on`**. Pending — breakpoint «ожидает», пока библиотека не загрузится.

## Ловушки catch

**`catch throw`** — C++ exception. **`catch syscall`** — редко. Для пула, если **`job()`** бросает, поймаешь в throw до terminate. Нужны символы libstdc++.

## Условия и типичные ошибки синтаксиса

Условие — выражение на C++ в контексте frame. Обращение к полям класса: иногда нужен **`this->`**. Если условие всегда false — «breakpoint не срабатывает»; проверь **`info break`**. Имена из другого потока в условии — опасно: читаешь без sync.

## Практика: elevator

**`04_elevator`**: break на **`wait`** (строка в твоём task). **`continue`** — дождись второго потока. **`thread apply all bt`** — кто держит mutex, кто спит. Breakpoint не заменяет понимание предиката (том 3), но показывает **географию** ожидания.

## Практика: bounded queue

**`07`**: conditional **`break pop if closed`** (имена полей — твои). Смотри, не срабатывает ли pop «рано» при **`closed && empty`** — логика из README, не из solutions.

## Hardware watchpoints

**`watch`** использует ограниченные регистры CPU; на больших структурах может не влезть. **`rwatch`** — read, **`awatch`** — read/write. В многопоточности watch на **`counter`** без mutex — срабатывания от всех потоков.

## § К собесу

Спроси себя: чем **software breakpoint** (INT3) отличается от **watchpoint** (hardware). Достаточно «watch ловит запись в память, break — исполнение адреса».

## break на строку vs функцию

**`break foo`** ставит на entry. **`break foo if argc>1`** — conditional entry. Для member function **`break Class::method`**. Overload — GDB list variants with numbers; **`break 2`**.

## save breakpoints

**`save break breakpoints.txt`** / **`source breakpoints.txt`** — share сценарий с репетитором без кода fix.

## clear vs delete

**`clear line`** удаляет break на строке. **`delete num`** — по номеру. **`disable`** — временно.

## tracepoints (упоминание)

**`trace`** — без полной stop, запись данных; advanced, не курс. Знай, что существует для high-frequency events.

## breakpoint на template instantiation

Шумно в headers. Лучше break в **.cpp** instantiation site или в user code only.

## Практика: highscore CAS

Break на **`compare_exchange`** failure path (твоя строка). Смотри expected vs current — учебная диагностика atomic, не спойлер эталона.

## Практика: mutex lock line

Break **после** успешного lock, **`print` counter**, **`continue`**. Видишь interleaving без print в коде — но помни про scheduler effects.

## rbreak regex

**`rbreak ^ThreadPool::`** — все методы класса; может быть много — смотри **`info break`**.

## break in loaded shared object

Static exe курса — не актуально.

## hit count

**`info break`** показывает счётчик срабатываний — видишь, hot ли точка.

## breakpoint на main до run

Можно **`break main`** до первого **`run`** — GDB запомнит pending on future load for PIE — на Fedora PIE default, GDB handles.

## Практическое закрепление (5–10 минут)

Открой Debug-сборку любого **зелёного** target из `multithread/build/`. Запусти GDB, поставь **`break main`**, **`run`**, выполни **`next`** пять раз, **`bt`**, **`info locals`**, **`quit`**. Повтори на другом target — muscle memory важнее заучивания глав. Если что-то не совпало с текстом главы, проверь cwd и путь к exe: nine из десяти «GDB сломан» — неверный путь или сборка без **`-g`**.

## Связь с главой тома 3 без спойлера кода

Вернись мысленно к инварианту упражнения, которое отлаживаешь: что должно быть истинно перед выходом из **`wait`**, когда очередь считается пустой, когда **`submit`** должен молчать. GDB показывает **момент**, когда инвариант уже нарушен или ещё не достигнут. Исправление остаётся в голове и в **`task.cpp`**, не в команде **`continue`**.

## § Коротко к собесу (повтор)

Сформулируй вслух одним предложением главную мысль этой главы и назови **одну** команду GDB, без которой тема не работает. Если не можешь — перечитай «Краткое резюме» и вернись сюда завтра; spaced repetition лучше зубрёжки за вечер.

## Вопросы для самопроверки

1. Как остановиться только при `i == 0`?
2. Чем `tbreak` отличается от `break`?
3. Зачем `continue` после осмотра locals?
4. Почему watch на shared counter шумит в threads?
5. Как удалить breakpoint №3?

## Обсуждение с репетитором

**Том:** 6 · **Глава:** `ch03` · **Файл:** `ch03-breakpoints.md`

**Дальше по курсу:** [Глава 4. Шагание по коду](./ch04-stepping.md)
