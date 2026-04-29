# Модуль данных SQLite в проекте `iot_device_full`

Учебный материал: **зачем** в проекте отдельная БД, **как устроена схема**, **как запись и чтение реализованы в коде** (C++ и Python), и **как продемонстрировать** это студентам на живом примере.

Связанные документы: [architecture.md](architecture.md) (поток данных), [theory.md](theory.md) (общий разбор кода).

---

## 1. Зачем здесь SQL и почему SQLite

- **Задача**: сохранять **историю снимков** состояния датчиков и выходов на каждом шаге главного цикла, чтобы потом смотреть их в браузере без перезапуска C++-демона.
- **SQLite** — встраиваемая СУБД: один **файл** на диске (`device.db`), без отдельного серверного процесса. Демон на C++ и веб на Python открывают **один и тот же файл** (см. раздел про WAL и конкурентный доступ).
- **Альтернатива** (клиент-сервер PostgreSQL/MySQL) для этой учебной установки избыточна: нужна установка сервиса, учёт сети и учётных записей. SQLite упрощает развёртывание на Raspberry Pi и в Docker.

Путь к файлу задаётся в конфигурации ядра и должен совпадать с путём, который использует Flask (переменная окружения `IOT_DB_PATH` или значение по умолчанию):

```cpp
// device_core/include/device_manager.h (стр. 26–27)
    std::string databasePath{"../data/device.db"};
```

```python
# web/app.py (стр. 9–12)
def _default_db_path():
    return os.path.join(_base_dir(), "..", "data", "device.db")

DB_PATH = os.environ.get("IOT_DB_PATH", _default_db_path())
```

---

## 2. Где в репозитории «живёт» SQL

| Артефакт | Назначение |
|----------|------------|
| `sql/schema.sql` | Эталонная DDL для ручного применения, демонстрации в `sqlite3`, сравнения с кодом. |
| `device_core/src/database.cpp` | При **каждом** `open()` создаётся таблица и индекс (если ещё нет), включается WAL, выполняется `INSERT` из `DeviceManager`. |
| `device_core/include/database.h` | Класс `Database`: `open`, `close`, `insertReading`. |
| `web/app.py` | Только **чтение**: `SELECT ... ORDER BY id DESC LIMIT ?`. |

Важно для лекции: **два представления одной схемы** — текстовый `schema.sql` и строки SQL внутри `database.cpp` — должны **совпадать** по именам таблиц, столбцов и типам; иначе веб и ядро разъедутся.

---

## 3. Концептуальная модель: один временной ряд

Логически это **одна таблица** `sensor_readings`: каждая строка — **один момент времени** (`recorded_at`) и **полный снимок** всех величин, которые тогда были в `DeviceStatus` (температура, влажность, расстояние, кнопка, LED, реле).

Соответствие полей структуре в C++:

```cpp
// device_core/include/device_manager.h (стр. 31–38)
struct DeviceStatus {
    float temperatureC{0.0f};
    float humidityPercent{0.0f};
    float distanceCm{0.0f};
    bool buttonActive{false};
    bool ledOn{false};
    bool relayOn{false};
};
```

В таблице добавлен суррогатный ключ `id` (автоинкремент) — удобно для сортировки «последние записи» в веб-интерфейсе без разбора строки времени.

### Описание столбцов таблицы `sensor_readings`

Ниже — **объекты** (атрибуты одной записи): что означает каждое поле, как оно заполняется и как к нему обращаются в приложении.

| Столбец | Тип SQLite | Обязательность | Описание | Откуда берётся значение |
|---------|------------|----------------|----------|-------------------------|
| `id` | `INTEGER PRIMARY KEY AUTOINCREMENT` | да, не в `INSERT` | Суррогатный ключ строки; монотонно растёт при новых вставках | Задаётся SQLite автоматически |
| `recorded_at` | `TEXT` | `NOT NULL` | Метка времени снимка в виде строки `YYYY-MM-DD HH:MM:SS` (локальное время процесса) | Аргумент `timestamp` в `Database::insertReading` из `DeviceManager::currentTimestamp()` |
| `temperature_c` | `REAL` | `NOT NULL` | Температура воздуха, °C | `DeviceStatus::temperatureC` после `DhtSensor::read()` (в учебной сборке — случайные значения) |
| `humidity_percent` | `REAL` | `NOT NULL` | Влажность, % | `DeviceStatus::humidityPercent` (тот же DHT-заглушка) |
| `distance_cm` | `REAL` | `NOT NULL` | Расстояние до препятствия, см | `DeviceStatus::distanceCm` после `UltrasonicSensor::read()` (или имитация) |
| `button_active` | `INTEGER` | `NOT NULL` | Логическое «кнопка активна»: **1** — да, **0** — нет | `DeviceStatus::buttonActive` (`DigitalSensor::isActive()`) |
| `led_on` | `INTEGER` | `NOT NULL` | Состояние выхода LED: **1** — включено, **0** — выключено | `DeviceStatus::ledOn` после успешного `LedActuator::setState` |
| `relay_on` | `INTEGER` | `NOT NULL` | Состояние реле: **1** — включено, **0** — выключено | `DeviceStatus::relayOn` после успешного `RelayActuator::setState` (логика порога в `DeviceManager`) |

**Связь с `DeviceStatus`:** поля `temperature_c` … `relay_on` по смыслу совпадают с членами структуры `DeviceStatus` в `device_manager.h`; при записи выполняется приведение `bool` → `0`/`1` и `float` → `REAL` через `sqlite3_bind_*`.

**Во Flask и Jinja2:** объекты строки доступны как ключи словаря/строки `Row`: `temperature_c`, `humidity_percent`, `distance_cm`, `button_active`, `led_on`, `relay_on` (подчёркивания, как в SQL).

Краткая сводка для обзора архитектуры — также в [architecture.md](architecture.md) (раздел «Таблица sensor_readings: объекты (столбцы)»).

---

## 4. DDL: таблица `sensor_readings` (разбор синтаксиса SQL)

Фрагмент из эталонного файла:

```sql
-- sql/schema.sql (стр. 3–12)
CREATE TABLE IF NOT EXISTS sensor_readings (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    recorded_at TEXT NOT NULL,
    temperature_c REAL NOT NULL,
    humidity_percent REAL NOT NULL,
    distance_cm REAL NOT NULL,
    button_active INTEGER NOT NULL,
    led_on INTEGER NOT NULL,
    relay_on INTEGER NOT NULL
);
```

- **`CREATE TABLE IF NOT EXISTS`** — создать таблицу только если её ещё нет; повторный запуск скрипта или `open()` не ломает существующую БД.
- **`sensor_readings`** — имя таблицы; в проекте оно же используется в `INSERT` и `SELECT`.
- **`id INTEGER PRIMARY KEY AUTOINCREMENT`** — целочисленный первичный ключ; в SQLite так задаётся **автоувеличение** нового `id` при вставке без явного значения `id`.
- **`TEXT NOT NULL`** — строка без NULL; время хранится как текст (формируется в C++ функцией `currentTimestamp()` в виде `YYYY-MM-DD HH:MM:SS`).
- **`REAL NOT NULL`** — числа с плавающей точкой (температура, влажность, расстояние); в C++ это `float`, при записи приводится к `double` для API SQLite.
- **`INTEGER NOT NULL` для логических полей** — в SQLite нет отдельного типа BOOLEAN; принято хранить **0/1**. В C++ `bool` преобразуется при `sqlite3_bind_int`.

Тот же DDL встроен в код открытия БД:

```cpp
// device_core/src/database.cpp (стр. 46–56)
    const char* ddl =
        "CREATE TABLE IF NOT EXISTS sensor_readings ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "recorded_at TEXT NOT NULL,"
        "temperature_c REAL NOT NULL,"
        "humidity_percent REAL NOT NULL,"
        "distance_cm REAL NOT NULL,"
        "button_active INTEGER NOT NULL,"
        "led_on INTEGER NOT NULL,"
        "relay_on INTEGER NOT NULL"
        ");";
```

---

## 5. Индекс по `recorded_at`

```sql
-- sql/schema.sql (стр. 14–14)
CREATE INDEX IF NOT EXISTS idx_sensor_readings_recorded_at ON sensor_readings (recorded_at);
```

- **Назначение индекса** — ускорить запросы с фильтром или сортировкой по столбцу `recorded_at`, когда таблица вырастет.
- **`IF NOT EXISTS`** — идемпотентное создание при повторных запусках.
- В текущем **веб-коде** выборка идёт по `ORDER BY id DESC` (см. ниже), а не по `recorded_at`; индекс по времени **пригодится** для отчётов «за интервал дат» или графиков — это хороший повод обсудить с студентами **какой индекс нужен под какой запрос**.

---

## 6. `PRAGMA foreign_keys` в `schema.sql`

```sql
-- sql/schema.sql (стр. 1–1)
PRAGMA foreign_keys = ON;
```

В учебной схеме **нет внешних ключей** между таблицами (таблица одна). Директива включена как **навык**: при появлении второй таблицы с `REFERENCES` SQLite будет проверять ссылочную целостность только если `foreign_keys` включён. В `database.cpp` этот `PRAGMA` **не** выполняется — при желании его можно добавить в `open()` рядом с WAL для полного совпадения с `schema.sql`.

---

## 7. Режим журнала WAL (Write-Ahead Logging)

```cpp
// device_core/src/database.cpp (стр. 43–45)
    if (!execSql(db, "PRAGMA journal_mode=WAL;")) {
        return false;
    }
```

**Идея**: при WAL записи сначала попадают в отдельный журнал; читатели могут читать согласованный снимок основного файла без длительной блокировки всего файла. Для сценария «**один процесс пишет** (C++), **другой читает** (Flask)» это типичный и устойчивый режим.

На лекции можно пояснить: это не «магия», а конкретный алгоритм SQLite; при сбое питания возможности восстановления определяются документацией SQLite.

---

## 8. Запись данных: класс `Database` и C API

### 8.1. Инкапсуляция дескриптора

В заголовке дескриптор хранится как `void*`, чтобы не тянуть `sqlite3.h` в каждый модуль:

```cpp
// device_core/include/database.h (стр. 17–21)
    bool insertReading(const DeviceStatus& status, const std::string& timestamp);

private:
    std::string path_;
    void* dbHandle_{nullptr};
```

В `.cpp` выполняется приведение к `sqlite3*`.

### 8.2. Открытие файла

```cpp
// device_core/src/database.cpp (стр. 38–42)
    sqlite3* db = nullptr;
    if (sqlite3_open(path_.c_str(), &db) != SQLITE_OK) {
        return false;
    }
    dbHandle_ = db;
```

`sqlite3_open` создаёт файл БД по пути, если его ещё нет (при успешном открытии).

### 8.3. Выполнение произвольного SQL без результата (`execSql`)

Для `PRAGMA`, `CREATE TABLE`, `CREATE INDEX` используется `sqlite3_exec`:

```cpp
// device_core/src/database.cpp (стр. 10–18)
bool execSql(sqlite3* db, const char* sql) {
    char* err = nullptr;
    if (sqlite3_exec(db, sql, nullptr, nullptr, &err) != SQLITE_OK) {
        if (err) {
            sqlite3_free(err);
        }
        return false;
    }
    return true;
}
```

Сообщение об ошибке выделяется SQLite в буфер `err`; его нужно освободить через `sqlite3_free`.

### 8.4. Подготовленный запрос и параметры `?`

Вставка строки делается через **подготовленное выражение** — SQL компилируется один раз, затем многократно выполняется с разными значениями:

```cpp
// device_core/src/database.cpp (стр. 73–89)
    const char* sql =
        "INSERT INTO sensor_readings (recorded_at, temperature_c, humidity_percent, distance_cm, "
        "button_active, led_on, relay_on) VALUES (?,?,?,?,?,?,?)";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, static_cast<int>(std::strlen(sql)), &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    sqlite3_bind_text(stmt, 1, timestamp.c_str(), static_cast<int>(timestamp.size()), SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 2, static_cast<double>(status.temperatureC));
    sqlite3_bind_double(stmt, 3, static_cast<double>(status.humidityPercent));
    sqlite3_bind_double(stmt, 4, static_cast<double>(status.distanceCm));
    sqlite3_bind_int(stmt, 5, status.buttonActive ? 1 : 0);
    sqlite3_bind_int(stmt, 6, status.ledOn ? 1 : 0);
    sqlite3_bind_int(stmt, 7, status.relayOn ? 1 : 0);
    const int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
```

Пояснения для аудитории:

| Шаг API | Смысл |
|--------|--------|
| `sqlite3_prepare_v2` | Скомпилировать SQL в `sqlite3_stmt`. |
| `sqlite3_bind_*` | Подставить значения вместо `?` **по позиции** (1 — первый параметр). |
| `SQLITE_TRANSIENT` для текста | SQLite скопирует строку метки времени до завершения шага. |
| `sqlite3_step` | Выполнить один шаг; для `INSERT` успех — код `SQLITE_DONE`. |
| `sqlite3_finalize` | Освободить подготовленный statement. |

**Почему не склеивать SQL строкой из чисел?** Так делают только в небезопасных примерах. Плейсхолдеры `?` исключают **SQL-инъекцию** и корректно экранируют данные.

Точка вызова из логики устройств — после каждого цикла опроса:

```cpp
// device_core/src/device_manager.cpp (стр. 205–207)
        if (database_ && !database_->insertReading(status_, currentTimestamp())) {
            logError("SQLite insert failed");
        }
```

---

## 9. Чтение данных: Flask и модуль `sqlite3` в Python

```python
# web/app.py (стр. 16–28)
def fetch_readings(limit):
    if not os.path.isfile(DB_PATH):
        return []
    conn = sqlite3.connect(DB_PATH)
    conn.row_factory = sqlite3.Row
    cur = conn.execute(
        "SELECT id, recorded_at, temperature_c, humidity_percent, distance_cm, "
        "button_active, led_on, relay_on FROM sensor_readings ORDER BY id DESC LIMIT ?",
        (limit,),
    )
    rows = [dict(r) for r in cur.fetchall()]
    conn.close()
    return rows
```

- **`ORDER BY id DESC`** — сначала **самые новые** записи (при монотонном росте `id`).
- **`LIMIT ?`** — ограничение числа строк; главная страница берёт 100, API — 200 (см. вызовы `fetch_readings` в `app.py`).
- **`sqlite3.Row`** — доступ к колонкам по имени в шаблоне: `r.temperature_c` и т.д.
- Параметр `(limit,)` — кортеж с одним элементом для единственного `?` в запросе.

Студентам можно показать, что **ни одна** строка в этом запросе не собирается конкатенацией из ввода пользователя — только целое `limit` из кода.

---

## 10. Согласованность ролей: кто пишет, кто читает

| Процесс | Операции |
|---------|----------|
| `iot_device_core` | `INSERT` на каждой итерации цикла. |
| Flask | `SELECT` последних записей. |

Оба открывают один файл; WAL облегчает одновременное чтение во время записи. Если запустить **два** писателя в один файл без координации, возможны блокировки и ошибки `SQLITE_BUSY` — в учебном проекте это **не предусмотрено** и его стоит явно оговорить.

---

## 11. Как показать работу студентам (практика)

1. **Запустить** `iot_device_core` с рабочим каталогом, где лежит `config.json` и доступен путь к `data/device.db`.
2. **Запустить** веб (из корня проекта удобно через `scripts/run_web.sh`, чтобы выставился `IOT_DB_PATH`).
3. В терминале открыть БД консольной утилитой SQLite:

```bash
sqlite3 path/to/data/device.db
```

Примеры запросов для демонстрации:

```sql
.tables
.schema sensor_readings
SELECT COUNT(*) FROM sensor_readings;
SELECT * FROM sensor_readings ORDER BY id DESC LIMIT 5;
```

4. Показать файлы рядом с `device.db` в режиме WAL (например, `device.db-wal`, `device.db-shm`) после первых записей — связь с `PRAGMA journal_mode=WAL`.

---

## 12. Краткий чек-лист для проверки понимания

- Почему для логических полей в SQLite используется `INTEGER`, а не отдельный тип «boolean».
- Чем отличается `sqlite3_exec` от связки `prepare` / `bind` / `step` / `finalize`.
- Зачем в `INSERT` используются знаки `?` и функции `sqlite3_bind_*`.
- Почему веб сортирует по `id DESC`, а индекс создан по `recorded_at`.
- Как путь к одному файлу БД задаётся в C++ и переопределяется во Flask через `IOT_DB_PATH`.

---

Дополнительный общий разбор синтаксиса C++ и места модуля в архитектуре — в [theory.md](theory.md), разделы про `database.cpp` и SQL.
