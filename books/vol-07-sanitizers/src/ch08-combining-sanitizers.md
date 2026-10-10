# Глава 8. Комбинации санитайзеров

## Зачем эта тема

Хочется «всё сразу». На практике **не все** санитайзеры дружат в одном бинарнике. Неправильная смесь — ложные отчёты или link error.

## ASan + UBSan

Обычно **OK** вместе:

```bash
-fsanitize=address,undefined
```

На compile и link. Память + shift/overflow в одном прогоне.

## TSan отдельно

**ThreadSanitizer** historically **не комбинируют** с ASan в одной сборке (конфликт instrumentation). Делай **два** build directory:

- `build-asan` — memory + UB
- `build-tsan` — races

Перед merge — прогон обоих на релевантных targets.

## MSan, LSan

**MemorySanitizer** (uninitialized read) — отдельная тяжёлая сборка, реже в курсе. **LSan** (leaks) — часть ASan на glibc Linux, глава 12.

## -fsanitize=thread и библиотеки

Весь executable и **статически** подтянутый код должен быть с TSan. Смешивание TSan `.o` с обычными из сторонней **`.a` без TSan** — проблемы. Учебные exe только из своих `.cpp` — просто.

## Оптимизация

Не добавляй `-O3` «для скорости» к sanitizer build. Держи **`-O1 -g`**.

## CI идея

Nightly: `02`–`08` с TSan; weekly ASan+UBan на smoke. Хозяин репо решает сам — книга даёт модель.

## UBSan + TSan separate runs

Same source, two configures — not two `-fsanitize` in one.

## LSan with TSan build

TSan build — leaks detection not focus; use ASan build for leaks.

## Hardware

Sanizers increase RAM — CI runner size matters for nightly.

## Docker

Fedora container with g++ — same flags as host — tom 5 CI ideas.

## Fuzz later

libFuzzer + ASan — industry; optional reading.

## When only one run time

Choose **TSan** if multithread diff; **ASan+UBSan** if pointer/lifetime diff.

## Regression

After green TSan, don't delete build-tsan until merge — rerun before push.

## Типичные заблуждения

«Один cmake с тремя sanitize» — TSan отдельно. «ASan поймает race» — не специализирован. «Sanitizer в static lib без флагов» — опасно.

## ASan + UBSan в одном exe

Standard pattern for memory + UB subset:

```bash
-fsanitize=address,undefined
```

Same on **link**. One `build-asan` directory. Run tests once — two layers.

## TSan isolation

**Do not** link `-fsanitize=address,thread` together — conflict, false reports, or link failure. **Two directories**:

- `build-asan` — pointer bugs + UB
- `build-tsan` — races

Before merge: run **both** on relevant targets.

## MSan

**MemorySanitizer** — uninitialized reads — **whole** program must be MSan-instrumented including DSO. Heavy; skip in course except mention.

## Static libraries

Prebuilt **`.a`** without sanitizer flags linked into TSan exe — **bad**. `multithread/` builds all from source — simple.

## -O3 trap

Don't `-O3` sanitizer builds. **-O1 -g**.

## CI pipeline sketch (tom 5 tie-in)

**Job compile**: `-Wall -Wextra -Wpedantic -Werror` (fast). **Job test**: default build + `./check.sh`. **Job tsan-nightly**: `cmake -B build-tsan ...`, run 02-08, long timeout. **Job asan-weekly**: `build-asan`. Owner enables in `.gitlab-ci.yml` when ready — book does **not** patch CI.

## Order of operations

Develop with normal build → green check.sh → TSan on touched → if raw pointers → ASan+UBSan.

## § К собесу

ASan+UBSan OK; TSan separate; two build dirs; CI layers compile vs runtime.

<!-- vol7-pass5-ch08 -->

## Дополнительное закрепление

Две папки build-asan / build-tsan. ASan+TSan one link — **нет**.

CI: `-Werror` compile ≠ sanitizer run (tom 5).

ccache: clear при первом sanitize.

## Краткое резюме

ASan+UBSan вместе; TSan — отдельная сборка. Два каталога build. Дальше — CMake.

## GitLab CI sketch (текст, не патч)

Stage **build** — cmake default, `./check.sh`. Stage **tsan** — `allow_failure: false` only when stable; `timeout: 2h`. Artifacts: `asan.log`, `tsan.log`. Variables: `CMAKE_FLAGS`. Tom 5 `-Werror` on **build** job separate.

## Local alias

`alias mt-tsan='cmake --build build-tsan --target'` — personal shell, not repo.

## Sanitizer and ccache

ccache may cache wrong objects if flags change — clear cache when enabling sanitize first time.

<!-- vol7-pass6 -->
## Строка для памяти (Kindle)

Перед сном одним взглядом: **compile+link**, **-g -O1**, **TSan+pthread**, **два build dir**, **два стека в отчёте**, **solutions не спойлерить**. Завтра — один TSan-прогон на 02 или на последнем изменённом advanced target.

## Вопросы для самопроверки

1. Можно ли ASan и TSan в одном link?
2. Строка флагов для ASan+UBSan?
3. Зачем два build dir?
4. Влияет ли сторонняя .a без TSan?
5. Какой `-O` с sanitizer?

<!-- vol7-pass4-ch08 -->
## Два каталога — не опция

Запиши в notes: `build-asan` = address+undefined; `build-tsan` = thread. **Никогда** не `-fsanitize=address,thread` в одном link на курсе.

Перед merge в personal workflow: checklist «TSan targets green? ASan если трогал raw memory?». Это **не** git hook по умолчанию — **твоя** дисциплина, tom 7 + tom 3 ch14.

CI tom 5 tie-in: **parallel jobs** в GitLab — compile fast, tsan slow; artifacts logs; не блокировать каждый push, пока TSan не стабилен по времени.

## Обсуждение с репетитором

**Том:** 7 · **Глава:** `ch08` · **Файл:** `ch08-combining-sanitizers.md`

**Дальше по курсу:** [Глава 9. CMake и отдельная сборка](./ch09-cmake-sanitizer-build.md)
