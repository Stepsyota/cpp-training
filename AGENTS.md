# Контекст для агентов (`/home/stepsyota/training`)

Этот репозиторий — личный тренажёр C++. Сейчас два трека:
**многопоточность** (`multithread/`) и **CI/CD в GitLab** (`cicd/`).
Файл нужен, чтобы новый чат не начинал с нуля и не ломал договорённости.

Язык общения с хозяином репозитория — **русский**. Идентификаторы в коде
и публичные API упражнений — английские.

---

## Кто учится и как с ним работать

Хозяин до этого трека **с многопоточностью не работал**. Пятёрка
новичков и три крупных модуля уже решены им самим (часто с подсказками,
без готового кода).

### Как учить (обязательно)

- Пока он пишет задачу: **подталкивать, не решать**. Описать дыру,
  аналогию, что проверить. Не вставлять готовый цикл/`wait`/CAS.
- Когда просит «проверь» / «оцени» — можно разобрать код прямо, гонять
  тесты, хвалить конкретное, указывать остаток.
- Теорию, которую пришлось разжёвывать, **дописывать в**
  `multithread/THEORY.md`, чтобы не объяснять третий раз с нуля.
- Не подсовывать `solutions/` как «просто скопируй». Эталоны — после
  его попытки.
- Коммиты и пуш — **только если явно попросил**.
- AI-подсказки в редакторе он отключал специально (думать самому).
  Не уговаривать включить Tab. Пользовательские настройки Cursor:
  `cursor.cpp.disabledLanguages`, `editor.inlineSuggest.enabled: false`.

### Что он уже хорошо понимает

Потоки ОС ≠ 6/12 SMT на Ryzen 5 5500; `std::thread` + `join`; гонка на
`++`; `mutex` / `lock_guard`; `async` + `launch::async` + `future`;
`promise` как ящик, когда нет `return`; `condition_variable` + предикат
+ lost wakeup; CAS для рекорда; пул (очередь, `stopped`, `busy`);
bounded queue (`close` ≠ выкинуть стол); `std::move` как метка, не
магия; `invoke_result_t` как «тип return у fn».

### На чём спотыкался (не повторять слепо, но помнить)

- «Отдать ядро» = планировщик снял поток A, крутит B; не лимит 12
  `std::thread`.
- `std::terminate` зовёт **деструктор ручки** `std::thread`, не поток ОС.
  Забыл `join`/`detach` у joinable — abort. `catch` не ловит.
- Пустой `std::unique_lock<std::mutex> m;` **не** берёт поле `m`
  (затенение). Нужно `lock(поле_мьютекса)`.
- `cv.wait` без предиката после раннего `notify` — lost wakeup.
- Кузнец в конструкторе: не `join` сразу; ручку класть в `workers`;
  вечный цикл + `wait`; `join` в деструкторе **без** того же mutex
  (иначе deadlock: воркер ждёт замок на `--busy`).
- `if (closed) return` / `if (stopped) return` без «и очередь пуста» —
  выкидывает оставшиеся задачи. Формула: закрыто **и** брать нечего.
- `notify` **под** mutex — нормально (спящий уже отпустил замок).
  Путать с `join` под mutex нельзя.
- `size / n_threads` как длина куска теряет хвост (`10/3 → 9`). Нужно
  `begin = i * n / n_threads`, `end = (i+1) * n / n_threads`
  (умножение **до** деления) или нарезка шагом `j = i; j += n`.
- Тесты `08` удачно делятся нацело (200/4, 24/4) — зелёный тест не
  доказывает, что куски без остатка полные.

Квиз по пятёрке: 5/6. Ошибка была только про «кто вызывает terminate».

---

## Что лежит в репо

```
training/
  AGENTS.md                 ← этот файл
  multithread/
    THEORY.md               ← теория, читать сверху вниз
    README.md, check.sh, CMakeLists.txt
    beginner/01..05         ← короткие задачи (task.cpp)
    advanced/01..03         ← крупные модули
    solutions/              ← эталоны, не спойлерить зря
    build/                  ← cmake, в git не надо
  cicd/
    THEORY.md               ← GitLab CI/CD, арtefact'ы, CD
    README.md, check.sh     ← check.sh пишет хозяин (упр. 01)
    workshop/               ← учебный «продукт» (forge-cli)
    beginner/01..06         ← check.sh, .gitlab-ci.yml, Docker…
    advanced/               ← bare metal позже
    docker/                 ← Dockerfile (упр. 04)
    solutions/              ← эталоны после попытки
    build/                  ← локальная сборка
```

C++17, `-pthread`, CMake ≥ 3.16. Clangd смотрит `compile-commands-dir=build`.

```bash
cd multithread
cmake -S . -B build
cmake --build build --target 06_thread_pool   # или 01_rocket_launch …
./build/06_thread_pool
./check.sh                                    # все 8
```

Сырой g++ (из папки задачи):  
`g++ -std=c++17 -pthread -Wall -o name task.cpp test.cpp && ./name`  
`-o` — имя бинарника, не исходник.

Править в упражнениях только то, что сказано в README («ты пишешь»).
Тесты и данные цеха — спецификация.

---

## Трек многопоточности

### Новички (`beginner/`) — закрыты

| Цель | Тема | Что писал |
|------|------|-----------|
| `01_rocket_launch` | `thread` + `join` | другой поток + дождаться |
| `02_ticket_office` | `mutex` + `lock_guard` | `++` под замком |
| `03_oracle` | `async(launch::async).get()` | результат из другого потока |
| `04_elevator` | `cv` + предикат | wait/notify, lost wakeup |
| `05_highscore` | `atomic` + CAS | max без окна load/store |

Писал только `task.cpp`. Его решения зелёные.

### Крупные (`advanced/`) — закрыты

**06 Кузница — `ThreadPool`.** Писал `thread_pool.hpp` + `.cpp`.
Готовые `order.hpp`, `smithy.cpp` (сдаёт через `submit`).
Поля: `workers`, `jobs`, `m`, `cv`, `stopped`, `busy`.
Кузнец: `[this]`, `wait` пока `!jobs.empty() || stopped`, уход если
пусто и закрыто, снять работу, `++busy`, отпустить лок, `job()`,
`--busy`, `notify` если простой. Деструктор: флаг под замком →
`notify_all` → `join` **без** `m`. `submit` после `stopped` игнорит.
`job()` в `try/catch`. Одна cv на работу и на `wait_idle`.

**07 Конвейер — `BoundedQueue<T>`.** Только `bounded_queue.hpp`.
Завод в `factory.cpp`: штамп → очередь → покраска → очередь → упаковка.
Поля: `cap_` (потолок, не длина), `q`, `m`, `not_empty`, `not_full`,
`closed`. `pop` false только если `closed && q.empty()`.
`push`/`front` через `std::move`.

**08 Архив — `parallel_map`.** Только `parallel_map.hpp`.
Сначала сделал нарезку **шагом** (`j = i; j += n`) — полная, без дыр.
Потом куски: сначала `range = size/n` (дыра в хвосте, тесты не поймали),
затем формула `i * size / n` и `(i+1) * size / n`.
`out` типа `vector<invoke_result_t<F, const T&>>`. Пустой вход и
`n_threads > size` учтены (`min`). Замок на `input`/`out` не нужен.

Следующий крупный модуль **ещё не заказывали**. Не плодить новые
упражнения без просьбы. Если попросит продолжение — логичные темы:
пул + bounded queue вместе, work-stealing, `jthread`/`latch`,
исключения в пуле, fair scheduling.

---

## Трек CI/CD (`cicd/`)

GitLab CI, не GitHub Actions. Хозяин на работе: Docker-образ + несколько
пакетов (CPack; часть пакетов — сборка внутри контейнеров под целевую ОС).
Сейчас deploy **manual**; bare metal — `advanced/` позже.

Знает bash, multistage Docker — базовый «что такое CI» не разжёвывать.
**Флаги компиляции** — отдельный трек, не смешивать.

### Как учить

- **`cicd/check.sh`** — пишет **он** (упр. 01); в репо заглушка `exit 1`.
  Не подменять готовым скриптом, пока не попросил проверку.
- **`.gitlab-ci.yml`** — в **корне репо**; упражнения 02–06, подсказки в
  `beginner/*/README.md`.
- **`workshop/`** — готовый мини-продукт; не просит дописывать C++.
- Эталоны: `cicd/solutions/` (check.sh, gitlab-ci.yml, Dockerfile).

### Упражнения (порядок)

| # | Папка | Суть |
|---|--------|------|
| 1 | `01_check_script` | свой `check.sh` для workshop |
| 2 | `02_ci_gate` | stage test |
| 3 | `03_cpack_artifact` | CPack → GitLab artifacts |
| 4 | `04_container_image` | образ → Container Registry |
| 5 | `05_cpack_in_container` | `.deb` внутри ubuntu-контейнера |
| 6 | `06_cd_manual` | `when: manual`, tag, без пересборки |

Теория: `cicd/THEORY.md` (pipeline, artifacts vs cache, immutable artifact).

---

## Теория (`multithread/THEORY.md`)

Живой конспект. Главы 1–8 — потоки, гонки, mutex, async/future/promise
(разобрано подробно, с «достать promise» = завести ящик самому),
cv, atomics/CAS. По ходу чата дописано:

- ядра CPU vs `std::thread`, SMT 6/12;
- захват лямбды: `[&]` vs `[this]` (кузнецу поля — `[this]`);
- зачем `wait` передают тот же mutex; почему `unique_lock`, не
  `lock_guard`; почему `notify` под замком законен;
- глава 10: `std::move` (метка, не «переложить»; очередь, `thread`,
  `function`);
- шпаргалка / что не входит в пятёрку.

Не дублировать THEORY в чат целиком — ссылать и при нужде дополнять.

---

## Стиль его кода (не переписывать без нужды)

- Сюжетные имена: кузница, конвейер, архив.
- Часто оставляет TODO-комментарии из заготовок — не обязательно чистить.
- `emplace_back(std::thread(...))` и `threads[i] = move(thread(...))`
  вместо голого `emplace_back(лямбда)` — работает, можно намекнуть.
- `std::min` без `<algorithm>` однажды собрался транзитивно — лучше
  включать хедер явно.

Эталоны: `multithread/solutions/` (`06_thread_pool.*`,
`07_bounded_queue.hpp`, `08_parallel_map.hpp`, плюс пятёрка).

---

## Чего не делать

- Не решать за него текущую задачу, пока не попросил проверку.
- Не менять сигнатуры публичных методов упражнений.
- Не коммитить и не пушить без просьбы.
- Не тащить C++20 (`jthread`, `latch`) в уже сданные задачи без запроса.
- Не включать снова Cursor Tab / inline suggest.
- Не считать зелёный `08` доказательством формулы кусков — см. остаток.

Если контекста мало — читать этот файл, потом `multithread/README.md` и
`THEORY.md`, потом README конкретной папки.


Темы, которые я хотел бы изучить:
- CI/CD ← трек `cicd/` в работе
- флаги компиляции
- gdb
- санитаризация
- лямбда
- асинхронность
- ключевые слова C++