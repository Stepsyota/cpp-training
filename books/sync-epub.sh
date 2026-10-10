#!/usr/bin/env bash
# Собрать все тома mdBook и скопировать EPUB в books/epub/
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
DEST="$ROOT/books/epub"
COPY_ONLY=false

if [[ "${1:-}" == "--copy-only" ]]; then
  COPY_ONLY=true
fi

export PATH="${HOME}/.cargo/bin:${PATH}"

mkdir -p "$DEST"

for vol in "$ROOT"/books/vol-*/; do
  name="$(basename "$vol")"
  if [[ ! -f "$vol/book.toml" ]]; then
    continue
  fi
  echo "=== $name ==="
  if [[ "$COPY_ONLY" == false ]]; then
    (cd "$vol" && mdbook build)
  fi
  shopt -s nullglob
  epubs=("$vol"/book/epub/*.epub)
  shopt -u nullglob
  if [[ ${#epubs[@]} -eq 0 ]]; then
    echo "  нет EPUB в $vol/book/epub/ (нужен mdbook-epub?)" >&2
    exit 1
  fi
  for f in "${epubs[@]}"; do
    cp -f "$f" "$DEST/"
    echo "  -> $(basename "$f")"
  done
done

echo ""
echo "Готово: $DEST"
ls -1 "$DEST"/*.epub 2>/dev/null || true
