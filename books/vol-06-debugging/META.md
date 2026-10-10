# META — том 6

| Поле | Значение |
|------|----------|
| **Цель** | Уверенно отлаживать C++17 на Fedora: GDB, потоки, зависания, core, cgdb, IDE |
| **Связь** | Том 5 (`-g`, Debug); том 3/4 (mutex, future); том 7 (TSan после GDB) |
| **Статус** | **расширенное издание** (2026-10-10): introduction + ch01–ch14, ~2600+ строк md |
| **Сборка** | `mdbook build` — OK (HTML + EPUB) |

## Оглавление

| ch | Файл | Тема |
|----|------|------|
| 01 | ch01-debug-mindset | Зачем отладчик, символы, Debug-сборка |
| 02 | ch02-gdb-start | Запуск, run, аргументы, quit |
| 03 | ch03-breakpoints | break, disable, conditions, watch |
| 04 | ch04-stepping | step, next, finish, until |
| 05 | ch05-inspect-data | print, display, locals, types |
| 06 | ch06-backtrace | bt, frame, up/down |
| 07 | ch07-threads-gdb | info threads, thread, all bt |
| 08 | ch08-deadlocks-hangs | Пул, cv, future.get |
| 09 | ch09-core-dumps | ulimit, core, post-mortem |
| 10 | ch10-cgdb | TUI, cgdb на Fedora |
| 11 | ch11-ide-debugger | launch.json, CMake Debug |
| 12 | ch12-pretty-print | STL в GDB, ограничения |
| 13 | ch13-optimized-code | -O2, inline, «variable optimized out» |
| 14 | ch14-summary-myths | Итог, к собесу, том 7 |

## Объём (src/*.md)

Уточняй: `wc -l books/vol-06-debugging/src/*.md` — цель серии ~2600–2800 строк на том (~180–200 на главу).

## Открытые вопросы

- Вынести общий `.gdbinit` пример в приложение — по желанию хозяина.
- Remote `gdbserver` — вне scope тома.
