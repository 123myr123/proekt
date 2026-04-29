#!/usr/bin/env sh
set -eu

DB_PATH="${DB_PATH:-/workspace/data/training.db}"
PORT="${PORT:-8090}"

mkdir -p "$(dirname "$DB_PATH")"

if [ ! -f "$DB_PATH" ]; then
  echo "Initializing SQLite database at $DB_PATH"
  sqlite3 "$DB_PATH" < /workspace/sql/init.sql
fi

export DB_PATH

exec python -m flask --app /workspace/app/app.py run --host 0.0.0.0 --port "$PORT"
