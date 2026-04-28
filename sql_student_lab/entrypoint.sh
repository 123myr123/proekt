#!/bin/sh
set -eu

mkdir -p "$(dirname "$DB_PATH")"

if [ ! -f "$DB_PATH" ]; then
  echo "[entrypoint] Создаю базу: $DB_PATH"
  sqlite3 "$DB_PATH" < /app/sql/init.sql
else
  echo "[entrypoint] База уже есть: $DB_PATH (пропуск init.sql)"
fi

exec python3 /app/app/demo.py
