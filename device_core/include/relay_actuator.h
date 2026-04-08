#pragma once

#include "actuator_base.h"

#include <gpiod.h>
#include <string>

class RelayActuator : public ActuatorBase {
public:
    RelayActuator(const std::string& name, const std::string& chipName, int lineOffset, bool activeHigh = true);
    ~RelayActuator() override;

    bool init() override;
    bool setState(bool on) override;
    [[nodiscard]] bool getState() const override;

private:
    std::string chipName_;
    int lineOffset_;
    bool activeHigh_;

    gpiod_chip* chip_{nullptr};
    gpiod_line* line_{nullptr};
    bool state_{false};
};

