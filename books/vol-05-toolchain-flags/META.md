# META — том 5

| Поле | Значение |
|------|----------|
| **Цель** | Уверенно собирать C++ на Fedora (GCC/Clang), читать флаги, CMake трека `multithread/` |
| **Связь** | Том 6 (GDB), том 7 (санитайзеры) опираются на `-g`, `-O`, CMake |
| **Стандарт** | C++17 в репо; `-std=c++20/23` с пометками |
| **Статус** | расширенное издание (ch01–ch14), ~2600+ строк md |
| **Сборка** | `mdbook build` — зелёный (2026-10-10) |

## Оглавление

| ch | Файл | Тема |
|----|------|------|
| 01 | ch01-compile-pipeline | Препроцессор → компиляция → линковка |
| 02 | ch02-fedora-toolchain | g++, clang++, версии, пакеты |
| 03 | ch03-std-flag | `-std=`, `__cplusplus`, extensions |
| 04 | ch04-warnings | `-Wall`, `-Wextra`, `-Wpedantic`, `-Werror` |
| 05 | ch05-debug-symbols | `-g`, `-O0`, Debug в CMake |
| 06 | ch06-optimization | `-O2`, `-O3`, LTO, PGO обзор |
| 07 | ch07-pthread-linking | `-pthread`, `Threads::Threads` |
| 08 | ch08-separate-compile | `.o`, TU, заголовки, ODR |
| 09 | ch09-static-dynamic | `.a`, `.so`, `-l`, rpath |
| 10 | ch10-cmake-multithread | Разбор `multithread/CMakeLists.txt` (полный фрагмент) |
| 11 | ch11-clangd | `compile_commands.json`, clangd |
| 12 | ch12-preprocessor-flags | `-D`, `-U`, `-I`, `-include` |
| 13 | ch13-one-shot-gpp | Ручная строка как в AGENTS.md |
| 14 | ch14-msvc-differences | cl.exe, `/std`, `/W4`, отличия от GCC |

## Якорь репозитория

Эталонный CMake: `training/multithread/CMakeLists.txt` — C++17, GNU\|Clang warnings, `find_package(Threads)`, `add_exercise` ×5, targets `06_thread_pool`, `07_pipeline`, `08_parallel_map`.

## Открытые вопросы

- Алфавитный справочник «флаг → глава» в приложении — по желанию.
- Отдельная глава про `ninja`/`ccache` — не в базовом плане серии.

**Последнее обновление META:** 2026-10-10
