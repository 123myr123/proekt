#pragma once

#include "sensor_base.h"

#include <gpiod.h>
#include <string>

class DigitalSensor : public SensorBase {
public:
    DigitalSensor(const std::string& name,
                  const std::string& chipName,
                  int lineOffset,
                  bool activeHigh = true,
                  bool simulate = false);
    ~DigitalSensor() override;

    bool init() override;
    bool read() override;

    [[nodiscard]] bool isActive() const noexcept;

private:
    std::string chipName_;
    int lineOffset_;
    bool activeHigh_;
    bool simulate_{false};

    gpiod_chip* chip_{nullptr};
    gpiod_line* line_{nullptr};
    bool lastValue_{false};
};

