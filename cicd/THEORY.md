# CI/CD — конспект (GitLab)

Читать сверху вниз. Дополняем по ходу чатов, как `multithread/THEORY.md`.

---

## 1. Зачем это вообще

**Локально:** «у меня собралось, тесты прошли».

**В CI:** то же самое на **чистой** машине GitLab Runner — без твоих
забытых файлов в `build/` и без «а у меня работало».

**В CD:** уже **собранное** (пакет, образ) **доставить** туда, где оно
нужно — без ручного «scp с ноутбука».

Твоя текущая работа: собрал пакет → скопировал → скинул.  
Цель трека: **сборка и хранение на GitLab**, доставка — сначала manual job,
потом bare metal.

---

## 2. CI vs CD (коротко)

| | CI | CD |
|---|----|----|
| Вопрос | Код **годится**? | **Куда** отдать готовое? |
| Когда | каждый push / MR | tag, main, кнопка Play |
| Результат | зелёный pipeline | артефакт на сервере / в registry |
| Пересборка на «проде» | — | **нет** — берут тот же артефакт |

**Immutable artifact:** один раз собрали пакет/образ с тегом `abc123` — CD
катает **его**, а не «ещё раз cmake на проде».

---

## 3. GitLab CI — словарь

```text
.gitlab-ci.yml
    └── pipeline          (один запуск на push/tag/MR)
            └── stages[]  (порядок: test → build → deploy)
                    └── jobs  (конкретная машина + скрипт)
```

| Термин | Смысл |
|--------|--------|
| **Stage** | группа jobs; все jobs stage `test` завершились → stage `build` |
| **Job** | один runner, один скрипт (`script:`) |
| **`needs:`** | job B стартует после A, может взять артефакты A |
| **Artifact** | файлы job'а, GitLab хранит N дней; скачать из UI / передать дальше |
| **Cache** | ускорение (ccache, vcpkg) — **не** то же самое, что artifact |
| **Registry** | GitLab Container Registry — Docker-образы `registry.gitlab.com/...` |
| **Manual job** | `when: manual` — кнопка Play (твой нынешний «я сам нажал») |
| **Environment** | staging / production — позже, для bare metal |

Runner = машина, где крутится job (shared на GitLab.com или свой).

---

## 4. Цепочка артефактов (как у тебя на работе, упрощённо)

На работе в итоге нужно:

1. **Docker-образ** приложения  
2. **Пакет 1** — CPack на runner'е (или в build-образе)  
3. **Пакет 2, 3** — CPack **внутри** контейнеров «как на целевой ОС», файлы
   **копируются наружу** как artifacts  

В тренажёре:

- пакет «на runner'е» — упражнение **03**;
- образ — **04**;
- пакет «внутри ubuntu» — **05** (один вместо двух систем; идея та же).

```text
  ┌─────────────┐     ┌──────────────┐     ┌─────────────────┐
  │ job: test   │ ──► │ job: package │ ──► │ artifacts в UI  │
  │ check.sh    │     │ cpack        │     │ .deb / .tar.gz  │
  └─────────────┘     └──────────────┘     └─────────────────┘
         │                                          │
         │            ┌──────────────┐              │
         └──────────► │ job: image   │ ──► registry │
                      │ docker push  │              │
                      └──────────────┘              │
         ┌──────────────┐                           │
         │ job: deb_in  │ ──────────────────────────┘
         │ docker+cpack │
         └──────────────┘
                              │
                              ▼
                      ┌──────────────┐
                      │ deploy:manual│  ← CD, Play
                      └──────────────┘
```

---

## 5. CPack в двух словах

CMake + `install()` + `include(CPack)` → `cpack` в каталоге сборки создаёт
`.tar.gz`, `.deb`, …

Job в CI:

```bash
cmake -S cicd/workshop -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
cd build && cpack
```

Пути к файлам — в `artifacts.paths` (например `cicd/build/*.deb`).

---

## 6. Docker-образ как арtefact

Образ **не** кладут в job artifacts (слишком тяжёлый). Его **push** в
**Container Registry**:

```text
registry.gitlab.com/<group>/<project>/forge-cli:<tag>
```

Теги:

- `$CI_COMMIT_SHA` — уникально на каждый commit;
- `$CI_COMMIT_TAG` — релиз;
- `latest` — осторожно, перезаписывается.

Multistage Dockerfile ты уже знаешь: builder (cmake) → runtime (только
бинарник). В упражнении 04 свяжеешь с `docker build` / `docker push` в job.

---

## 7. Пакет внутри «чужой» ОС (упражнение 05)

Runner может быть Debian, а `.deb` нужен под конкретную базу. Решение:

```bash
docker run --rm -v "$PWD:/src" -w /src ubuntu:22.04 bash -c '
  apt-get update && apt-get install -y cmake g++ ...
  cmake ... && cpack
'
```

Готовый `.deb` копируется в volume → job забирает → `artifacts:`.

Так же на работе собирают «RPM в centos-контейнере» — второй и третий
пакет; здесь один пример достаточен.

---

## 8. CD без bare metal (упражнение 06)

Пока **нет** SSH на железо. CD = **manual job**, который:

- `needs:` jobs с арtefactами / после push образа;
- `when: manual` + опционально `only: tags`;
- в `script` — проверка «артефакты на месте» (ls, `docker pull`) и echo
  «готово к выкладке».

Ты жмёшь Play в GitLab UI — аналог «я решил, что пора выкатывать», только
артефакты уже **лежат в GitLab**, не на ноутбуке.

**Следующий этап (`advanced/`):** тот же `.deb` / образ → scp / ansible /
systemd на bare metal.

---

## 9. Переменные GitLab (шпаргалка)

| Переменная | Значение |
|------------|----------|
| `CI_PROJECT_DIR` | корень репозитория на runner'е |
| `CI_COMMIT_SHA` | hash commit |
| `CI_COMMIT_TAG` | tag, если pipeline по tag |
| `CI_REGISTRY` | адрес registry |
| `CI_REGISTRY_IMAGE` | `registry.../group/project` |
| `CI_JOB_TOKEN` | login для docker push (часто) |

Полный список: GitLab docs → Predefined variables.

---

## 10. Что сознательно не в этом треке

- флаги компиляции (`-Wall`, sanitizers) — отдельно;
- matrix gcc/clang;
- Kubernetes, Helm;
- автоматический deploy на prod без manual.

---

## 11. Локальная отладка пайплайна

- Править `.gitlab-ci.yml` → push в ветку → смотреть **CI/CD → Pipelines**.
- Job log — весь stdout/stderr; красный job = exit ≠ 0.
- Lint: **CI/CD → Editor** или `gitlab-ci-lint` в UI.
- Образ/registry: **Deploy → Container registry**, **Build → Artifacts**.

Перед push полезно прогнать `./cicd/check.sh` локально — тот же скрипт
вызовет job `test`.

---

## 12. Bash для `check.sh` (упражнение 01)

Не «магия CI», а обычный shell-скрипт: последовательность команд + код
выхода. GitLab потом просто запускает `./check.sh`.

### Shebang — первая строка

```bash
#!/usr/bin/env bash
```

| Часть | Смысл |
|-------|--------|
| `#!` | «дальше — какой программой выполнять файл» |
| `/usr/bin/env` | найти программу в `$PATH` |
| `bash` | нужен интерпретатор bash, не sh |

Зачем: `./check.sh` — ОС читает shebang и стартует bash с этим файлом.
Без shebang при `./check.sh` поведение может быть непредсказуемым.

`env bash` вместо `/bin/bash` — на другой машине bash может лежать в другом
месте; `env` ищет его сам.

### `$0` — «как меня вызвали»

`$0` — имя скрипта **как в команде**:

```bash
cd /tmp && /home/user/training/cicd/check.sh   # $0 = полный путь
cd cicd && ./check.sh                           # $0 = ./check.sh
```

Откуда вызвали — разное. Поэтому **нельзя** писать жёстко `cd cicd`.

### `dirname "$0"` — каталог, где лежит скрипт

`dirname` отрезает имя файла, оставляет путь к папке:

```text
/home/user/training/cicd/check.sh  →  /home/user/training/cicd
./check.sh                         →  .
```

Кавычки `"$0"` — на случай пробелов в пути.

### `cd "..." && pwd` — перейти и узнать полный путь

```bash
cd "$(dirname "$0")" && pwd
```

- `cd` в каталог скрипта (даже если `./check.sh` — окажемся в `cicd/`);
- `pwd` печатает **абсолютный** путь (`/home/.../cicd`);
- `&&` — `pwd` только если `cd` удался.

### `ROOT="$( ... )"` — сохранить в переменную

`$( ... )` — **подстановка команды**: выполнить внутри, подставить stdout.

```bash
ROOT="$(cd "$(dirname "$0")" && pwd)"
```

Читать **изнутри наружу**:

1. `"$0"` — как вызвали скрипт  
2. `dirname` — папка скрипта  
3. `cd` туда + `pwd` — абсолютный путь к `cicd/`  
4. присвоить в `ROOT`

Дальше: `"$ROOT/workshop"`, `"$ROOT/build"` — пути не ломаются от `cd` перед
запуском.

### `set -euo pipefail`

| Флаг | Эффект |
|------|--------|
| `-e` | любая команда с exit ≠ 0 → скрипт сразу падает |
| `-u` | необъявленная переменная → ошибка |
| `-o pipefail` | пайп `cmd1 \| cmd2` падает, если упала **любая** часть |

Для CI важно `-e`: упала сборка — скрипт не доходит до `exit 0`.

### CMake: три команды

```bash
cmake -S "$ROOT/workshop" -B "$ROOT/build"    # сгенерировать Makefile
cmake --build "$ROOT/build"                   # собрать (компиляция)
"$ROOT/build/forge_cli_test"                  # запустить тест
```

- `-S` — **S**ource (где `CMakeLists.txt`);
- `-B` — **B**uild (куда класть артефакты сборки);
- `--build` — **одно слово**, не `-- build`.

Последняя строка — запуск бинарника; exit code теста = exit code скрипта
(при `-e` упадёт сам, либо явный `exit $?`).

### `-j$(nproc)` — параллельная сборка

```bash
cmake --build "$ROOT/build" -j"$(nproc)"
```

- `nproc` — сколько потоков CPU видит система (у тебя 12);
- `-j12` — cmake/make компилируют до 12 файлов параллельно;
- `$(nproc)` — подставить число автоматически.

**Не обязательно** для `check.sh`. Можно просто `cmake --build "$ROOT/build"`.
`-j` — ускорение, не часть логики «прошёл / не прошёл».

### Мини-проверка понимания

В терминале из **любой** папки:

```bash
/home/stepsyota/training/cicd/check.sh   # после дописывания
echo $?                                   # 0 = успех
```

В скрипте временно добавь `echo "ROOT=$ROOT"` — должен быть путь к `cicd/`,
не `.` и не пусто.
