# META — том 2

| Поле | Значение |
|------|----------|
| **Цель** | Глубокий учебник по lambda expressions и объектам-функциям |
| **Глубина** | R+P+I |
| **Стиль** | Повествовательный учебник (как том 1 после вычитки) |
| **Стандарт** | C++17 база; C++14 init-capture; C++20 generic/template lambdas — с пометками |
| **Эталон** | GCC/Clang, Fedora |
| **Последнее обновление** | 2026-10-10 |
| **Статус** | **книжное издание** (ch01–ch12 + введение; повествовательный объём ~2200+ строк md; mdbook build зелёный) |

## План оглавления (полный)

| # | Файл | Тема |
|---|------|------|
| 01 | ch01-closure-intuition | Замыкание, functor, зачем лямбда |
| 02 | ch02-syntax-and-call | Синтаксис `[](){}`, вызов, `operator()` |
| 03 | ch03-capture-defaults | `[=]`, `[&]`, смешанный захват |
| 04 | ch04-init-capture | Init-capture, move в лямбду **(C++14)** |
| 05 | ch05-mutable | `mutable` в теле лямбды |
| 06 | ch06-generic-lambdas | `auto` параметры; template lambda **(C++20)** |
| 07 | ch07-types-storage | Тип замыкания, `decltype`, `std::function` |
| 08 | ch08-stl-algorithms | Алгоритмы, компараторы, predicate |
| 09 | ch09-threads-async | `thread`, `async`, пулы — теория, без спойлеров 06–08 |
| 10 | ch10-lifetime-pitfalls | `[&]`, `this`, dangling |
| 11 | ch11-alternatives | Functor, `std::bind` (legacy), указатели на функции |
| 12 | ch12-summary-myths | Резюме серии, мифы, чеклист review |

## Открытые вопросы

- Нужна ли отдельная глава про `std::ref` / `reference_wrapper` в захвате — пока в ch10.
