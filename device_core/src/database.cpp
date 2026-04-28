#include "database.h"
#include "device_manager.h"

#include <sqlite3.h>

#include <cstring>

namespace {

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

}

Database::Database(std::string path) : path_(std::move(path)) {}

Database::~Database() {
    close();
}

void Database::close() {
    if (dbHandle_) {
        sqlite3_close(static_cast<sqlite3*>(dbHandle_));
        dbHandle_ = nullptr;
    }
}

bool Database::open() {
    close();
    sqlite3* db = nullptr;
    if (sqlite3_open(path_.c_str(), &db) != SQLITE_OK) {
        return false;
    }
    dbHandle_ = db;
    if (!execSql(db, "PRAGMA journal_mode=WAL;")) {
        return false;
    }
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
    if (!execSql(db, ddl)) {
        return false;
    }
    const char* idx =
        "CREATE INDEX IF NOT EXISTS idx_sensor_readings_recorded_at ON sensor_readings (recorded_at);";
    if (!execSql(db, idx)) {
        return false;
    }
    return true;
}

bool Database::insertReading(const DeviceStatus& status, const std::string& timestamp) {
    sqlite3* db = static_cast<sqlite3*>(dbHandle_);
    if (!db) {
        return false;
    }
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
}
