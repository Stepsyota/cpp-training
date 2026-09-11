#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
cmake -S "$ROOT" -B "$ROOT/build" -DCMAKE_BUILD_TYPE=Debug
cmake --build "$ROOT/build" -j"$(nproc)"

failed=0
for t in 01_rocket_launch 02_ticket_office 03_oracle 04_elevator 05_highscore \
         06_thread_pool 07_pipeline 08_parallel_map; do
    echo
    echo "======== ${t} ========"
    if "${ROOT}/build/${t}"; then
        echo "OK  ${t}"
    else
        echo "FAIL ${t}"
        failed=1
    fi
done

echo
if [[ "${failed}" -eq 0 ]]; then
    echo "Все задачи пройдены."
else
    echo "Есть незакрытые задачи — смотри FAIL выше."
fi
exit "${failed}"
