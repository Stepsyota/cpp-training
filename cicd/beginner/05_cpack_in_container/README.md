# 05. CPack внутри контейнера (как «второй пакет»)

На работе два пакета собирают **не на runner'е**, а в Docker на конкретной
системе, потом **копируют** `.deb`/`.rpm` наружу. Здесь — **один** такой
пакет (идея масштабируется на второй и третий).

## Что должно получиться

Job `package:deb-in-ubuntu` (имя любое):

- stage `build`, после тестов;
- монтирует репозиторий в контейнер `ubuntu:22.04`;
- внутри: apt, cmake, g++, `cpack` → `.deb`;
- `.deb` попадает в **artifacts** (тот же механизм, что в 03, но файл
  собран «как на Ubuntu», а не на произвольном runner'е).

Отличие от упражнения 03:

| 03 | 05 |
|----|-----|
| cpack на runner'е job'а | cpack **внутри** `docker run` |
| «универсальный» TGZ/DEB | имитация **целевой** OS для пакета |

## Подсказки

Идея скрипта (адаптируй под свой runner с Docker):

```bash
docker run --rm \
  -v "${CI_PROJECT_DIR}/cicd:/work" \
  -w /work \
  ubuntu:22.04 \
  bash -c '
    set -euo pipefail
    apt-get update
    apt-get install -y cmake g++ make fakeroot dpkg-dev
    cmake -S workshop -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build
    cd build && cpack
  '
```

Арtefacts:

```yaml
artifacts:
  paths:
    - cicd/build/*.deb
  expire_in: 2 weeks
```

Если job 03 тоже кладёт `*.deb`, различай имена или `paths` к конкретному
файлу — в UI должно быть видно **два** способа сборки (или отключи DEB в
одном из jobs через `-DCPACK_GENERATOR=TGZ` — по желанию).

## Проверка

Pipeline → job → artifacts → `.deb` существует.  
Опционально: `dpkg-deb -I` локально после скачивания.

## Дальше

`06_cd_manual` — manual CD: «всё собрано, я жму Play».
