# Готовые EPUB (все тома в одной папке)

Сюда складываются собранные файлы после `mdbook build` по всей серии. Исходники томов по-прежнему в `books/vol-XX-.../book/epub/`; эта папка — удобная «полка» для Send to Kindle и бэкапа.

## Обновить все книги

Из **корня репозитория** `training`:

```bash
./books/sync-epub.sh
```

Скрипт собирает каждый том (`mdbook build`) и копирует `*.epub` сюда. Нужны `mdbook` и `mdbook-epub` в `PATH` (см. `vol-00-guide/README.md`).

## Только скопировать (без пересборки)

```bash
./books/sync-epub.sh --copy-only
```

## Имена файлов

Имя EPUB задаётся полем `title` в `book.toml` тома (без `:` и `+` в title).

Файлы `*.epub` в git не коммитятся (см. `.gitignore` в этой папке).
