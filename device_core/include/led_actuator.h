#pragma once

#include "actuator_base.h"

#include <gpiod.h>
#include <string>

class LedActuator : public ActuatorBase {
public:
    LedActuator(const std::string& name,
                const std::string& chipName,
                int lineOffset,
                bool activeHigh = true,
                bool simulate = false);
    ~LedActuator() override;

    bool init() override;
    bool setState(bool on) override;
    [[nodiscard]] bool getState() const override;

private:
    std::string chipName_;
    int lineOffset_;
    bool activeHigh_;
    bool simulate_{false};

    gpiod_chip* chip_{nullptr};
    gpiod_line* line_{nullptr};
    bool state_{false};
};

