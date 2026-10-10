# Глава 9. CMake и отдельная сборка

## Зачем эта тема

Ручная строка `g++` из главы 7 хороша для одного файла. Для `06_thread_pool` с тремя `.cpp` удобнее **отдельный** каталог CMake с флагами sanitizer, не ломая обычный `build/`.

## Отдельный build-tsan

```bash
cd multithread
cmake -S . -B build-tsan \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS="-fsanitize=thread -g -O1 -fno-omit-frame-pointer" \
  -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=thread -pthread"
cmake --build build-tsan --target 06_thread_pool
./build-tsan/06_thread_pool
```

`Threads::Threads` уже добавляет pthread — иногда достаточно только `CXX_FLAGS` + `CMAKE_EXE_LINKER_FLAGS` с тем же sanitize. Проверь link line в `VERBOSE=1`.

## Опция ENABLE_TSAN (как в томе 5)

В локальной копии или будущем патче (не без запроса хозяина):

```cmake
option(ENABLE_TSAN "Build with ThreadSanitizer" OFF)
if(ENABLE_TSAN)
  add_compile_options(-fsanitize=thread -g -O1 -fno-omit-frame-pointer)
  add_link_options(-fsanitize=thread)
endif()
```

Configure: `cmake -B build -DENABLE_TSAN=ON`.

## Не смешивать объекты

Один `build/` без sanitizer, второй `build-tsan` — **не** переиспользуй `.o` между ними. `rm -rf build-tsan` при смене флагов.

## ASan через CMake

```cmake
add_compile_options(-fsanitize=address,undefined -g -O1)
add_link_options(-fsanitize=address,undefined)
```

Отдельный `build-asan`.

## compile_commands

`CMAKE_EXPORT_COMPILE_COMMANDS=ON` для clangd — флаги sanitizer попадут в json; IDE покажет красное — нормально для tsan build dir.

## check.sh

Скрипт не знает про `build-tsan` — запускай бинарник вручную или обёртку сам. Не меняем `check.sh` без запроса.

## Ninja vs Make

Either generator — VERBOSE shows link.

## Multiple targets one build-tsan

One configure — all targets TSan — `./build-tsan/05_highscore` etc.

## Rerun cmake

Changed flags — re-run **cmake** configure, not only build.

## IDE Run button

Point to build-tsan binary path manually — CLion/VS Code launch.json — tom 6 IDE chapter echo.

## Install prefix

Not used in multithread — no install step.

## Cross compile

Not course focus — sanitizer on target arch must match.

## Presets CMake 3.19+

Could encode tsan preset — optional modern CMake; repo uses 3.16 min.

## Failure: cannot find -lasan

Wrong link stage — add sanitize to EXE_LINKER_FLAGS.

## Типичные заблуждения

«Добавлю флаг в существующий build» — пересоздай или clean. «Только add_compile без link» — неполный sanitizer.

## Зачем отдельный `-B build-tsan`

Same source, different flags. **`build/`** stays fast for daily work. **`build-tsan`** for race hunting. Never reuse object files between them.

## Полный пример TSan для 06

```bash
cd multithread
cmake -S . -B build-tsan   -DCMAKE_BUILD_TYPE=Debug   -DCMAKE_CXX_FLAGS="-fsanitize=thread -g -O1 -fno-omit-frame-pointer"   -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=thread -pthread"
cmake --build build-tsan --target 06_thread_pool
./build-tsan/06_thread_pool
```

**VERBOSE=1** on build — verify **link line** contains `-fsanitize=thread`.

## ENABLE_TSAN option (tom 5 pattern)

Tom 5 ch10 shows **optional** block — **OFF** by default in repo:

```cmake
option(ENABLE_TSAN "Build with ThreadSanitizer" OFF)
if(ENABLE_TSAN)
  add_compile_options(-fsanitize=thread -g -O1 -fno-omit-frame-pointer)
  add_link_options(-fsanitize=thread)
endif()
```

Configure: `cmake -B build -DENABLE_TSAN=ON`. **Do not** commit without owner request.

## ASan target

```cmake
option(ENABLE_ASAN "Address+UB sanitizer" OFF)
if(ENABLE_ASAN)
  add_compile_options(-fsanitize=address,undefined -g -O1)
  add_link_options(-fsanitize=address,undefined)
endif()
```

Separate **`build-asan`**.

## Threads::Threads

`find_package(Threads)` + `target_link_libraries(... Threads::Threads)` already in multithread CMake. TSan still wants **`-pthread`** on link line explicitly in some setups — check VERBOSE.

## compile_commands.json

`-DCMAKE_EXPORT_COMPILE_COMMANDS=ON` — clangd shows sanitizer flags for that build dir. Switch symlink or copy for IDE.

## check.sh

Script runs **build/** binaries. For TSan: `./build-tsan/02_ticket_office` manually. Wrapper script — personal choice.

## cicd/ и multithread

**cicd/** — separate CMake project (tom 5). Same **pattern**: sanitizer flags on **separate** build tree if you add tests there later.

## Clean on flag change

Changed `-fsanitize`? **`rm -rf build-tsan`** and reconfigure.

## § К собесу

Separate build dir; CXX_FLAGS + EXE_LINKER_FLAGS; ENABLE_TSAN optional; VERBOSE link line.

<!-- vol7-pass5-ch09 -->

## Дополнительное закрепление

VERBOSE link line must have `-fsanitize=thread`. ENABLE_TSAN локально — не commit без запроса.

compile_commands для clangd из build-tsan.

check.sh не знает build-tsan — ручной прогон.

## Краткое резюме

Отдельный `-B build-tsan`, флаги на compile **и** link, target как обычно. OPTION для удобства. Дальше — GCC vs Clang.

## add_compile_options scope

Global in CMakeLists affects **all** targets in project — OK for multithread small tree. Bigger repos use `target_compile_options` per target.

## Generator expression (preview)

`$<$<BOOL:${ENABLE_TSAN}>:-fsanitize=thread>` — modern style; optional read, repo uses simple if().

## Running check.sh vs single target

check.sh all 8 TSan — long; `--target 02_ticket_office` for focused fix.

## multithread/CMakeLists.txt line count

Small file — tom 5 ch10 walkthrough — add ENABLE block at end **locally** only.

<!-- vol7-pass6 -->
## Строка для памяти (Kindle)

Перед сном одним взглядом: **compile+link**, **-g -O1**, **TSan+pthread**, **два build dir**, **два стека в отчёте**, **solutions не спойлерить**. Завтра — один TSan-прогон на 02 или на последнем изменённом advanced target.

## Вопросы для самопроверки

1. Зачем отдельный каталог build?
2. Какие две CMake переменные для link TSan?
3. Что смотреть в VERBOSE=1?
4. Можно ли reuse build/ после TSan?
5. Связь с томом 5 option ENABLE_TSAN?

<!-- vol7-pass4-ch09 -->
## CMake как tom 5 обещал

Глава 10 tom 5 показала **куда** вставить `ENABLE_TSAN`; tom 7 даёт **полный** workflow `build-tsan` и VERBOSE link line. Если link без `-fsanitize=thread` — вернись к `CMAKE_EXE_LINKER_FLAGS`.

`compile_commands.json` из build-tsan полезен clangd: подсветка может отличаться от обычного build — **нормально**, переключай symlink когда работаешь с sanitizer.

Не меняй `check.sh` без запроса хозяина: книга учит **ручному** `./build-tsan/...` или личному alias.

## Обсуждение с репетитором

**Том:** 7 · **Глава:** `ch09` · **Файл:** `ch09-cmake-sanitizer-build.md`

**Дальше по курсу:** [Глава 10. GCC и Clang](./ch10-gcc-vs-clang.md)
