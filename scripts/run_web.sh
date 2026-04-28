#!/bin/bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
export IOT_DB_PATH="${IOT_DB_PATH:-$ROOT/data/device.db}"
cd "$ROOT/web"
exec python3 -m flask --app app run --host 0.0.0.0 --port "${PORT:-8080}"
