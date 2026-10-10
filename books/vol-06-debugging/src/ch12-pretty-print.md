# Глава 12. Красивый вывод STL

## Зачем эта тема

`print my_vector` в голом GDB на libstdc++ без helpers показывает внутренние `_M_impl` — боль. **Pretty-printers** Python-скрипты в GDB рисуют `std::vector` как список элементов. VS Code часто включает `-enable-pretty-printing` автоматически.

## Почему без printer vector «страшен»

Внутренности libstdc++ — **implementation detail**: имена **`_M_start`**, **`_M_finish`** не стандарт C++, а договорённость конкретной версии GCC. Книга показывает их только как **fallback**, когда printer не подключён. На собеседовании достаточно сказать: «в GDB включу libstdc++ pretty-printers или посмотрю **`size`** / break после модификации».

## Включить в GDB

```
(gdb) source /usr/share/gcc/python/libstdcxx/v6/printers.py
(gdb) python from libstdcxx.v6.printers import register_libstdcxx_printers
(gdb) python register_libstdcxx_printers(gdb.current_objfile())
```

На Fedora путь может быть в `gdb-python` / пакете `gcc-c++`. Проще:

```
(gdb) set print pretty on
```

Это **не** то же самое, что STL printers, но улучшает struct.

Для постоянства — `~/.gdbinit`:

```
set print pretty on
```

## Ограничения

Шаблоны, custom allocator, `std::optional` **(C++17)** — зависит от версии printers. `std::function` внутри — не всегда красиво. Move-only типы в очереди пула — смотри поля вручную.

## Fedora: пакеты и версии

Если **`python register_libstdcxx_printers`** падает после **`dnf upgrade`**, проверь пару версий **`gcc`** и **`gdb`**. Иногда помогает переустановка **`gcc-c++`**. Запиши рабочий путь printers в личный `.gdbinit` — не коммить в **`training`**, это локальная машина.

## Альтернатива: printf в коде временно

Для учебного дебага «размер очереди» — `std::cerr << jobs.size()` под `#ifdef DEBUG` — не стыдно, если убрать после fix. Не оставляй в сдаче.

## watch на vector

`watch v` следит за объектом vector целиком — тяжело. Лучше break после `push` и `print size`.

## Типичные заблуждения

«pretty всегда включён» — в терминальном gdb нет. «вижу элементы = нет гонки» — нет.

## Краткое резюме

libstdc++ python printers, `set print pretty on`, `.gdbinit`. IDE часто включает сама. Сложные контейнеры — break + size. Дальше — Release.

## libstdcxx printers путь

Fedora: часто под **`/usr/share/gcc/python/libstdcxx/...`**. Пакет **`gcc-c++`**. Если **`python register_libstdcxx_printers`** падает — версия gdb python vs gcc.

## enable pretty-printing в MI

IDE шлёт при старте. Terminal gdb — вручную или `.gdbinit`:

```
python
import sys
sys.path.insert(0, '/usr/share/gcc/python')
from libstdcxx.v6.printers import register_libstdcxx_printers
register_libstdcxx_printers(gdb.current_objfile())
end
```

(Путь проверь на машине.)

## std::function и job queue

Closure type opaque — **`print job`** может быть бесполезен. Смотри **`jobs.size()`**, break **внутри** выполняемого job если знаешь тип задачи в тесте.

## vector без printer

**`print v.size()`** безопасно. Raw **`_M_start`** — хрупко между версиями libstdc++.

## optional, variant **(C++17)**

Printers эволюционируют; fallback — **`has_value`**, **`index`**.

## Не путать pretty с correctness

Красивый **`vector`** не значит thread-safe read без lock.

## § К собесу

Pretty-printers = Python pretty visualization in GDB.

## boost printers

Not in course; only stdlib.

## custom pretty printer

You can write Python for your **`Order`** type — overkill for training.

## print map/set

Printers help; large containers slow GDB.

## move semantics in print

Moved-from object state may show empty — not always bug during debug.

## queue of move-only

**`std::queue<std::unique_ptr<T>>`** — size only, not elements without deep hack.

## verify printer loaded

**`info pretty-printer`** — list active.

## string as vector of char

Without printer **`print s[0]`** may fail — use **`c_str`**.

## deque queue adapter

Queue hides deque — **`print q`** opaque; **`size`** if API exposed.

## gdb 14 / gcc 14

Printers track releases; after **`dnf upgrade`** retest one **`print vector`**.

## disable pretty

**`set print pretty off`** — compare raw when confused.

## Дополнительный сценарий: проверка printers

После настройки printers создай локальный tiny `.cpp` с контейнером **`std::vector`** из нескольких `int` — **вне** git repo или в scratch — **`print v`** на breakpoint после push. Один раз увидеть `{0, 1, 2}` в GDB успокаивает. В markdown книги избегаем «голых» угловых скобок вне code fence — в терминале код обычный C++.

## Когда pretty врёт

Moved-from **`std::string`** может выглядеть пустым корректно; moved-from **`vector`** — size 0. Не путай с логической ошибкой в очереди.

## Практическое закрепление (5–10 минут)

Открой Debug-сборку любого **зелёного** target из `multithread/build/`. Запусти GDB, поставь **`break main`**, **`run`**, выполни **`next`** пять раз, **`bt`**, **`info locals`**, **`quit`**. Повтори на другом target — muscle memory важнее заучивания глав. Если что-то не совпало с текстом главы, проверь cwd и путь к exe: nine из десяти «GDB сломан» — неверный путь или сборка без **`-g`**.

## Связь с главой тома 3 без спойлера кода

Вернись мысленно к инварианту упражнения, которое отлаживаешь: что должно быть истинно перед выходом из **`wait`**, когда очередь считается пустой, когда **`submit`** должен молчать. GDB показывает **момент**, когда инвариант уже нарушен или ещё не достигнут. Исправление остаётся в голове и в **`task.cpp`**, не в команде **`continue`**.

## § Коротко к собесу (повтор)

Сформулируй вслух одним предложением главную мысль этой главы и назови **одну** команду GDB, без которой тема не работает. Если не можешь — перечитай «Краткое резюме» и вернись сюда завтра; spaced repetition лучше зубрёжки за вечер.

## Вопросы для самопроверки

1. Почему vector показывает `_M_start`?
2. Где включить pretty в VS Code launch?
3. Что положить в `.gdbinit`?
4. Почему `print` очереди `function` трудён?
5. Заменяет ли pretty TSan?

## Обсуждение с репетитором

**Том:** 6 · **Глава:** `ch12` · **Файл:** `ch12-pretty-print.md`

**Дальше по курсу:** [Глава 13. Оптимизированный код](./ch13-optimized-code.md)
