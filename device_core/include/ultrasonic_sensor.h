#pragma once

#include "sensor_base.h"

#include <gpiod.h>
#include <string>

class UltrasonicSensor : public SensorBase {
public:
    UltrasonicSensor(const std::string& name,
                     const std::string& chipName,
                     int triggerOffset,
                     int echoOffset,
                     bool simulate = false);
    ~UltrasonicSensor() override;

    bool init() override;
    bool read() override;

    [[nodiscard]] float getDistanceCm() const noexcept;

private:
    std::string chipName_;
    int triggerOffset_;
    int echoOffset_;
    bool simulate_{false};

    gpiod_chip* chip_{nullptr};
    gpiod_line* triggerLine_{nullptr};
    gpiod_line* echoLine_{nullptr};

    float lastDistanceCm_{0.0f};
};

