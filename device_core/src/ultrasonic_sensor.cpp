#include "ultrasonic_sensor.h"

#include <chrono>
#include <iostream>
#include <thread>

namespace {

void logError(const std::string& message) {
    std::cerr << "[ERROR] " << message << std::endl;
}

} // namespace

UltrasonicSensor::UltrasonicSensor(const std::string& name,
                                   const std::string& chipName,
                                   int triggerOffset,
                                   int echoOffset)
    : SensorBase(name),
      chipName_(chipName),
      triggerOffset_(triggerOffset),
      echoOffset_(echoOffset) {}

UltrasonicSensor::~UltrasonicSensor() {
    if (triggerLine_) {
        gpiod_line_release(triggerLine_);
        triggerLine_ = nullptr;
    }
    if (echoLine_) {
        gpiod_line_release(echoLine_);
        echoLine_ = nullptr;
    }
    if (chip_) {
        gpiod_chip_close(chip_);
        chip_ = nullptr;
    }
}

bool UltrasonicSensor::init() {
    chip_ = gpiod_chip_open_by_name(chipName_.c_str());
    if (!chip_) {
        logError("Failed to open GPIO chip: " + chipName_);
        return false;
    }

    triggerLine_ = gpiod_chip_get_line(chip_, triggerOffset_);
    echoLine_ = gpiod_chip_get_line(chip_, echoOffset_);
    if (!triggerLine_ || !echoLine_) {
        logError("Failed to get GPIO lines for ultrasonic sensor");
        return false;
    }

    if (gpiod_line_request_output(triggerLine_, name_.c_str(), 0) < 0) {
        logError("Failed to request trigger line as output");
        return false;
    }

    if (gpiod_line_request_input(echoLine_, name_.c_str()) < 0) {
        logError("Failed to request echo line as input");
        return false;
    }

    return true;
}

bool UltrasonicSensor::read() {
    if (!triggerLine_ || !echoLine_) {
        logError("Ultrasonic sensor lines are not initialized");
        return false;
    }

    using namespace std::chrono;

    gpiod_line_set_value(triggerLine_, 0);
    std::this_thread::sleep_for(microseconds(2));
    gpiod_line_set_value(triggerLine_, 1);
    std::this_thread::sleep_for(microseconds(10));
    gpiod_line_set_value(triggerLine_, 0);

    auto startWait = steady_clock::now();
    while (gpiod_line_get_value(echoLine_) == 0) {
        if (steady_clock::now() - startWait > milliseconds(50)) {
            logError("Ultrasonic sensor: timeout waiting for echo HIGH");
            return false;
        }
    }

    auto pulseStart = steady_clock::now();
    while (gpiod_line_get_value(echoLine_) == 1) {
        if (steady_clock::now() - pulseStart > milliseconds(50)) {
            logError("Ultrasonic sensor: timeout waiting for echo LOW");
            return false;
        }
    }
    auto pulseEnd = steady_clock::now();

    auto pulseDurationUs = duration_cast<microseconds>(pulseEnd - pulseStart).count();

    lastDistanceCm_ = static_cast<float>(pulseDurationUs) / 58.0f;

    return true;
}

float UltrasonicSensor::getDistanceCm() const noexcept {
    return lastDistanceCm_;
}

