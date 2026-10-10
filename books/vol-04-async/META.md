# META — том 4

| Поле | Значение |
|------|----------|
| **Цель** | Учебник по асинхронной модели C++: future/promise, политики `async`, executors, coroutines **(C++20)** |
| **Связь** | Расширяет `multithread/THEORY.md` (гл. 6); упражнение `03_oracle`; том 3 — потоки и синхронизация |
| **Стандарт** | C++17 база; C++20 coroutines / `jthread` — с бейджами |
| **Статус** | черновик **расширенный** (ch01–ch14, ~2700 строк md, книжный стиль) |
| **Строк md (src)** | ~2717 (2026-10-10, раздувание) |

## Оглавление

| ch | Файл | Тема |
|----|------|------|
| 01 | ch01-tasks-not-threads | Задачи vs потоки, зачем future |
| 02 | ch02-std-async | `std::async`, `launch::async`, `deferred` |
| 03 | ch03-future-shared | `future`, `shared_future`, ожидание |
| 04 | ch04-promise | Ручной ящик, `broken_promise` |
| 05 | ch05-packaged-task | `packaged_task`, готовность к вызову |
| 06 | ch06-exceptions | Исключения через future |
| 07 | ch07-pitfalls | Деструктор future, взрыв потоков, policy |
| 08 | ch08-choose-model | thread / async / пул / coroutine |
| 09 | ch09-callbacks-erasure | Колбэки, `function`, владение |
| 10 | ch10-executors | Идея executor, TBB/Asio, P2300 обзор |
| 11 | ch11-coroutines-basics | `co_await`, promise_type, минимальный пример |
| 12 | ch12-generators-tasks | `co_yield`, task-типы в библиотеках |
| 13 | ch13-blocking-vs-async-io | Блокирующий I/O, preview без Asio-учебника |
| 14 | ch14-summary-myths | Мифы, к собесу, том 5 |
