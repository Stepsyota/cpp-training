# META — том 3

| Поле | Значение |
|------|----------|
| **Цель** | Учебник по concurrency в C++17 + пометки C++20; модель памяти R+P+I |
| **Связь** | Расширяет `multithread/THEORY.md`; упражнения 01–08 — практика |
| **Стандарт** | C++17; C++20 (`jthread`, latch…) — с бейджами |
| **Статус** | расширенное издание (~2600+ строк md, 14 глав ≈ 170–190 строк/глава + введение); `mdbook build` зелёный |
| **Объём (2026-10-10)** | см. `wc -l src/*.md` в каталоге тома; цель серии — уровень тома 1 |

## Оглавление

| ch | Файл | Тема |
|----|------|------|
| 01 | ch01-threads-and-cpu | Потоки ОС, ядра, SMT, `std::thread` |
| 02 | ch02-data-races | Data race, UB, happens-before введение |
| 03 | ch03-mutexes | mutex, lock_guard, unique_lock, scoped_lock |
| 04 | ch04-condition-variable | cv, предикат, lost wakeup |
| 05 | ch05-atomics | atomic, CAS, счётчики |
| 06 | ch06-memory-orders | relaxed, acquire, release, acq_rel, seq_cst |
| 07 | ch07-happens-before | synchronizes-with, цепочки |
| 08 | ch08-lock-free | Идея lock-free, ABA обзор |
| 09 | ch09-false-sharing | Cache line, padding |
| 10 | ch10-thread-pool | Паттерн пула (теория) |
| 11 | ch11-bounded-queue | Producer–consumer, close |
| 12 | ch12-parallel-stl | Execution policies **(C++17)** |
| 13 | ch13-cpp20-sync | jthread, stop_token, latch, barrier, semaphore |
| 14 | ch14-debugging-races | TSan обзор → том 7, чеклист |

## Открытые вопросы

- Алфавитный индекс примитивов sync в приложении — по желанию.
- Отдельная глава work-stealing — только при запросе (сейчас упоминание в ch08).
