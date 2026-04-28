#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
COMPOSE_FILE="$ROOT/docker-compose.yml"

if ! command -v docker >/dev/null 2>&1; then
  echo "Docker CLI не найден. Установите Docker и повторите." >&2
  exit 1
fi

docker compose -f "$COMPOSE_FILE" down
