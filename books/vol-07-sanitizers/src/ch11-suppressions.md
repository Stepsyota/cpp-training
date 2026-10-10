# Глава 11. Suppressions

## Зачем эта тема

Иногда отчёт sanitizer указывает на **библиотеку**, **старый баг в тестовом harness**, или **известный false positive**. **Suppression** говорит runtime «не считать эту пару стеков ошибкой». В учебном курсе suppress — **последнее** средство после fix.

## Файлы suppress

TSan: `tsan_suppressions.txt` с правилами, переменная `TSAN_OPTIONS=suppressions=path`.

ASan: `ASAN_OPTIONS=suppressions=...`.

Синтаксис — в документации LLVM/Google sanitizers; для курса достаточно знать, что **существует**.

## Опасность

Заглушить race в **своём** `task.cpp` — обмануть себя и CI. Code review должен отвергать suppress без ссылки на upstream bug.

## Ложные срабатывания

Редки в простом учебном коде. Чаще **реальный** баг в stdlib usage (data race на `static` внутри не thread-safe функции). Не спеши suppress — разбери.

## Внешние библиотеки

Если подключишь стороннюю `.so` без sanitizer — шум. В `multithread/` такого нет.

## Format sketch

Lines like `race:libfoo.so` — read upstream doc before writing.

## Version control

If suppress file exists — review in PR like code.

## Expiry

Calendar reminder remove suppress when lib fixed.

## Student task

If tempted suppress — write **why fix is hard** to tutor first.

## ASan interceptor

Advanced — skip.

## UBSan suppress

Also exists — same discipline.

## Corporate

Large codebases many suppress — training repo stays zero suppress goal.

## Типичные заблуждения

«suppress = professional» — fix professional. «Один suppress на весь проект» — маскирует баги.

## Когда suppress уместен

Known bug in **third-party** library, tracked upstream issue, cannot fix locally yet. **Not** for your **`task.cpp`** race.

## TSAN_OPTIONS

`TSAN_OPTIONS=suppressions=/path/tsan.supp` — file format in LLVM sanitizer docs. Course: know it **exists**.

## ASAN suppressions

`ASAN_OPTIONS=suppressions=...` — same idea.

## Review culture

Every suppress line needs **owner**, **issue URL**, **expiry date**. Review rejects «suppress to green CI».

## False positives in course code

**Rare** in simple exercises. Usually **real bug**. Re-read two stacks (ch6).

## std::locale / iostream static

Advanced courses mention non-thread-safe static in iostream — don't suppress — **sync** or avoid.

## multithread/ no external .so

No suppress files needed for clean student code.

## Alternative to suppress

Fix bug. Fork test. Skip test in CI **explicitly** (worse than fix).

## § К собесу

Suppress last resort; not for student race; needs ticket + review.

<!-- vol7-pass5-ch11 -->

## Дополнительное закрепление

Suppress в task.cpp — плохо для обучения. Industry — ticket + expiry.

Trace своё API если frame в /usr/.

Fix before suppress.

## Краткое резюме

Suppress для чужого/legacy кода, не для своих гонок. TSAN_OPTIONS/ASAN_OPTIONS. Дальше — leaks.

## Team policy one paragraph

If suppress: link issue, owner, remove-by date, test that would fail without suppress. **Zero** suppress in student exercises.

## grep suppress in interview

Bad sign if suppress hides product race — good sign if temporary third-party.

## Logging instead

Sometimes verbose log beats suppress for one-time debug.

<!-- vol7-pass6 -->
## Строка для памяти (Kindle)

Перед сном одним взглядом: **compile+link**, **-g -O1**, **TSan+pthread**, **два build dir**, **два стека в отчёте**, **solutions не спойлерить**. Завтра — один TSan-прогон на 02 или на последнем изменённом advanced target.

## Вопросы для самопроверки

1. Когда suppress уместен?
2. Почему опасен suppress в task.cpp?
3. Какая env для TSan suppress?
4. Что проверить перед suppress?
5. Есть ли suppress в чистом 02 с багом?

<!-- vol7-pass4-ch11 -->
## Культура без suppress в учебнике

В tom 7 suppress — **этика**: скрыть race в `task.cpp` = саботаж обучения. В industry suppress иногда нужен; ты учишься **сначала fix**, suppress — с ticket и сроком.

Если TSan ругается на код в `/usr/` — trace **своё** API: race часто **выше** по стеку в твоём submit/push.

Репетитор может спросить: «когда suppress уместен?» — ответ из этой главы, не «когда лень».

## Обсуждение с репетитором

**Том:** 7 · **Глава:** `ch11` · **Файл:** `ch11-suppressions.md`

**Дальше по курсу:** [Глава 12. LeakSanitizer](./ch12-leaks-lsan.md)
