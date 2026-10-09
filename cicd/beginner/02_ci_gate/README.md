# 02. CI — гейт на merge request

GitLab при push читает **`.gitlab-ci.yml` в корне репозитория** и запускает
pipeline. Здесь — минимальный CI: один stage, один job, твой `check.sh`.

## Что должно получиться

Файл **`<корень репо>/.gitlab-ci.yml`** (не внутри `cicd/`):

- stage `test` (или один job без явных stages — GitLab создаст `test` сам);
- job, например `workshop:test`, на образе с cmake и g++ (`ubuntu:22.04` или
  `registry.gitlab.com/gitlab-org/cli-utils/...` — главное, чтобы были
  `cmake`, `g++`, `make`/`ninja`);
- `script` вызывает `cicd/check.sh`;
- pipeline **зелёный** на ветке, где workshop собирается;
- pipeline **красный**, если намеренно сломать тест (проверь один раз).

## Подсказки

Минимальный каркас:

```yaml
stages:
  - test

workshop:test:
  stage: test
  image: ubuntu:22.04
  before_script:
    - apt-get update && apt-get install -y cmake g++ make
  script:
    - cd cicd && chmod +x check.sh && ./check.sh
```

Уточни сам:

- нужен ли `apt-get` каждый раз (пока да — cache в другой раз);
- правила **когда** гонять: `rules:` / `only:` — хотя бы на MR и push в
  default branch.

Документация: GitLab → CI/CD → «Getting started» → «Add a `.gitlab-ci.yml`».

## Проверка

1. Commit + push в GitLab.
2. **CI/CD → Pipelines** — job прошёл.
3. В MR — галочка pipeline (если включены merge checks).

Локально YAML не исполняется — только на GitLab (или свой runner).

## Дальше

`03_cpack_artifact` — второй job собирает пакет и кладёт в artifacts.
