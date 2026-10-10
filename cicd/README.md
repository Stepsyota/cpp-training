# CI/CD в GitLab

Трек про **процесс**: проверка → сборка артефактов → registry → ручная
доставка (CD). Флаги компиляции, sanitizers, gdb — **другие треки**, здесь
только `-DCMAKE_BUILD_TYPE=Release` где нужно.

Если CI/CD в целом не ясен — сначала [THEORY.md](THEORY.md).

## Что здесь «продукт»

`workshop/` — маленький `forge-cli`. Его не дописываешь: через него гоняют
пайплайн. Как на работе: код уже есть, задача — **собрать и отдать** пакет
и образ.

## Как работать

```bash
cd cicd
./check.sh          # упражнение 01 — допишешь сам
```

GitLab CI живёт в **корне репозитория**: `.gitlab-ci.yml` (упражнения 02+).

## Задачи

| # | Папка | Тема | Что написать |
|---|--------|------|----------------|
| 1 | `beginner/01_check_script` | локальная проверка | `cicd/check.sh` |
| 2 | `beginner/02_ci_gate` | CI | `.gitlab-ci.yml` — stage `test` |
| 3 | `beginner/03_cpack_artifact` | пакет CPack | job `package` + `artifacts` |
| 4 | `beginner/04_container_image` | Docker-образ | `docker/Dockerfile` + push в registry |
| 5 | `beginner/05_cpack_in_container` | пакет под другую ОС | сборка `.deb` внутри контейнера |
| 6 | `beginner/06_cd_manual` | CD | manual job, tag, артефакты остаются в GitLab |

Иди по порядку: каждый шаг опирается на предыдущий.

## Что получится в конце (как на работе, упрощённо)

На push / tag GitLab сам:

1. гоняет `./cicd/check.sh` (MR не ломает сборку);
2. собирает **TGZ/DEB** через CPack и кладёт в **Job artifacts**;
3. собирает **Docker-образ** и пушит в **GitLab Container Registry**;
4. (упр.) второй `.deb` собирает в контейнере `ubuntu:22.04` — имитация
   «пакет только на целевой системе»;
5. CD-job **manual** — ты нажимаешь «Play», забираешь артефакты (пока без
   bare metal; это `advanced/` потом).

## Решения

`solutions/` — эталоны **после** своей попытки.

## Связь с multithread

`multithread/` не трогаем. Там свой `./check.sh` на все 8 задач — эталон
**идеи**, но в `cicd/check.sh` пишешь **ты**, по README упражнения 01.
