#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
DOCKER_DIR="$ROOT/docker"
COMPOSE_FILE="$DOCKER_DIR/docker-compose.yml"
ENV_EXAMPLE="$DOCKER_DIR/.env.example"
ENV_FILE="$DOCKER_DIR/.env"

if ! command -v docker >/dev/null 2>&1; then
  echo "Docker CLI not found. Install Docker and retry." >&2
  exit 1
fi

if ! docker info >/dev/null 2>&1; then
  echo "Docker daemon is not available. Start Docker and retry." >&2
  exit 1
fi

if [ ! -f "$ENV_FILE" ] && [ -f "$ENV_EXAMPLE" ]; then
  cp "$ENV_EXAMPLE" "$ENV_FILE"
  echo "Created docker/.env from docker/.env.example"
fi

docker compose -f "$COMPOSE_FILE" up --build -d

echo
echo "Container started."
echo "Web UI:   http://localhost:8080"
echo "noVNC:    http://localhost:6080/vnc.html"
echo "Logs:     docker compose -f docker/docker-compose.yml logs -f"
