# 06. CD — manual job (артефакты уже в GitLab)

CI собрал пакеты и образ. **CD** — решение «выкатываем» + использование
**уже собранного**. Сейчас без SSH на bare metal: job с **`when: manual`**.

## Что должно получиться

Stage `deploy` в `.gitlab-ci.yml`:

- job `release:manual` (или `deploy:staging`):
  - `when: manual`;
  - `needs:` jobs из 03, 04, 05 (или подмножество, если что-то ещё не готово);
  - правило запуска: например только `tags` или ветка `main` — выбери и
    зафиксируй в комментарии в YAML;
  - `script`:
    - скачать/проверить artifacts (`ls cicd/build/*.deb`, …);
    - `docker pull $CI_REGISTRY_IMAGE/forge-cli:$CI_COMMIT_SHA` (если 04 готов);
    - echo шаги, которые **на работе** заменишь на scp/ansible (пока
      достаточно явного echo «скопировал бы на bare metal X»).

После pipeline на tag/commit ты жмёшь **Play** в GitLab — это замена «я
вручную решил выкатить», но **без** ручной сборки пакета.

## Подсказки

```yaml
deploy:manual:
  stage: deploy
  when: manual
  needs:
    - job: package:cpack
      artifacts: true
    - job: package:deb-in-ubuntu
      artifacts: true
    - job: image:docker
  rules:
    - if: $CI_COMMIT_TAG
  script:
    - ls -la cicd/build/
    - docker pull "$CI_REGISTRY_IMAGE/forge-cli:$CI_COMMIT_SHA"
    - echo "CD placeholder — здесь scp/rsync на bare metal"
```

Имена jobs в `needs` должны **совпадать** с твоими из упражнений 03–05.

Опционально: GitLab **Environment** `staging` с `url:` на фиктивный хост —
появится в UI «Deployments».

## Проверка

1. Tag: `git tag v0.1.0 && git push origin v0.1.0`
2. Pipeline → job deploy → **Play**
3. Job зелёный, в логе видны арtefact'ы и pull образа

## Дальше (advanced, позже)

- SSH на bare metal: положить `.deb`, `systemctl restart …`
- тот же SHA образа/пакета, без пересборки
- protected tags, approval rules как на работе

Смотри `advanced/README.md`.
