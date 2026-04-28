#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
COMPOSE_FILE="$ROOT/docker-compose.yml"
ENV_EXAMPLE="$ROOT/.env.example"
ENV_FILE="$ROOT/.env"

if ! command -v docker >/dev/null 2>&1; then
  echo "Docker CLI не найден. Установите Docker и повторите." >&2
  exit 1
fi

if ! docker info >/dev/null 2>&1; then
  echo "Docker daemon недоступен. Запустите Docker и повторите." >&2
  exit 1
fi

if [ ! -f "$ENV_FILE" ] && [ -f "$ENV_EXAMPLE" ]; then
  cp "$ENV_EXAMPLE" "$ENV_FILE"
  echo "Создан .env из .env.example"
fi

docker compose -f "$COMPOSE_FILE" up --build -d

echo
echo "Тренировочный SQL-стенд запущен."
echo "UI:  http://localhost:8090"
echo "API: http://localhost:8090/api/summary"
