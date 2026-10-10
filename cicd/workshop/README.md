# Workshop — учебный «продукт»

Маленькое C++-приложение, которое в треке CI/CD **собирают, упаковывают и
отдают** как на работе: пакет через CPack, Docker-образ, артефакты в GitLab.

Ты **не** дописываешь логику приложения — оно готово. Твоя работа в других
папках: `check.sh`, `.gitlab-ci.yml`, Dockerfile.

## Локальная проверка (после того как допишешь `../check.sh`)

```bash
cd cicd
./check.sh
```

Или вручную:

```bash
cmake -S workshop -B build
cmake --build build
./build/forge_cli ping
./build/forge_cli_test
```

## CPack (понадобится в упражнении 03)

```bash
cmake -S workshop -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
cd build && cpack
ls -la forge-cli*.deb forge-cli*.tar.gz 2>/dev/null || ls -la _CPack_Packages/
```

Пакеты появятся в `build/` (имя зависит от генератора и версии).
