#pragma once

#include <string>

struct DeviceStatus;

class Database {
public:
    explicit Database(std::string path);
    ~Database();

    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    bool open();
    void close();
    bool insertReading(const DeviceStatus& status, const std::string& timestamp);

private:
    std::string path_;
    void* dbHandle_{nullptr};
};
