# iot_device_full

Учебный IoT-проект: C++-ядро собирает телеметрию и пишет ее в SQLite, Flask показывает данные в web-интерфейсе.  
Для студентов основной сценарий запуска — через Docker.

## Быстрый старт (рекомендуется)

### Windows (PowerShell)

Из корня проекта:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\docker_up.ps1
```

После старта:
- Web UI: `http://localhost:8080`
- noVNC: `http://localhost:6080/vnc.html`

Остановить:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\docker_down.ps1
```

### Linux/macOS

Из корня проекта:

```bash
bash ./scripts/docker_up.sh
```

После старта:
- Web UI: `http://localhost:8080`
- noVNC: `http://localhost:6080/vnc.html`

Остановить:

```bash
bash ./scripts/docker_down.sh
```

## Что делает Docker-сценарий

- Собирает образ из `docker/Dockerfile`.
- Поднимает контейнер по `docker/docker-compose.yml`.
- Автоматически создает `docker/.env` из `docker/.env.example`, если файла нет.
- Внутри контейнера запускаются:
  - C++ core (`iot_device_core`);
  - Flask web (`web/app.py`);
  - VNC + noVNC для GUI-лаборатории.

## Базовые команды Docker вручную

Если нужно запускать без скриптов:

```bash
docker compose -f docker/docker-compose.yml up --build -d
docker compose -f docker/docker-compose.yml logs -f
docker compose -f docker/docker-compose.yml down
```

## Частые проблемы

- **Docker daemon недоступен**  
  Запустите Docker Desktop / Docker Engine и повторите команду.

- **Порты заняты (`6080` или `8080`)**  
  Освободите порты или измените проброс в `docker/docker-compose.yml`.

- **Долго собирается образ при первом запуске**  
  Это нормально: устанавливаются системные зависимости и собирается C++-часть.

## Структура проекта (кратко)

- `device_core/` — C++ ядро устройства.
- `web/` — Flask-интерфейс и API.
- `sql/` — схема базы данных.
- `docker/` — Docker-инфраструктура запуска.
- `scripts/` — кросс-платформенные команды запуска/остановки.
