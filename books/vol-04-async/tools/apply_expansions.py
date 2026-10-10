#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Insert per-chapter expansion blocks before ## Краткое резюме."""
from __future__ import annotations

import pathlib

SRC = pathlib.Path(__file__).resolve().parents[1] / "src"
MARKER = "## Краткое резюме"
INSERT_HEAD = "## Развёрнутый разбор"

# Each value: markdown body (without the heading); inserted once per file.
EXPANSIONS: dict[str, str] = {}

def P(*paragraphs: str) -> str:
    return "\n\n".join(paragraphs)

EXPANSIONS["ch02-std-async.md"] = P(
    "Функция `std::async` — шаблон в `<future>`. Компилятор выводит тип результата из callable так же, как для `std::thread` и `std::invoke_result_t` в parallel_map (том 3): return callable становится типом `future`. Если callable возвращает `void`, получишь `future<void>` — `get()` только синхронизирует завершение.",
    "Первый аргумент — политика `std::launch`. В C++17 это битовая маска: `async`, `deferred`, или OR обоих. **Не** передавать политику — значит «реализация решает». На Fedora с libstdc++ это не абстракция: поведение влияет на тесты oracle и на профилирование.",
    "### Oracle без спойлера решения",
    "README **`03_oracle`** формулирует три условия: вернуть то, что вернула `question`; выполнить её **не** в вызывающем потоке; не вернуться раньше конца работы. `std::async(std::launch::async, question)` (или эквивалент с аргументами) запускает работу; `.get()` на future блокирует до готовности и возвращает значение. §6 **`THEORY.md`** показывает ту же однострочную идею и объясняет, почему без `launch::async` id потоков может совпасть.",
    "Альтернатива из README — `promise`/`future` + `std::thread`: promise уходит в worker, future остаётся у caller, `set_value` с return `question`, снаружи `get()` и `join`. Больше строк, тот же контракт. Oracle не заставляет выбирать — обучает оба пути.",
    "### Deferred по шагам",
    "При `std::launch::deferred` тело callable **не** обязано выполниться в момент вызова `async`. Оно выполняется, когда кто-то на связанном future вызовет `wait` или `get` — **в том потоке**, который вызвал wait/get. Параллелизма нет: это ленивое вычисление с блокировкой на точке ожидания. Полезно для редких тяжёлых функций, которые могут и не понадобиться; вредно, если ты **ждал** другой поток для разгрузки UI.",
    "`wait_for` на deferred future до первого `get` может вернуть `deferred`, а не `ready` — см. главу 3. Путаница deferred/async — частая причина «почему в gdb один поток».",
    "### Несколько async и порядок get",
    "Идиома «запусти все, потом забери все» сохраняет параллелизм:",
    "```cpp",
    "auto fa = std::async(std::launch::async, work_a);",
    "auto fb = std::async(std::launch::async, work_b);",
    "auto ra = fa.get();",
    "auto rb = fb.get();",
    "```",
    "Если между `async` и `get` вставить другую длинную работу в main, это не отменяет фоновые задачи — но откладывает момент, когда ты забираешь результат. Если второй `async` стоит **после** `get` первого, второй стартует позже — иногда так и задумано (цепочка зависимостей), иногда это ошибка планирования.",
    "### Ограничения и executor",
    "В C++17 у `async` **нет** параметра «выполни на пуле X». Каждый вызов с `launch::async` — потенциально новый поток ОС (глава 7). Пул из тома 3 — **ручной executor**: ты сам кладёшь job. `packaged_task` (глава 5) — мост «future + выбор момента вызова».",
    "### Fedora / GCC / Clang",
    "Сборка как у упражнений: `-std=c++17 -pthread`. Заголовок `<future>` тянет threading support. Для отладки oracle: `cmake --build build --target 03_oracle`, бreakpoint на `get()` и сравнение `std::this_thread::get_id()` — том 6. Не полагайся на «libstdc++ всегда создаёт поток» без `launch::async` — стандарт этого не обещает.",
    "### К собесу",
    "Объясни default policy и почему oracle требует `async`. Чем `deferred` отличается по потоку выполнения. Почему `async` не заменяет thread pool при 10k задач.",
)

EXPANSIONS["ch03-future-shared.md"] = P(
    "`std::future<T>` — move-only handle читателя. Копирование запрещено, чтобы **один** поток забрал значение. Это отличает future от многих других типов STL и от `shared_future`.",
    "После move `std::move(fut)` старый future пуст: `valid()` false. В контейнерах `vector<future<int>>` при emplace_back и последующем move следи, какой элемент уже опустошён — use-after-move в цикле типичная ошибка новичка.",
    "### wait и get для void",
    "Для `future<void>` `wait()` ждёт завершения без «значения». `get()` тоже ждёт и **потребляет** future; при исключении в задаче — rethrow. Паттерн «только дождаться» — `wait` или `get` для void; разница в том, что `get` для void всё равно помечает future использованным.",
    "### wait_for и polling",
    "Цикл с `wait_for(0ms)` или коротким timeout — способ не блокировать UI или worker навечно. Это **не отмена** задачи: фоновый `async` с `launch::async` продолжает крутиться. Отмена в C++17 — отдельный протокол (флаг, закрытие сокета, **(C++20)** stop_token в других API).",
    "`future_status::deferred` напоминает: задача может ещё не стартовать. Не трактуй timeout как «ошибку сети» без контекста политики запуска.",
    "### shared_future семантика",
    "`share()` или конструирование `shared_future` из `future` переводит handle в режим нескольких читателей. Каждый `get()` возвращает **копию** `T` (для `T` copyable) или move для rvalue — все видят одно и то же опубликованное значение. Исключение при `get()` на shared_future получают все читатели одинаково.",
    "Use case: один раз загрузили конфиг, несколько worker’ов вызывают `sf.get()` и стартуют с одной версии. Anti-case: очередь задач — каждому нужен **свой** элемент; это `BoundedQueue`, не shared_future.",
    "### Связь с condition_variable",
    "CV + mutex: «жди, пока pred()». Future: «жди, пока положат **одно** T». Узкий контракт future проще рассуждать, но не заменяет очередь. В пуле (том 3) cv будит worker’ов при появлении job; future на каждую мелкую job — overhead.",
    "### happens-before",
    "Стандарт гарантирует синхронизацию между публикацией результата в shared state и успешным `get`/`wait`. Это **не** распространяется на произвольные данные, которые worker писал в глобальные без mutex параллельно с публикацией. Oracle обычно не делит состояние между потоками кроме канала результата — идеальный учебный случай.",
    "### К собесу",
    "Почему future move-only. Когда shared_future. Что после второго get на обычном future.",
)

exec((pathlib.Path(__file__).parent / "expansions_rest.py").read_text(encoding="utf-8"), globals())

def apply():
    for name, body in EXPANSIONS.items():
        path = SRC / name
        if not path.exists():
            print("skip missing", name)
            continue
        text = path.read_text(encoding="utf-8")
        tag = f"<!-- expand:{name} -->"
        if tag in text:
            print("already", name)
            continue
        if MARKER not in text:
            print("no marker", name)
            continue
        head, tail = text.split(MARKER, 1)
        block = f"{tag}\n{INSERT_HEAD}\n\n{body.strip()}\n\n"
        path.write_text(head + block + MARKER + tail, encoding="utf-8")
        print("ok", name, "->", len(path.read_text(encoding="utf-8").splitlines()), "lines")

if __name__ == "__main__":
    apply()
