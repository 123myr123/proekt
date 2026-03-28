#pragma once

#include <string>

class ActuatorBase {
public:
    explicit ActuatorBase(std::string name) : name_(std::move(name)) {}
    virtual ~ActuatorBase() = default;

    virtual bool init() = 0;

    virtual bool setState(bool on) = 0;

    [[nodiscard]] virtual bool getState() const = 0;

    [[nodiscard]] const std::string& getName() const noexcept { return name_; }

protected:
    std::string name_;
};

