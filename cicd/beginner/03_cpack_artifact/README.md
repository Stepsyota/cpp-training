# 03. Пакет через CPack → GitLab artifacts

Первый **настоящий арtefact** с работы: `.deb` или `.tar.gz` из CPack.
Job складывает файлы в **Job artifacts** — их видно в UI и можно скачать.

## Что должно получиться

В `.gitlab-ci.yml`:

- новый stage `build` **после** `test`;
- job `package:cpack` (имя любое):
  - `needs: ["workshop:test"]` — не собирать пакет, если тесты упали;
  - Release-сборка workshop + `cpack` в `cicd/build/`;
  - `artifacts` с путями к `*.deb` и/или `*.tar.gz`, `expire_in` (например
    1 week).

Локально заранее убедись, что cpack работает — см. `workshop/README.md`.

## Подсказки

Скелет job:

```yaml
package:cpack:
  stage: build
  needs: ["workshop:test"]
  image: ubuntu:22.04
  before_script:
    - apt-get update && apt-get install -y cmake g++ make
  script:
    - cd cicd
    - cmake -S workshop -B build -DCMAKE_BUILD_TYPE=Release
    - cmake --build build
    - cd build && cpack
    - ls -la
  artifacts:
    paths:
      - cicd/build/*.deb
      - cicd/build/*.tar.gz
    expire_in: 1 week
```

Пути в `artifacts.paths` — **от корня репо**. Если cpack кладёт файлы в
подкаталог `_CPack_Packages`, поправь paths под фактический вывод `ls`.

На runner'е для `.deb` может понадобиться `dpkg-dev` / `fakeroot` — если
job падает, читай лог и добавь пакет в `before_script`.

## Что проверить в GitLab UI

После зелёного pipeline: job `package:cpack` → **Browse artifacts** →
скачай `.deb` или `.tar.gz`.

Это и есть «пакет лежит в GitLab», а не на ноутбуке.

## Дальше

`04_container_image` — Docker-образ в Container Registry.
