#include "led_actuator.h"

#include <iostream>

namespace {

void logError(const std::string& message) {
    std::cerr << "[ERROR] " << message << std::endl;
}

} // namespace

LedActuator::LedActuator(const std::string& name,
                         const std::string& chipName,
                         int lineOffset,
                         bool activeHigh)
    : ActuatorBase(name),
      chipName_(chipName),
      lineOffset_(lineOffset),
      activeHigh_(activeHigh) {}

LedActuator::~LedActuator() {
    if (line_) {
        gpiod_line_release(line_);
        line_ = nullptr;
    }
    if (chip_) {
        gpiod_chip_close(chip_);
        chip_ = nullptr;
    }
}

bool LedActuator::init() {
    chip_ = gpiod_chip_open_by_name(chipName_.c_str());
    if (!chip_) {
        logError("Failed to open GPIO chip for LED: " + chipName_);
        return false;
    }

    line_ = gpiod_chip_get_line(chip_, lineOffset_);
    if (!line_) {
        logError("Failed to get GPIO line for LED: " + std::to_string(lineOffset_));
        return false;
    }

    int initialValue = activeHigh_ ? 0 : 1;
    if (gpiod_line_request_output(line_, name_.c_str(), initialValue) < 0) {
        logError("Failed to request GPIO line as output for LED");
        return false;
    }

    state_ = false;
    return true;
}

bool LedActuator::setState(bool on) {
    if (!line_) {
        logError("LED line is not initialized");
        return false;
    }

    int value = activeHigh_ ? (on ? 1 : 0) : (on ? 0 : 1);
    if (gpiod_line_set_value(line_, value) < 0) {
        logError("Failed to set value on LED GPIO line");
        return false;
    }

    state_ = on;
    return true;
}

bool LedActuator::getState() const {
    return state_;
}

