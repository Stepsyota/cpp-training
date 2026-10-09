# Advanced — bare metal (позже)

Сюда перенесём, когда пройдёшь beginner/06:

- deploy job с **SSH** (`scp` / `rsync` `.deb` на сервер);
- установка: `dpkg -i` или свой install script;
- **Environment** `production` + manual approval;
- rollback: redeploy job с предыдущим `$CI_COMMIT_TAG`;
- второй «целевой» пакет (вторая OS в Docker) — копия упражнения 05.

Пока не открывай — сначала зелёный pipeline с manual CD в GitLab UI.
