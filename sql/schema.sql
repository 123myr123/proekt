PRAGMA foreign_keys = ON;

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

CREATE INDEX IF NOT EXISTS idx_sensor_readings_recorded_at ON sensor_readings (recorded_at);