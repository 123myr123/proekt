#include "device_manager.h"

#include <jsoncpp/json/json.h>

#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <thread>

namespace {

std::string currentTimestamp() {
    using namespace std::chrono;
    auto now = system_clock::now();
    auto timeT = system_clock::to_time_t(now);
    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &timeT);
#else
    localtime_r(&timeT, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

void logInfo(const std::string& message) {
    std::cout << "[" << currentTimestamp() << "][INFO] " << message << std::endl;
}

void logError(const std::string& message) {
    std::cerr << "[" << currentTimestamp() << "][ERROR] " << message << std::endl;
}

} // namespace

DeviceManager::DeviceManager() = default;

bool DeviceManager::loadConfig(const std::string& path) {
    std::ifstream in(path);
    if (!in.is_open()) {
        logError("Failed to open config file: " + path);
        return false;
    }

    Json::Value root;
    in >> root;

    if (root.isMember("gpio_chip")) {
        config_.gpioChip = root["gpio_chip"].asString();
    }
    if (root.isMember("dht_pin")) {
        config_.dhtPin = root["dht_pin"].asInt();
    }
    if (root.isMember("ultrasonic_trigger_pin")) {
        config_.ultrasonicTriggerPin = root["ultrasonic_trigger_pin"].asInt();
    }
    if (root.isMember("ultrasonic_echo_pin")) {
        config_.ultrasonicEchoPin = root["ultrasonic_echo_pin"].asInt();
    }
    if (root.isMember("button_pin")) {
        config_.buttonPin = root["button_pin"].asInt();
    }
    if (root.isMember("led_pin")) {
        config_.ledPin = root["led_pin"].asInt();
    }
    if (root.isMember("relay_pin")) {
        config_.relayPin = root["relay_pin"].asInt();
    }
    if (root.isMember("poll_interval_ms")) {
        config_.pollInterval = std::chrono::milliseconds(root["poll_interval_ms"].asInt());
    }

    logInfo("Configuration loaded from " + path);
    return true;
}

bool DeviceManager::initDevices() {
    logInfo("Initializing sensors and actuators...");

    dht_ = std::make_unique<DhtSensor>("DHT", config_.gpioChip, config_.dhtPin);
    ultrasonic_ = std::make_unique<UltrasonicSensor>("Ultrasonic", config_.gpioChip,
                                                     config_.ultrasonicTriggerPin, config_.ultrasonicEchoPin);
    button_ = std::make_unique<DigitalSensor>("Button", config_.gpioChip, config_.buttonPin);
    led_ = std::make_unique<LedActuator>("LED", config_.gpioChip, config_.ledPin);
    relay_ = std::make_unique<RelayActuator>("Relay", config_.gpioChip, config_.relayPin);

    if (!dht_->init()) {
        logError("Failed to initialize DHT sensor");
        return false;
    }
    if (!ultrasonic_->init()) {
        logError("Failed to initialize ultrasonic sensor");
        return false;
    }
    if (!button_->init()) {
        logError("Failed to initialize digital sensor (button)");
        return false;
    }
    if (!led_->init()) {
        logError("Failed to initialize LED actuator");
        return false;
    }
    if (!relay_->init()) {
        logError("Failed to initialize relay actuator");
        return false;
    }
    database_ = std::make_unique<Database>(config_.databasePath);
    if (!database_->open()) {
        logError("failed to open SQLite database: "+config_.databasePath);
        return false;
    }

    initialized_ = true;
    logInfo("All devices initialized successfully");
    return true;
}

void DeviceManager::runMainLoop() {
    if (!initialized_) {
        logError("DeviceManager::runMainLoop called before initialization");
        return;
    }

    logInfo("Starting main loop. Press Ctrl+C to stop.");

    while (true) {
        if (dht_->read()) {
            status_.temperatureC = dht_->getTemperatureC();
            status_.humidityPercent = dht_->getHumidityPercent();
        }

        if (ultrasonic_->read()) {
            status_.distanceCm = ultrasonic_->getDistanceCm();
        }

        if (button_->read()) {
            status_.buttonActive = button_->isActive();
        }

        const float distanceThresholdCm = 20.0f;

        bool desiredLedState = status_.buttonActive;
        if (desiredLedState != status_.ledOn) {
            if (led_->setState(desiredLedState)) {
                status_.ledOn = desiredLedState;
            }
        }

        bool desiredRelayState = status_.distanceCm > 0.0f && status_.distanceCm < distanceThresholdCm;
        if (desiredRelayState != status_.relayOn) {
            if (relay_->setState(desiredRelayState)) {
                status_.relayOn = desiredRelayState;
            }
        }

        logInfo("T=" + std::to_string(status_.temperatureC) +
                "C, H=" + std::to_string(status_.humidityPercent) +
                "%, D=" + std::to_string(status_.distanceCm) +
                "cm, Button=" + std::string(status_.buttonActive ? "ON" : "OFF") +
                ", LED=" + std::string(status_.ledOn ? "ON" : "OFF") +
                ", Relay=" + std::string(status_.relayOn ? "ON" : "OFF"));

        std::this_thread::sleep_for(config_.pollInterval);
    }
}

DeviceStatus DeviceManager::getStatus() const noexcept {
    return status_;
}

const DeviceConfig& DeviceManager::getConfig() const noexcept {
    return config_;
}

