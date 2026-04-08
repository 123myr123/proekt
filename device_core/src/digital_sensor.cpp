#include "digital_sensor.h"

#include <chrono>
#include <iostream>

namespace {

void logError(const std::string& message) {
    std::cerr << "[ERROR] " << message << std::endl;
}

} // namespace

DigitalSensor::DigitalSensor(const std::string& name,
                             const std::string& chipName,
                             int lineOffset,
                             bool activeHigh)
    : SensorBase(name),
      chipName_(chipName),
      lineOffset_(lineOffset),
      activeHigh_(activeHigh) {}

DigitalSensor::~DigitalSensor() {
    if (line_) {
        gpiod_line_release(line_);
        line_ = nullptr;
    }
    if (chip_) {
        gpiod_chip_close(chip_);
        chip_ = nullptr;
    }
}

bool DigitalSensor::init() {
    chip_ = gpiod_chip_open_by_name(chipName_.c_str());
    if (!chip_) {
        logError("Failed to open GPIO chip: " + chipName_);
        return false;
    }

    line_ = gpiod_chip_get_line(chip_, lineOffset_);
    if (!line_) {
        logError("Failed to get GPIO line for digital sensor: " + std::to_string(lineOffset_));
        return false;
    }

    if (gpiod_line_request_input(line_, name_.c_str()) < 0) {
        logError("Failed to request GPIO line as input for digital sensor");
        return false;
    }

    return true;
}

bool DigitalSensor::read() {
    if (!line_) {
        logError("Digital sensor line is not initialized");
        return false;
    }

    int value = gpiod_line_get_value(line_);
    if (value < 0) {
        logError("Failed to read value from digital sensor GPIO line");
        return false;
    }

    lastValue_ = activeHigh_ ? (value != 0) : (value == 0);
    return true;
}

bool DigitalSensor::isActive() const noexcept {
    return lastValue_;
}

