#!/usr/bin/env bash
# Упражнение 01: допиши этот скрипт сам.
# Спецификация — beginner/01_check_script/README.md


# собирает таргеты forge_cli и forge_cli_test;
# запускает ./build/forge_cli_test — он печатает OK и exit 0;
# при успехе exit 0, при провале теста или сборки — exit ≠ 0.
# путь к корню cicd/ не должен зависеть от того, откуда вызвали скрипт

# Настраиваю скрипт так, чтобы он возвращал != 0 при любой ошибке
set -euo pipefail

# Получаю абсолютный путь где лежит скрипт
ROOT="$(cd "$(dirname "$0")" && pwd)"

# Генерирую Makefile для сборки
cmake -S "$ROOT/workshop" -B "$ROOT/build"

# Собираю проект, задействуя параллельно nproc потоков
cmake --build "$ROOT/build" -j$(nproc)

# Запускаю бинарник
"$ROOT/build/forge_cli_test"