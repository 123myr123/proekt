#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
COMPOSE_FILE="$ROOT/docker/docker-compose.yml"

if ! command -v docker >/dev/null 2>&1; then
  echo "Docker CLI not found. Install Docker and retry." >&2
  exit 1
fi

docker compose -f "$COMPOSE_FILE" down
