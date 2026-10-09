#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
cmake -S "$ROOT/workshop" -B "$ROOT/build"
cmake --build "$ROOT/build" -j"$(nproc)"

echo
echo "======== forge_cli_test ========"
if "$ROOT/build/forge_cli_test"; then
    echo "OK  forge_cli_test"
else
    echo "FAIL forge_cli_test"
    exit 1
fi

echo
echo "Workshop проверен."
