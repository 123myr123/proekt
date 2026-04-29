Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$Root = Split-Path -Parent $PSScriptRoot
$ComposeFile = Join-Path $Root "docker-compose.yml"
$EnvExample = Join-Path $Root ".env.example"
$EnvFile = Join-Path $Root ".env"

if (-not (Get-Command docker -ErrorAction SilentlyContinue)) {
    throw "Docker CLI не найден. Установите Docker Desktop и повторите."
}

docker info *> $null
if ($LASTEXITCODE -ne 0) {
    throw "Docker daemon недоступен. Запустите Docker Desktop и повторите."
}

if (-not (Test-Path $EnvFile) -and (Test-Path $EnvExample)) {
    Copy-Item $EnvExample $EnvFile
    Write-Host "Создан .env из .env.example"
}

docker compose -f $ComposeFile up --build -d

Write-Host ""
Write-Host "Тренировочный SQL-стенд запущен."
Write-Host "UI:  http://localhost:8090"
Write-Host "API: http://localhost:8090/api/summary"
