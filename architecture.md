# Архитектура программного комплекса `iot_device_full`

## Назначение

Единый учебный проект для Raspberry Pi: низкоуровневое чтение датчиков и управление исполнительными устройствами на C++ ([libgpiod](https://git.kernel.org/pub/scm/libs/libgpiod/libgpiod.git/)), сохранение показаний в SQLite и просмотр истории через веб-интерфейс на Python (Flask). Лабораторная среда Docker повторяет стек сборки без реального GPIO (режим `simulate_hardware` в `device_core/config.json` и путь к БД).

## Каталоги верхнего уровня

| Каталог | Содержание |
|--------|------------|
| `device_core/` | Исполняемый модуль `iot_device_core`: точка входа `main.cpp`, координация в `DeviceManager`, драйверы датчиков и актуаторов, слой `database.cpp` для записи в SQLite. Сборка — в `CMakeLists.txt` (см. [пример CMake](#code-examples)). |
| `web/` | Веб-приложение Flask: маршруты `/` и `/api/readings`, шаблон `templates/index.html` (см. [Flask](#code-examples)). |
| `sql/` | `schema.sql` — эталонная схема таблицы и индекса (дублируется в `database.cpp` при открытии БД). Подробно: [`sql.md`](sql.md). |
| `data/` | Рабочий каталог для `device.db`; путь задаётся в `config.json` как `database_path` (по умолчанию `../data/device.db` относительно рабочего каталога процесса, см. [конфигурацию по умолчанию](#code-examples)). |
| `deploy/` | Примеры unit-файлов systemd для служб ядра и веб-интерфейса на целевой системе. |
| `docker/` | `Dockerfile` лабораторного контейнера (VNC, noVNC, сборка) и `docker-compose.yml` (см. [docker-compose](#code-examples)). |
| `scripts/` | `run_web.sh` — запуск веб-сервера с `IOT_DB_PATH` (см. [скрипт](#code-examples)). |

## Поток данных

1. **`iot_device_core`** загружает `config.json`, при необходимости проверяет доступ к GPIO-чипу, инициализирует датчики и актуаторы, открывает SQLite по `database_path`.
2. В **бесконечном цикле**: опрос DHT (в учебной сборке — случайные значения), ультразвука, кнопки; LED по кнопке; реле по порогу расстояния; лог в stdout; вставка строки в `sensor_readings`.
3. **Flask** читает тот же файл БД; режим **WAL** допускает запись из C++ и чтение из Python.

## Таблица `sensor_readings`: объекты (столбцы)

Каждая **строка** — один снимок состояния на момент записи; **объекты** (поля) хранят идентификатор записи, метку времени и копию показаний/выходов из `DeviceStatus`. Развёрнутое описание столбцов и типов SQLite — в [`sql.md`](sql.md) (подраздел *«Описание столбцов таблицы sensor_readings»* внутри раздела 3).

| Столбец (SQL) | Тип в БД | Смысл | Источник в коде (C++) | Имя во Flask / шаблоне |
|----------------|----------|--------|------------------------|-------------------------|
| `id` | `INTEGER` PK, автоинкремент | Номер записи по порядку вставки | Генерируется SQLite | `r.id` (в выборке есть) |
| `recorded_at` | `TEXT` | Локальное время записи строки | `currentTimestamp()` при `insertReading` | `r.recorded_at` |
| `temperature_c` | `REAL` | Температура, °C (учебная заглушка DHT) | `status_.temperatureC` | `r.temperature_c` |
| `humidity_percent` | `REAL` | Относительная влажность, % | `status_.humidityPercent` | `r.humidity_percent` |
| `distance_cm` | `REAL` | Расстояние по ультразвуку, см | `status_.distanceCm` | `r.distance_cm` |
| `button_active` | `INTEGER` 0/1 | Кнопка нажата (логический уровень с учётом `activeHigh`) | `status_.buttonActive` | `r.button_active` |
| `led_on` | `INTEGER` 0/1 | Состояние LED после `setState` | `status_.ledOn` | `r.led_on` |
| `relay_on` | `INTEGER` 0/1 | Состояние реле (порог по `relay_distance_threshold_cm`) | `status_.relayOn` | `r.relay_on` |

Имена столбцов в Python — **snake_case** (как в SQL); во встроенном C++ они совпадают с именами в строке `INSERT`, задаваемой в `database.cpp`.

## Зависимости сборки (Linux / Raspberry Pi)

- CMake 3.14+, компилятор **C++17**.
- **libgpiod**, **jsoncpp**, **SQLite3** — см. `CMakeLists.txt` в [примерах](#code-examples).
- Python 3 и пакеты из `web/requirements.txt`.

## Связь с исходными проектами

- `device_core` — учебное ядро с GPIO и конфигурацией.
- `docker/` — образ лаборатории с предустановленными зависимостями и предсборкой (фрагмент `Dockerfile` в [примерах](#code-examples)).

---

## Ссылки на документацию по коду

- Схема БД, WAL, запись и чтение: [`sql.md`](sql.md).
- Разбор синтаксиса и API: [`theory.md`](theory.md).

---

<a id="code-examples"></a>

## Примеры кода

Фрагменты для сопоставления с репозиторием; в редакторах включается подсветка синтаксиса по языку блока.

### CMake (`device_core/CMakeLists.txt`)

```cmake
cmake_minimum_required(VERSION 3.14)

project(iot_device_core LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(SOURCES
    src/main.cpp
    src/database.cpp
    src/device_manager.cpp
    src/dht_sensor.cpp
    src/ultrasonic_sensor.cpp
    src/digital_sensor.cpp
    src/led_actuator.cpp
    src/relay_actuator.cpp
)

add_executable(iot_device_core ${SOURCES})

target_include_directories(iot_device_core
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

find_package(PkgConfig REQUIRED)
find_package(SQLite3 REQUIRED)
pkg_check_modules(GPIOD REQUIRED libgpiod)
pkg_check_modules(JSONCPP REQUIRED jsoncpp)

target_include_directories(iot_device_core
    PRIVATE
        ${GPIOD_INCLUDE_DIRS}
        ${JSONCPP_INCLUDE_DIRS}
)

target_link_libraries(iot_device_core
    PRIVATE
        SQLite::SQLite3
        ${GPIOD_LIBRARIES}
        ${JSONCPP_LIBRARIES}
)

target_compile_options(iot_device_core
    PRIVATE
        ${GPIOD_CFLAGS_OTHER}
        ${JSONCPP_CFLAGS_OTHER}
        -Wall
        -Wextra
        -Wpedantic
)

install(TARGETS iot_device_core RUNTIME DESTINATION bin)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/config.json DESTINATION etc/iot_device_core)
```

### Flask (`web/app.py`)

```python
@app.route("/")
def index():
    rows = fetch_readings(100)
    return render_template("index.html", rows=rows)

@app.route("/api/readings")
def api_readings():
    return jsonify(fetch_readings(200))
```

### DDL и индекс (`device_core/src/database.cpp`)

```cpp
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
    // ...
    const char* idx =
        "CREATE INDEX IF NOT EXISTS idx_sensor_readings_recorded_at ON sensor_readings (recorded_at);";
```

### Конфигурация по умолчанию (`device_core/include/device_manager.h`)

```cpp
struct DeviceConfig {
    std::string gpioChip{"gpiochip0"};
    // ...
    std::string databasePath{"../data/device.db"};
    bool simulateHardware{false};
};
```

### docker-compose (`docker/docker-compose.yml`)

```yaml
services:
  iot-lab:
    build:
      context: ..
      dockerfile: docker/Dockerfile
    container_name: iot_device_full_lab
    ports:
      - "6080:6080"
      - "8080:8080"
    environment:
      VNC_PASSWORD: ${VNC_PASSWORD:-changeme}
      VNC_RESOLUTION: ${VNC_RESOLUTION:-1600x900}
    volumes:
      - student_workspace:/home/student/workspace
    shm_size: "256mb"
    restart: unless-stopped

volumes:
  student_workspace:
```

### Запуск веба (`scripts/run_web.sh`)

```bash
#!/bin/bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
export IOT_DB_PATH="${IOT_DB_PATH:-$ROOT/data/device.db}"
cd "$ROOT/web"
exec python3 -m flask --app app run --host 0.0.0.0 --port "${PORT:-8080}"
```

### Точка входа и GPIO (`device_core/src/main.cpp`)

```cpp
int main(int argc, char* argv[]) {
    std::string configPath = "config.json";
    if (argc > 1) {
        configPath = argv[1];
    }
    // ...
    if (!manager.getConfig().simulateHardware) {
        if (!hasGpioAccess(manager.getConfig().gpioChip)) {
            logError("Insufficient permissions to access GPIO. Exiting.");
            return 1;
        }
    }
```

### Инициализация и цикл (`device_core/src/device_manager.cpp`)

```cpp
bool DeviceManager::initDevices() {
    // ...
    database_ = std::make_unique<Database>(config_.databasePath);
    if (!database_->open()) {
        logError("Failed to open SQLite database: " + config_.databasePath);
        return false;
    }
```

```cpp
void DeviceManager::runMainLoop() {
    while (true) {
        if (dht_->read()) {
            status_.temperatureC = dht_->getTemperatureC();
            // ...
```

### Имитация датчиков

`device_core/src/dht_sensor.cpp`:

```cpp
bool DhtSensor::read() {
    lastTemperatureC_ = randomInRange(20.0f, 30.0f);
    lastHumidity_ = randomInRange(30.0f, 70.0f);
    return true;
}
```

`device_core/src/ultrasonic_sensor.cpp`:

```cpp
    if (simulate_) {
        lastDistanceCm_ = randomCm(5.0f, 90.0f);
        return true;
    }
```

`device_core/src/digital_sensor.cpp`:

```cpp
    if (simulate_) {
        static std::mt19937 rng{std::random_device{}()};
        std::bernoulli_distribution dist(0.4);
        lastValue_ = dist(rng);
        return true;
    }
```

### WAL (`device_core/src/database.cpp`)

```cpp
    if (!execSql(db, "PRAGMA journal_mode=WAL;")) {
        return false;
    }
```

### Фрагмент `Dockerfile` (установка и сборка)

```dockerfile
RUN bash -c 'apt-get update; \
for attempt in 1 2 3; do \
  if apt-get install -y \
    libgpiod-dev \
    libjsoncpp-dev \
    libsqlite3-dev \
  ; then \
    rm -rf /var/lib/apt/lists/*; exit 0; \
  fi; \
  sleep 45; \
  apt-get update; \
done; \
exit 1'
# ... pip3 install, cmake --build ...
```
