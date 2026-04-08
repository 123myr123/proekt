#pragma once

#include "sensor_base.h"

#include <string>

class DhtSensor : public SensorBase {
public:
    DhtSensor(const std::string& name, const std::string& chipName, int lineOffset);
    ~DhtSensor() override = default;

    bool init() override;
    bool read() override;

    [[nodiscard]] float getTemperatureC() const noexcept;
    [[nodiscard]] float getHumidityPercent() const noexcept;

private:
    std::string chipName_;
    int lineOffset_;

    float lastTemperatureC_{0.0f};
    float lastHumidity_{0.0f};
};

