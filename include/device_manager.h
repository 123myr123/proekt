#pragma once

#include "dht_sensor.h"
#include "digital_sensor.h"
#include "led_actuator.h"
#include "relay_actuator.h"
#include "ultrasonic_sensor.h"

#include <chrono>
#include <memory>
#include <string>

struct DeviceConfig {
    std::string gpioChip{"gpiochip0"};

    int dhtPin{17};
    int ultrasonicTriggerPin{23};
    int ultrasonicEchoPin{24};
    int buttonPin{25};
    int ledPin{27};
    int relayPin{22};

    std::chrono::milliseconds pollInterval{1000};
};

struct DeviceStatus {
    float temperatureC{0.0f};
    float humidityPercent{0.0f};
    float distanceCm{0.0f};
    bool buttonActive{false};
    bool ledOn{false};
    bool relayOn{false};
};

class DeviceManager {
public:
    DeviceManager();

    bool loadConfig(const std::string& path);

    bool initDevices();

    void runMainLoop();

    [[nodiscard]] DeviceStatus getStatus() const noexcept;

    [[nodiscard]] const DeviceConfig& getConfig() const noexcept;

private:
    DeviceConfig config_{};
    std::unique_ptr<DhtSensor> dht_;
    std::unique_ptr<UltrasonicSensor> ultrasonic_;
    std::unique_ptr<DigitalSensor> button_;
    std::unique_ptr<LedActuator> led_;
    std::unique_ptr<RelayActuator> relay_;

    DeviceStatus status_{};
    bool initialized_{false};
};

