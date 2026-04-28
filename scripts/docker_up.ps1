Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$Root = Split-Path -Parent $PSScriptRoot
$DockerDir = Join-Path $Root "docker"
$ComposeFile = Join-Path $DockerDir "docker-compose.yml"
$EnvExample = Join-Path $DockerDir ".env.example"
$EnvFile = Join-Path $DockerDir ".env"

if (-not (Get-Command docker -ErrorAction SilentlyContinue)) {
    throw "Docker CLI не найден. Установите Docker Desktop и повторите."
}

docker info *> $null
if ($LASTEXITCODE -ne 0) {
    throw "Docker daemon недоступен. Запустите Docker Desktop и повторите."
}

if (-not (Test-Path $EnvFile) -and (Test-Path $EnvExample)) {
    Copy-Item $EnvExample $EnvFile
    Write-Host "Создан docker/.env из docker/.env.example"
}

docker compose -f $ComposeFile up --build -d

Write-Host ""
Write-Host "Контейнер запущен."
Write-Host "Web UI:   http://localhost:8080"
Write-Host "noVNC:    http://localhost:6080/vnc.html"
Write-Host "Логи:     docker compose -f docker/docker-compose.yml logs -f"
