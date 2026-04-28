#pragma once

#include "database.h"
#include "dht_sensor.h"
#include "digital_sensor.h"
#include "led_actuator.h"
#include "relay_actuator.h"
#include "ultrasonic_sensor.h"

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

    float relayDistanceThresholdCm{20.0f};
    int pollIntervalMs{1000};

    std::string databasePath{"../data/device.db"};

    bool simulateHardware{false};
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

    DeviceStatus getStatus() const;

    const DeviceConfig& getConfig() const;

private:
    DeviceConfig config_{};
    std::unique_ptr<DhtSensor> dht_;
    std::unique_ptr<UltrasonicSensor> ultrasonic_;
    std::unique_ptr<DigitalSensor> button_;
    std::unique_ptr<LedActuator> led_;
    std::unique_ptr<RelayActuator> relay_;
    std::unique_ptr<Database> database_;

    DeviceStatus status_{};
    bool initialized_{false};
};

