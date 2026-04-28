#include "dht_sensor.h"

#include <chrono>
#include <iostream>
#include <random>

namespace {

void logError(const std::string& message) {
    std::cerr << "[ERROR] " << message << std::endl;
}

float randomInRange(float min, float max) {
    static std::mt19937 rng{std::random_device{}()};
    std::uniform_real_distribution<float> dist(min, max);
    return dist(rng);
}

}

DhtSensor::DhtSensor(const std::string& name,
                     const std::string& chipName,
                     int lineOffset)
    : SensorBase(name),
      chipName_(chipName),
      lineOffset_(lineOffset) {}

bool DhtSensor::init() {
    (void)chipName_;
    (void)lineOffset_;
    return true;
}

bool DhtSensor::read() {
    lastTemperatureC_ = randomInRange(20.0f, 30.0f);
    lastHumidity_ = randomInRange(30.0f, 70.0f);
    return true;
}

float DhtSensor::getTemperatureC() const noexcept {
    return lastTemperatureC_;
}

float DhtSensor::getHumidityPercent() const noexcept {
    return lastHumidity_;
}

