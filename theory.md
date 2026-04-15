# Теория и разбор кода `iot_device_full`

Документ описывает теоретические основы, **используемый синтаксис** (C++17, SQLite C API, Python/Flask, Jinja2, SQL, CMake) и привязку к фрагментам репозитория. Примеры кода оформлены обычными блоками Markdown с подсветкой языка; источник указан в первой строке блока (`//` для C++, `--` для SQL и т.д.). Полное описание SQL-модуля для лекций: [`sql.md`](sql.md).

---

## 1. Общая идея

Программа разделена на **два процесса**: демон на C++ взаимодействует с «железом» (или имитацией) и пишет снимки состояния в SQLite; веб-сервер на Python **только читает** БД и отдаёт HTML/JSON. Граница «реальное время + GPIO» / «представление в браузере» явно выражена в архитектуре[^split].

[^split]:

Запись в БД из ядра (`device_core/src/device_manager.cpp`, стр. 205–207):

```cpp
        if (database_ && !database_->insertReading(status_, currentTimestamp())) {
            logError("SQLite insert failed");
        }
```

Чтение во Flask (`web/app.py`, стр. 16–28):

```python
def fetch_readings(limit):
    if not os.path.isfile(DB_PATH):
        return []
    conn = sqlite3.connect(DB_PATH)
```

---

## 2. Взаимосвязь файлов

| Файл | Роль |
|------|------|
| `device_core/config.json` | Пины BCM, интервал опроса, `database_path`, `simulate_hardware`[^cfgjson]. |
| `device_core/src/device_manager.cpp` | Разбор JSON, создание `unique_ptr` датчиков/актуаторов и `Database`, главный цикл[^dm]. |
| `device_core/src/database.cpp` | `sqlite3_open`, WAL, DDL, подготовленный `INSERT`[^db]. |
| `sql/schema.sql` | Документация схемы для ручного применения или обучения SQL[^sqlfile]. |
| `web/app.py` | Путь к БД: `IOT_DB_PATH` или значение по умолчанию рядом с `data/`[^dbpathpy]. |
| `web/templates/index.html` | Таблица и цикл Jinja2 по `rows`[^template]. |

**Объекты строки в БД:** столбцы таблицы `sensor_readings` (что хранится, связь с `DeviceStatus`, имена во Flask) сведены в [architecture.md](architecture.md) и подробно разобраны в [sql.md](sql.md) (раздел 3, подраздел про описание столбцов).

[^cfgjson]: *Файл `device_core/config.json` (стр. 1–13):*

```json
{
  "simulate_hardware": true,
  "gpio_chip": "gpiochip0",
  "database_path": "../data/device.db"
}
```

[^dm]:

```cpp
// device_core/src/device_manager.cpp (стр. 45–108)
bool DeviceManager::loadConfig(const std::string& path) {
    std::ifstream in(path);
    ...
    Json::Value root;
    in >> root;
```

[^db]:

```cpp
// device_core/src/database.cpp (стр. 36–66)
bool Database::open() {
    close();
    sqlite3* db = nullptr;
    if (sqlite3_open(path_.c_str(), &db) != SQLITE_OK) {
```

[^sqlfile]:

```sql
-- sql/schema.sql (стр. 1–14)
PRAGMA foreign_keys = ON;

CREATE TABLE IF NOT EXISTS sensor_readings (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    ...
);
```

[^dbpathpy]:

```python
# web/app.py (стр. 7–14)
def _default_db_path():
    return os.path.join(_base_dir(), "..", "data", "device.db")

DB_PATH = os.environ.get("IOT_DB_PATH", _default_db_path())
```

[^template]:

```html
<!-- web/templates/index.html (стр. 35–47) -->
      {% for r in rows %}
      <tr>
        <td>{{ r.recorded_at }}</td>
        <td>{{ "%.1f"|format(r.temperature_c) }}</td>
        ...
      {% else %}
      <tr><td colspan="7">Нет данных. ...
```

---

## 3. `device_core/src/main.cpp`

### 3.1. Директивы препроцессора и заголовки

- `#include "device_manager.h"` — заголовок из каталога include проекта; в кавычках ищется сначала относительно текущего файла, затем по путям компилятора[^inc].
- `#include <fcntl.h>`, `#include <unistd.h>` — **POSIX**: `open`, `close` для проверки доступа к `/dev/gpiochipN`[^posix].
- `#include <cerrno>`, `#include <cstring>` — C-совместимые обёртки: `errno`, `std::strerror`[^cerr].
- Стандартная библиотека C++: потоки ошибок/вывода, строки[^iostream].

[^inc]:

```cpp
// device_core/src/main.cpp (стр. 1–9)
#include "device_manager.h"

#include <fcntl.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <iostream>
#include <string>
```

[^posix]:

Использование:

```cpp
// device_core/src/main.cpp (стр. 16–29)
bool hasGpioAccess(const std::string& chipName) {
    std::string path = "/dev/" + chipName;
    int fd = ::open(path.c_str(), O_RDONLY);
    ...
    ::close(fd);
```

[^cerr]:

```cpp
// device_core/src/main.cpp (стр. 19–25)
        if (errno == EACCES) {
            logError("No permission to access " + path +
                     ". Ensure user is in 'gpio' group and re-login.");
        } else {
            logError("Failed to open " + path + ": " + std::strerror(errno));
```

[^iostream]:

`std::cerr`, `std::endl` — неконтейнерный вывод с сбросом буфера.

### 3.2. Анонимное пространство имён

`namespace { ... }` без имени ограничивает видимость функций **одной единицей трансляции** (файла), даёт внутреннее связывание по смыслу (аналог `static` для свободных функций в C)[^anon].

[^anon]:

```cpp
// device_core/src/main.cpp (стр. 11–33)
namespace {
void logError(const std::string& message) {
    std::cerr << "[ERROR] " << message << std::endl;
}

bool hasGpioAccess(const std::string& chipName) {
```

### 3.3. Точка входа и аргументы командной строки

- Сигнатура `int main(int argc, char* argv[])` — стандарт C++: `argc` — число аргументов (включая имя программы), `argv` — массив C-строк[^mainsig].
- `std::string configPath = "config.json"` — инициализация строки; при `argc > 1` подставляется `argv[1]`[^cfgarg].
- `DeviceManager manager` — объект со **статической продолжительностью хранения** на стеке `main`[^dmstack].

[^mainsig]:

```cpp
// device_core/src/main.cpp (стр. 35–39)
int main(int argc, char* argv[]) {
    std::string configPath = "config.json";
    if (argc > 1) {
        configPath = argv[1];
    }
```

[^cfgarg]: Первый необязательный аргумент — путь к JSON конфигурации.

[^dmstack]:

```cpp
// device_core/src/main.cpp (стр. 41–41)
    DeviceManager manager;
```

### 3.4. Последовательность и коды возврата

- `if (!manager.loadConfig(configPath))` — логическое НЕ для `bool`; при ошибке `return 1` — соглашение Unix о ненулевом коде[^ret].
- Проверка `simulateHardware`: если ложь, вызывается `hasGpioAccess`[^sim].
- `manager.runMainLoop()` — бесконечный цикл внутри менеджера; `return 0` после цикла в норме недостижим[^loop].

[^ret]:

```cpp
// device_core/src/main.cpp (стр. 43–46)
    if (!manager.loadConfig(configPath)) {
        logError("Could not load configuration. Exiting.");
        return 1;
    }
```

[^sim]:

```cpp
// device_core/src/main.cpp (стр. 48–53)
    if (!manager.getConfig().simulateHardware) {
        if (!hasGpioAccess(manager.getConfig().gpioChip)) {
            logError("Insufficient permissions to access GPIO. Exiting.");
            return 1;
        }
    }
```

[^loop]:

```cpp
// device_core/src/main.cpp (стр. 60–62)
    manager.runMainLoop();

    return 0;
```

### 3.5. Оператор разрешения области видимости `::`

`::open`, `::close` явно указывают на глобальные функции POSIX, а не на возможные одноимённые символы из пространств имён[^global].

[^global]:

```cpp
// device_core/src/main.cpp (стр. 18–18)
    int fd = ::open(path.c_str(), O_RDONLY);
```

---

## 4. `device_core/include/device_manager.h` — конфигурация и менеджер

### 4.1. `#pragma once`

Защита от повторного включения заголовка альтернативой классическим include guards[^once].

[^once]:

```cpp
// device_core/include/device_manager.h (стр. 1–1)
#pragma once
```

### 4.2. Структуры с инициализаторами по умолчанию (C++11+)

`DeviceConfig` и `DeviceStatus` — агрегаты с **скобочными** инициализаторами членов в объявлении (`gpioChip{"gpiochip0"}`, `float relayDistanceThresholdCm{20.0f}`)[^brace].

[^brace]:

```cpp
// device_core/include/device_manager.h (стр. 13–38)
struct DeviceConfig {
    std::string gpioChip{"gpiochip0"};

    int dhtPin{17};
    ...
    float relayDistanceThresholdCm{20.0f};
    int pollIntervalMs{1000};

    std::string databasePath{"../data/device.db"};

    bool simulateHardware{false};
};

struct DeviceStatus {
    float temperatureC{0.0f};
    ...
    bool relayOn{false};
};
```

### 4.3. Класс `DeviceManager` и умные указатели

- `std::unique_ptr<...>` — **единственное владение** объектом на куче; при разрушении менеджера деструкторы датчиков вызываются автоматически[^unique].
- Методы с `const` в конце — не изменяют логическое состояние объекта (`getStatus`, `getConfig`)[^constmeth].

[^unique]:

```cpp
// device_core/include/device_manager.h (стр. 54–64)
private:
    DeviceConfig config_{};
    std::unique_ptr<DhtSensor> dht_;
    std::unique_ptr<UltrasonicSensor> ultrasonic_;
    std::unique_ptr<DigitalSensor> button_;
    std::unique_ptr<LedActuator> led_;
    std::unique_ptr<RelayActuator> relay_;
    std::unique_ptr<Database> database_;

    DeviceStatus status_{};
    bool initialized_{false};
```

[^constmeth]:

```cpp
// device_core/include/device_manager.h (стр. 50–52)
    DeviceStatus getStatus() const;

    const DeviceConfig& getConfig() const;
```

---

## 5. `device_core/src/device_manager.cpp`

### 5.1. Время и многопоточность

- `std::chrono::system_clock` — системные часы; `to_time_t` переводит `time_point` в `time_t`[^chrono].
- Условная компиляция `#if defined(_WIN32)` — на Windows `localtime_s`, иначе POSIX `localtime_r` (потокобезопасно)[^time].
- `std::put_time` + `std::ostringstream` форматируют время в строку `"%Y-%m-%d %H:%M:%S"`[^puttime].

[^chrono]:

```cpp
// device_core/src/device_manager.cpp (стр. 14–17)
std::string currentTimestamp() {
    using namespace std::chrono;
    auto now = system_clock::now();
    auto timeT = system_clock::to_time_t(now);
```

[^time]:

```cpp
// device_core/src/device_manager.cpp (стр. 18–23)
    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &timeT);
#else
    localtime_r(&timeT, &tm);
#endif
```

[^puttime]:

```cpp
// device_core/src/device_manager.cpp (стр. 24–26)
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
```

### 5.2. JsonCpp: разбор конфигурации

- `std::ifstream in(path)` — поток ввода из файла; `in >> root` использует перегрузку `operator>>` для `Json::Value`[^jsonstream].
- `root.isMember("key")` — проверка ключа; `asString()`, `asInt()`, `asFloat()`, `asBool()` — приведение типов JSON[^jsonapi].
- После чтения — валидация пинов `isValidBcmPin` (диапазон 0..27) и исправление неположительного `poll_interval_ms` на 1000[^validate].

[^jsonstream]:

```cpp
// device_core/src/device_manager.cpp (стр. 45–53)
bool DeviceManager::loadConfig(const std::string& path) {
    std::ifstream in(path);
    if (!in.is_open()) {
        logError("Failed to open config file: " + path);
        return false;
    }

    Json::Value root;
    in >> root;
```

[^jsonapi]:

```cpp
// device_core/src/device_manager.cpp (стр. 55–87)
    if (root.isMember("gpio_chip")) {
        config_.gpioChip = root["gpio_chip"].asString();
    }
    ...
    if (root.isMember("simulate_hardware")) {
        config_.simulateHardware = root["simulate_hardware"].asBool();
    }
```

[^validate]:

```cpp
// device_core/src/device_manager.cpp (стр. 89–101)
    if (!isValidBcmPin(config_.dhtPin) ||
        ...
        return false;
    }

    if (config_.pollIntervalMs <= 0) {
        config_.pollIntervalMs = 1000;
    }
```

### 5.3. `std::make_unique` и инициализация устройств

`std::make_unique<T>(args...)` создаёт объект на куче и оборачивает в `unique_ptr` (C++14)[^makeunique]. Затем для каждого устройства вызывается `init()`, после — `Database::open()`[^initdev].

[^makeunique]:

```cpp
// device_core/src/device_manager.cpp (стр. 113–121)
    dht_ = std::make_unique<DhtSensor>("DHT", config_.gpioChip, config_.dhtPin);
    ultrasonic_ = std::make_unique<UltrasonicSensor>("Ultrasonic", config_.gpioChip,
                                                     config_.ultrasonicTriggerPin, config_.ultrasonicEchoPin,
                                                     config_.simulateHardware);
    button_ = std::make_unique<DigitalSensor>("Button", config_.gpioChip, config_.buttonPin, true,
                                              config_.simulateHardware);
```

[^initdev]:

```cpp
// device_core/src/device_manager.cpp (стр. 144–148)
    database_ = std::make_unique<Database>(config_.databasePath);
    if (!database_->open()) {
        logError("Failed to open SQLite database: " + config_.databasePath);
        return false;
    }
```

### 5.4. Главный цикл: логика управления

- Опрос датчиков: `read()`, затем геттеры (`getTemperatureC`, `getDistanceCm`, `isActive`)[^read].
- **Гистерезис по событию**: LED и реле меняют состояние только если «желаемое» отличается от `status_` и `setState` успешен[^hyst].
- `std::to_string` для чисел в строке лога; тернарный оператор `? :` для текста ON/OFF[^log].
- `std::this_thread::sleep_for(std::chrono::milliseconds(...))` — пауза между итерациями без busy-wait[^sleep].

[^read]:

```cpp
// device_core/src/device_manager.cpp (стр. 163–181)
    while (true) {
        if (dht_->read()) {
            status_.temperatureC = dht_->getTemperatureC();
            status_.humidityPercent = dht_->getHumidityPercent();
        } else {
            logError("DHT read failed");
        }
        ...
        if (button_->read()) {
            status_.buttonActive = button_->isActive();
```

[^hyst]:

```cpp
// device_core/src/device_manager.cpp (стр. 183–196)
        bool desiredLedState = status_.buttonActive;
        if (desiredLedState != status_.ledOn) {
            if (led_->setState(desiredLedState)) {
                status_.ledOn = desiredLedState;
            }
        }

        bool desiredRelayState =
            status_.distanceCm > 0.0f && status_.distanceCm < config_.relayDistanceThresholdCm;
        if (desiredRelayState != status_.relayOn) {
            if (relay_->setState(desiredRelayState)) {
                status_.relayOn = desiredRelayState;
            }
        }
```

[^log]:

```cpp
// device_core/src/device_manager.cpp (стр. 198–203)
        logInfo("T=" + std::to_string(status_.temperatureC) +
                "C, H=" + std::to_string(status_.humidityPercent) +
                "%, D=" + std::to_string(status_.distanceCm) +
                "cm, Button=" + std::string(status_.buttonActive ? "ON" : "OFF") +
                ", LED=" + std::string(status_.ledOn ? "ON" : "OFF") +
                ", Relay=" + std::string(status_.relayOn ? "ON" : "OFF"));
```

[^sleep]:

```cpp
// device_core/src/device_manager.cpp (стр. 209–209)
        std::this_thread::sleep_for(std::chrono::milliseconds(config_.pollIntervalMs));
```

### 5.5. Явно определённый конструктор по умолчанию

`DeviceManager::DeviceManager() = default;` — компилятор генерирует конструктор, инициализируя члены их значениями по умолчанию[^defaulted].

[^defaulted]:

```cpp
// device_core/src/device_manager.cpp (стр. 43–43)
DeviceManager::DeviceManager() = default;
```

---

## 6. Базовые классы `sensor_base.h` / `actuator_base.h`

### 6.1. Полиморфизм и чистые виртуальные функции

- `virtual bool init() = 0` — **чистая виртуальная** функция: базовый класс не создаётся сам по себе (без определения), наследники обязаны реализовать `init` и `read`[^pure].
- `virtual ~SensorBase() = default` — виртуальный деструктор, чтобы при удалении через указатель на базовый класс вызвались деструкторы производных[^virtdest].
- `[[nodiscard]]` — атрибут C++17: результат не должен игнорироваться (подсказка компилятору и читателю)[^nodiscard].

[^pure]:

```cpp
// device_core/include/sensor_base.h (стр. 7–12)
    explicit SensorBase(std::string name) : name_(std::move(name)) {}
    virtual ~SensorBase() = default;

    virtual bool init() = 0;

    virtual bool read() = 0;
```

[^virtdest]: Деструктор базового класса виртуальный — корректное уничтожение `unique_ptr<SensorBase>` при фактическом типе `DhtSensor`.

[^nodiscard]:

```cpp
// device_core/include/sensor_base.h (стр. 14–14)
    [[nodiscard]] const std::string& getName() const noexcept { return name_; }
```

### 6.2. `explicit` и `noexcept`

- `explicit` у конструктора запрещает неявные преобразования из `std::string` в `SensorBase`[^explicit].
- `noexcept` на геттере — гарантия отсутствия исключений (упрощает оптимизации)[^noexcept].

[^explicit]:

```cpp
// device_core/include/sensor_base.h (стр. 7–7)
    explicit SensorBase(std::string name) : name_(std::move(name)) {}
```

[^noexcept]:

```cpp
// device_core/include/sensor_base.h (стр. 14–14)
    [[nodiscard]] const std::string& getName() const noexcept { return name_; }
```

### 6.3. `std::move` в списке инициализации

`name_(std::move(name))` — передача владения строкой в член `name_` без лишнего копирования[^move].

[^move]: См. конструктор `SensorBase` в `sensor_base.h` (строка 7).

---

## 7. `database.h` / `database.cpp` — SQLite

### 7.1. Неполный тип и PIMPL-подобное сокрытие

В заголовке объявлено `struct DeviceStatus;` — **предварительное объявление**; полное определение нужно в `.cpp`, где вызывается `insertReading` с полем структуры[^forward].

`void* dbHandle_` скрывает `sqlite3*` от заголовка (упрощённый вариант без явного включения `sqlite3.h` в интерфейс)[^opaque].

[^forward]:

```cpp
// device_core/include/database.h (стр. 1–5)
#pragma once

#include <string>

struct DeviceStatus;
```

[^opaque]:

```cpp
// device_core/include/database.h (стр. 19–21)
private:
    std::string path_;
    void* dbHandle_{nullptr};
```

### 7.2. Удалённые копирование

`Database(const Database&) = delete` и присваивание `= delete` — запрет копирования (владение одним дескриптором БД)[^delete].

[^delete]:

```cpp
// device_core/include/database.h (стр. 12–13)
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;
```

### 7.3. C API SQLite

- `sqlite3_open` — открытие файла[^open].
- `sqlite3_exec` — выполнение SQL без пошагового результата; буфер ошибок освобождается `sqlite3_free`[^exec].
- `sqlite3_prepare_v2` — компиляция запроса в **подготовленное выражение**[^prepare].
- `sqlite3_bind_*` — привязка параметров `?` по индексам 1..N[^bind].
- `sqlite3_step` — выполнение одного шага; для INSERT ожидается `SQLITE_DONE`[^step].
- `SQLITE_TRANSIENT` — SQLite копирует строку привязки до завершения шага[^transient].

[^open]:

```cpp
// device_core/src/database.cpp (стр. 38–42)
    if (sqlite3_open(path_.c_str(), &db) != SQLITE_OK) {
        return false;
    }
    dbHandle_ = db;
```

[^exec]:

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
```

[^prepare]:

```cpp
// device_core/src/database.cpp (стр. 73–78)
    const char* sql =
        "INSERT INTO sensor_readings (recorded_at, temperature_c, humidity_percent, distance_cm, "
        "button_active, led_on, relay_on) VALUES (?,?,?,?,?,?,?)";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, static_cast<int>(std::strlen(sql)), &stmt, nullptr) != SQLITE_OK) {
```

[^bind]:

```cpp
// device_core/src/database.cpp (стр. 80–86)
    sqlite3_bind_text(stmt, 1, timestamp.c_str(), static_cast<int>(timestamp.size()), SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 2, static_cast<double>(status.temperatureC));
    ...
    sqlite3_bind_int(stmt, 5, status.buttonActive ? 1 : 0);
```

[^step]:

```cpp
// device_core/src/database.cpp (стр. 87–89)
    const int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
```

[^transient]: Константа `SQLITE_TRANSIENT` (-1) сообщает, что строка может быть освобождена сразу после вызова bind — SQLite сделает внутреннюю копию.

### 7.4. Конструктор перемещения строки

`Database::Database(std::string path) : path_(std::move(path)) {}` — прием аргумента по значению и перемещение в член[^dbctor].

[^dbctor]:

```cpp
// device_core/src/database.cpp (стр. 23–23)
Database::Database(std::string path) : path_(std::move(path)) {}
```

---

## 8. Датчики и актуаторы

### 8.1. `DhtSensor` — заглушка и `<random>`

- `std::mt19937` — генератор Мерсенна Твистера; `std::random_device{}()` как seed[^random].
- `std::uniform_real_distribution<float>` — равномерное распределение в интервале[^uniform].
- `(void)chipName_;` — подавление предупреждения о неиспользуемых параметрах в учебной заглушке[^void].

[^random]:

```cpp
// device_core/src/dht_sensor.cpp (стр. 13–16)
float randomInRange(float min, float max) {
    static std::mt19937 rng{std::random_device{}()};
    std::uniform_real_distribution<float> dist(min, max);
    return dist(rng);
```

[^uniform]: См. выше.

[^void]:

```cpp
// device_core/src/dht_sensor.cpp (стр. 28–31)
bool DhtSensor::init() {
    (void)chipName_;
    (void)lineOffset_;
    return true;
```

### 8.2. `DigitalSensor` — libgpiod

- `gpiod_chip_open_by_name`, `gpiod_chip_get_line`, `gpiod_line_request_input` — типовая последовательность для **входа**[^diginit].
- `activeHigh_ ? (value != 0) : (value == 0)` — интерпретация уровня с учётом активного уровня схемы[^activehigh].
- В режиме `simulate_` — `std::bernoulli_distribution` для случайного логического значения[^bern].

[^diginit]:

```cpp
// device_core/src/digital_sensor.cpp (стр. 40–55)
    chip_ = gpiod_chip_open_by_name(chipName_.c_str());
    ...
    line_ = gpiod_chip_get_line(chip_, lineOffset_);
    ...
    if (gpiod_line_request_input(line_, name_.c_str()) < 0) {
```

[^activehigh]:

```cpp
// device_core/src/digital_sensor.cpp (стр. 78–78)
    lastValue_ = activeHigh_ ? (value != 0) : (value == 0);
```

[^bern]:

```cpp
// device_core/src/digital_sensor.cpp (стр. 61–64)
        static std::mt19937 rng{std::random_device{}()};
        std::bernoulli_distribution dist(0.4);
        lastValue_ = dist(rng);
```

### 8.3. `UltrasonicSensor` — импульс и измерение интервала

- Импульс на trigger: низкий → короткая задержка → высокий 10 мкс → низкий[^trig].
- Ожидание фронтов на echo с таймаутом 50 мс через `steady_clock`[^echo].
- Расстояние: длительность импульса в микросекундах делится на **58** — эмпирический коэффициент для HC-SR04 (см/мкс приближённо)[^dist].

[^trig]:

```cpp
// device_core/src/ultrasonic_sensor.cpp (стр. 90–94)
    gpiod_line_set_value(triggerLine_, 0);
    std::this_thread::sleep_for(microseconds(2));
    gpiod_line_set_value(triggerLine_, 1);
    std::this_thread::sleep_for(microseconds(10));
    gpiod_line_set_value(triggerLine_, 0);
```

[^echo]:

```cpp
// device_core/src/ultrasonic_sensor.cpp (стр. 96–110)
    auto startWait = steady_clock::now();
    while (gpiod_line_get_value(echoLine_) == 0) {
        if (steady_clock::now() - startWait > milliseconds(50)) {
            logError("Ultrasonic sensor: timeout waiting for echo HIGH");
            return false;
        }
    }
```

[^dist]:

```cpp
// device_core/src/ultrasonic_sensor.cpp (стр. 113–115)
    auto pulseDurationUs = duration_cast<microseconds>(pulseEnd - pulseStart).count();

    lastDistanceCm_ = static_cast<float>(pulseDurationUs) / 58.0f;
```

### 8.4. `LedActuator` / `RelayActuator`

Идентичная структура: выход `gpiod_line_request_output`, установка уровня `gpiod_line_set_value` с инверсией при `activeHigh_ == false`[^led].

[^led]:

```cpp
// device_core/src/led_actuator.cpp (стр. 52–76)
    int initialValue = activeHigh_ ? 0 : 1;
    if (gpiod_line_request_output(line_, name_.c_str(), initialValue) < 0) {
    ...
    int value = activeHigh_ ? (on ? 1 : 0) : (on ? 0 : 1);
    if (gpiod_line_set_value(line_, value) < 0) {
```

### 8.5. `override` в производных классах

Явное указание `override` при переопределении виртуальных методов базового класса — проверка на этапе компиляции[^override].

[^override]:

Пример:

```cpp
// device_core/include/dht_sensor.h (стр. 12–13)
    bool init() override;
    bool read() override;
```

---

## 9. `web/app.py` — Python / Flask

### 9.1. Пути и окружение

- `os.path.dirname(os.path.abspath(__file__))` — каталог файла `app.py`[^abspath].
- `os.environ.get("IOT_DB_PATH", _default_db_path())` — чтение переменной окружения с запасным значением[^environ].

[^abspath]:

```python
# web/app.py (стр. 7–10)
def _base_dir():
    return os.path.dirname(os.path.abspath(__file__))

def _default_db_path():
```

[^environ]:

```python
# web/app.py (стр. 12–12)
DB_PATH = os.environ.get("IOT_DB_PATH", _default_db_path())
```

### 9.2. Модуль `sqlite3`

- `sqlite3.connect` — соединение с файлом БД[^connect].
- `conn.row_factory = sqlite3.Row` — строки как объекты с доступом по имени колонки[^row].
- Параметризованный запрос: `LIMIT ?` и кортеж `(limit,)` предотвращают SQL-инъекции[^param].

[^connect]:

```python
# web/app.py (стр. 19–20)
    conn = sqlite3.connect(DB_PATH)
    conn.row_factory = sqlite3.Row
```

[^row]: См. выше; затем `dict(r)` для сериализации.

[^param]:

```python
# web/app.py (стр. 21–25)
    cur = conn.execute(
        "SELECT id, recorded_at, temperature_c, humidity_percent, distance_cm, "
        "button_active, led_on, relay_on FROM sensor_readings ORDER BY id DESC LIMIT ?",
        (limit,),
    )
```

### 9.3. Декораторы маршрутов Flask

`@app.route("/")` регистрирует функцию обработки HTTP GET для корня[^route]. `jsonify` формирует ответ `application/json`[^jsonify].

[^route]:

```python
# web/app.py (стр. 30–33)
@app.route("/")
def index():
    rows = fetch_readings(100)
    return render_template("index.html", rows=rows)
```

[^jsonify]:

```python
# web/app.py (стр. 35–37)
@app.route("/api/readings")
def api_readings():
    return jsonify(fetch_readings(200))
```

### 9.4. Точка входа

`if __name__ == "__main__":` — код запуска встроенного сервера только при прямом вызове `python app.py`[^mainpy].

[^mainpy]:

```python
# web/app.py (стр. 39–40)
if __name__ == "__main__":
    app.run(host="0.0.0.0", port=int(os.environ.get("PORT", "8080")))
```

---

## 10. `web/templates/index.html` — HTML5 и Jinja2

- `<!DOCTYPE html>` — режим стандартов HTML5[^doctype].
- `{% for r in rows %} ... {% else %} ... {% endfor %}` — цикл с веткой «пусто»[^for].
- `{{ "%.1f"|format(r.temperature_c) }}` — фильтр `format` для чисел[^format].
- `{{ 'on' if r.button_active else 'off' }}` — условное выражение в шаблоне[^ifj].
- `setTimeout(..., 5000)` — перезагрузка страницы без WebSocket[^reload].

[^doctype]:

```html
<!-- web/templates/index.html (стр. 1–1) -->
<!DOCTYPE html>
```

[^for]:

```html
<!-- web/templates/index.html (стр. 35–47) -->
      {% for r in rows %}
      <tr>
        ...
      {% else %}
      <tr><td colspan="7">Нет данных. ...
```

[^format]:

```html
<!-- web/templates/index.html (стр. 38–40) -->
        <td>{{ "%.1f"|format(r.temperature_c) }}</td>
        <td>{{ "%.1f"|format(r.humidity_percent) }}</td>
        <td>{{ "%.1f"|format(r.distance_cm) }}</td>
```

[^ifj]:

```html
<!-- web/templates/index.html (стр. 41–43) -->
        <td class="{{ 'on' if r.button_active else 'off' }}">{{ 'да' if r.button_active else 'нет' }}</td>
        <td class="{{ 'on' if r.led_on else 'off' }}">{{ 'вкл' if r.led_on else 'выкл' }}</td>
```

[^reload]:

```html
<!-- web/templates/index.html (стр. 50–52) -->
  <script>
    setTimeout(function () { location.reload(); }, 5000);
  </script>
```

---

## 11. SQL

**Развёрнутое учебное описание** модуля данных (DDL, WAL, C API `sqlite3_*`, чтение во Flask, демонстрация в `sqlite3` CLI, чек-лист для студентов) вынесено в отдельный документ: **[`sql.md`](sql.md)**.

Кратко: таблица `sensor_readings` хранит время записи `recorded_at` (TEXT ISO-подобной строки из C++) и снимок всех величин; индекс по `recorded_at` ускоряет выборки по времени при росте объёма[^idx].

Типы SQLite: `INTEGER PRIMARY KEY AUTOINCREMENT`, `REAL`, `TEXT`; булевы флаги хранятся как `INTEGER` 0/1[^types].

[^idx]:

```cpp
// device_core/src/database.cpp (стр. 60–64)
    const char* idx =
        "CREATE INDEX IF NOT EXISTS idx_sensor_readings_recorded_at ON sensor_readings (recorded_at);";
```

[^types]: См. DDL в `database.cpp` и `sql/schema.sql`.

---

## 12. CMake (сборка `iot_device_core`)

- `cmake_minimum_required(VERSION 3.14)` — минимальная версия CMake[^min].
- `project(... LANGUAGES CXX)` — только C++[^proj].
- `add_executable` + список исходников[^addexe].
- `find_package` / `pkg_check_modules` / `target_link_libraries` — декларативное описание зависимостей[^tlink].

[^min]:

```cmake
// device_core/CMakeLists.txt (стр. 1–3)
cmake_minimum_required(VERSION 3.14)

project(iot_device_core LANGUAGES CXX)
```

[^proj]: См. выше.

[^addexe]:

```cmake
// device_core/CMakeLists.txt (стр. 10–21)
set(SOURCES
    src/main.cpp
    src/database.cpp
    ...
)

add_executable(iot_device_core ${SOURCES})
```

[^tlink]:

```cmake
// device_core/CMakeLists.txt (стр. 28–44)
find_package(PkgConfig REQUIRED)
find_package(SQLite3 REQUIRED)
pkg_check_modules(GPIOD REQUIRED libgpiod)
pkg_check_modules(JSONCPP REQUIRED jsoncpp)
...
target_link_libraries(iot_device_core
    PRIVATE
        SQLite::SQLite3
        ${GPIOD_LIBRARIES}
        ${JSONCPP_LIBRARIES}
)
```

---

## 13. Docker (обзор)

`docker-compose.yml` поднимает сервис с пробросом портов 6080 (noVNC) и 8080 (веб), томом `student_workspace`[^compose].

`Dockerfile` устанавливает компилятор, CMake, `libgpiod-dev`, `jsoncpp`, `sqlite3`, Python и зависимости из `web/requirements.txt`, копирует дерево в `/opt/iot_workspace`, выполняет `cmake` и сборку[^dockerfile].

[^compose]: См. `docker/docker-compose.yml`.

[^dockerfile]: См. `docker/Dockerfile` — блоки `RUN apt-get`, `COPY`, `pip3 install`, предсборка в `/opt/iot_workspace/device_core/build`.

---

## 14. Сводка: синтаксис и возможности C++17, встречающиеся в проекте

| Возможность | Где проявляется |
|-------------|-----------------|
| Инициализация членов в классе/структуре | `DeviceConfig`, поля датчиков |
| `= default` / `= delete` | конструктор/деструктор, запрет копирования `Database` |
| `override`, `final` (не используется) | производные датчики |
| `[[nodiscard]]` | геттеры в базовых классах |
| `std::unique_ptr`, `std::make_unique` | владение датчиками и БД |
| `std::move`, `std::string` по значению | конструкторы, передача путей |
| `auto`, `chrono`, потоки | время, `sleep_for` |
| Виртуальные функции, полиморфизм | `SensorBase` / `ActuatorBase` |
| Условная компиляция `#if defined(_WIN32)` | `localtime_s` / `localtime_r` |
| Анонимное пространство имён | вспомогательные функции в `.cpp` |

---

Архитектурный обзор без углубления в каждый синтаксический элемент см. в [`architecture.md`](architecture.md).
