# Тренировочный SQL-стенд

Автономный тренировочный стенд для изучения основ SQL и типовых запросов.

Стенд полностью изолирован:
- своя Docker-конфигурация;
- свой код приложения;
- своя база SQLite и схема;
- нет runtime-зависимостей от других папок репозитория.

## Цель

Студенты могут:
- разобрать простую, но реалистичную схему данных;
- выполнить примеры `SELECT`/`JOIN`/`GROUP BY`/транзакций;
- увидеть результаты запросов в браузере и через API;
- перенести этот Docker-подход в собственные проекты.

## Быстрый старт

Откройте терминал в `sql_training_stand`.

### Windows (PowerShell)

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\up.ps1
```

Остановка:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\down.ps1
```

### Linux/macOS

```bash
bash ./scripts/up.sh
```

Остановка:

```bash
bash ./scripts/down.sh
```

## Адреса

- UI: `http://localhost:8090`
- API: `http://localhost:8090/api/summary`

## Файлы для практики

- Схема и начальные данные: `sql/init.sql`
- Примеры запросов: `sql/practice_queries.sql`
- Подробный теоретический туториал: `docs/TUTORIAL_RU.md`
- Диаграмма структуры проекта (draw.io): `docs/sql_training_stand_structure.drawio`

## Ручные команды Docker

```bash
docker compose up --build -d
docker compose logs -f
docker compose down
```
