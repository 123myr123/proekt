#include "device_manager.h"

#include <fcntl.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <iostream>
#include <string>

namespace {
void logError(const std::string& message) {
    std::cerr << "[ERROR] " << message << std::endl;
}

bool hasGpioAccess(const std::string& chipName) {
    std::string path = "/dev/" + chipName;
    int fd = ::open(path.c_str(), O_RDONLY);
    if (fd < 0) {
        if (errno == EACCES) {
            logError("No permission to access " + path +
                     ". Ensure user is in 'gpio' group and re-login.");
        } else {
            logError("Failed to open " + path + ": " + std::strerror(errno));
        }
        return false;
    }

    ::close(fd);
    return true;
}

}

int main(int argc, char* argv[]) {
    std::string configPath = "config.json";
    if (argc > 1) {
        configPath = argv[1];
    }

    DeviceManager manager;

    if (!manager.loadConfig(configPath)) {
        logError("Could not load configuration. Exiting.");
        return 1;
    }

    if (!manager.getConfig().simulateHardware) {
        if (!hasGpioAccess(manager.getConfig().gpioChip)) {
            logError("Insufficient permissions to access GPIO. Exiting.");
            return 1;
        }
    }

    if (!manager.initDevices()) {
        logError("Device initialization failed. Exiting.");
        return 1;
    }

    manager.runMainLoop();

    return 0;
}

