#pragma once

#include <string>

class SensorBase {
public:
    explicit SensorBase(std::string name) : name_(std::move(name)) {}
    virtual ~SensorBase() = default;

    virtual bool init() = 0;

    virtual bool read() = 0;

    [[nodiscard]] const std::string& getName() const noexcept { return name_; }

protected:
    std::string name_;
};

