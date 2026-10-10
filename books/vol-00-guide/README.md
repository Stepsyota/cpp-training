# Том 0 — Как пользоваться курсом

**Версия черновика:** 2026-10-10

Отдельный EPUB: навигация по серии томов, репозиторию `training`, сборка mdBook и чтение на Kindle Paperwhite.

## Обложка

Не обязательна. На Kindle достаточно **уникального `title`** в `book.toml` каждого тома
(например: «C++ Training — Том 3: Многопоточность…») — так тома различаются в списке книг.

## Установка mdBook (Fedora)

**Вариант A — из репозитория (только HTML, проще всего):**

```bash
sudo dnf install mdbook
mdbook --version
```

**Вариант B — Rust + EPUB (нужен плагин `mdbook-epub`):**

```bash
sudo dnf install rust cargo
cargo install mdbook mdbook-epub
# бинарники в ~/.cargo/bin — добавь в PATH, если ещё нет:
echo 'export PATH="$HOME/.cargo/bin:$PATH"' >> ~/.bashrc
source ~/.bashrc
mdbook --version
mdbook epub --help
```

На Fedora пакет `mdbook` есть в `dnf`; **mdbook-epub** в репозитории обычно нет — его ставят через `cargo install mdbook-epub` (или оба инструмента через cargo, вариант B).

Проверка EPUB после установки:

```bash
export PATH="$HOME/.cargo/bin:$PATH"   # если mdbook-epub ставили через cargo
which mdbook mdbook-epub               # оба должны находиться
cd books/vol-00-guide
mdbook build
ls book/epub/*.epub
```

## Зависимости (кратко)

- [mdBook](https://rust-lang.github.io/mdBook/) — `dnf install mdbook` или `cargo install mdbook`
- EPUB: [mdbook-epub](https://github.com/Michael-F-Bryan/mdbook-epub) — `cargo install mdbook-epub` (версию подбирай под `mdbook --version`, сейчас 0.5.x)

## Сборка

В **mdBook 0.5** нет команды `mdbook epub`. EPUB собирается так:

1. В `book.toml` есть секция `[output.epub]` (уже есть).
2. В `PATH` доступен бинарник `mdbook-epub` (плагин).
3. Одна команда:

```bash
cd books/vol-00-guide
mdbook build
```

Альтернатива только EPUB (без HTML):

```bash
mdbook-epub -s .
# из корня тома; см. mdbook-epub --help
```

- HTML: `book/html/index.html` (в 0.5 каталог может называться `book/html/`)
- EPUB: `book/epub/<название из title>.epub` — в `title` нельзя `:` и `+` (имя файла); см. комментарий в `book.toml`

Проверка локально: любой ридер EPUB или `foliate`, Calibre.

## Структура

- `book.toml` — метаданные тома
- `src/SUMMARY.md` — оглавление
- `src/ch*.md` — главы
