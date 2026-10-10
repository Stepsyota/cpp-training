# 04. Docker-образ → GitLab Container Registry

Второй тип арtefact'а на работе — **образ**. Его не кладут в job artifacts,
а **push** в registry GitLab.

## Что должно получиться

1. **`cicd/docker/Dockerfile`** — multistage: собрать `forge_cli`, runtime-
   stage только с бинарником (базу выбери сам: `debian:bookworm-slim`,
   `ubuntu:22.04`, …).

2. Job в `.gitlab-ci.yml`, например `image:docker`:
   - stage `build`, `needs: ["workshop:test"]`;
   - `docker build` с контекстом `cicd/` (Dockerfile видит `workshop/`);
   - login в `CI_REGISTRY` (часто через `CI_JOB_TOKEN`);
   - `docker push` на `$CI_REGISTRY_IMAGE/forge-cli:$CI_COMMIT_SHA` (или
     другое имя — главное, зафиксируй в README проекта).

Runner должен уметь Docker (**Docker executor** или `docker:dind` service).
Если shared runner на gitlab.com — обычно `image: docker:24` +
`services: [docker:24-dind]`. Свой runner — смотри, как у вас на работе.

## Подсказки (не готовый Dockerfile)

Builder stage:

- установить cmake, g++;
- `cmake -S workshop -B build && cmake --build build`;
- бинарник: `build/forge_cli`.

Runtime stage:

- `COPY --from=builder ... /usr/local/bin/forge-cli` (или свой PATH);
- `ENTRYPOINT` / `CMD` — например `forge-cli ping`.

Push (идея, синтаксис уточни в docs GitLab):

```yaml
before_script:
  - docker login -u "$CI_REGISTRY_USER" -p "$CI_JOB_TOKEN" "$CI_REGISTRY"
script:
  - docker build -f cicd/docker/Dockerfile -t "$CI_REGISTRY_IMAGE/forge-cli:$CI_COMMIT_SHA" cicd
  - docker push "$CI_REGISTRY_IMAGE/forge-cli:$CI_COMMIT_SHA"
```

## Проверка

**Deploy → Container registry** — образ с тегом commit SHA.  
Локально (если есть access): `docker pull registry.gitlab.com/...`.

## Дальше

`05_cpack_in_container` — пакет, собранный **внутри** образа «целевой ОС».
