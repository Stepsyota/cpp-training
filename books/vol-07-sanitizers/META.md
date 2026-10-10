# META — том 7

| Поле | Значение |
|------|----------|
| **Цель** | Собирать и читать отчёты ASan, UBSan, TSan на Fedora (GCC/Clang) |
| **Связь** | Том 3 (гонки), том 5 (флаги), том 6 (GDB после отчёта), `multithread/` |
| **Статус** | черновик расширенный (ch01–ch14, ~2600 строк md, книжный стиль) |

## Оглавление

| ch | Файл | Тема |
|----|------|------|
| 01 | ch01-what-are-sanitizers | Instrumentation, compile+link, -g -O1 |
| 02 | ch02-address-sanitizer | ASan, heap, stack, UAF |
| 03 | ch03-asan-report | Читать отчёт, shadow memory |
| 04 | ch04-ubsan | UB, -fsanitize=undefined |
| 05 | ch05-thread-sanitizer | TSan, data race |
| 06 | ch06-tsan-report | Два стека, happens-before |
| 07 | ch07-multithread-exercises | 02, 05, stress, не спойлерить solutions |
| 08 | ch08-combining-sanitizers | ASan+UBSan, TSan отдельно |
| 09 | ch09-cmake-sanitizer-build | Цель/опция в CMake |
| 10 | ch10-gcc-vs-clang | Кто что поддерживает на Fedora |
| 11 | ch11-suppressions | Когда и как (осторожно) |
| 12 | ch12-leaks-lsan | LeakSanitizer с ASan |
| 13 | ch13-limits | Что не ловят, deadlock, логика |
| 14 | ch14-summary-myths | Чеклист, том 8 |
