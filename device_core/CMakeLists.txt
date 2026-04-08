cmake_minimum_required(VERSION 3.10)

project(iot_device_core LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(SOURCES
    src/main.cpp
    src/device_manager.cpp
    src/dht_sensor.cpp
    src/ultrasonic_sensor.cpp
    src/digital_sensor.cpp
    src/led_actuator.cpp
    src/relay_actuator.cpp
)

add_executable(iot_device_core ${SOURCES})

target_include_directories(iot_device_core
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GPIOD REQUIRED libgpiod)
pkg_check_modules(JSONCPP REQUIRED jsoncpp)

target_include_directories(iot_device_core
    PRIVATE
        ${GPIOD_INCLUDE_DIRS}
        ${JSONCPP_INCLUDE_DIRS}
)

target_link_libraries(iot_device_core
    PRIVATE
        ${GPIOD_LIBRARIES}
        ${JSONCPP_LIBRARIES}
)

target_compile_options(iot_device_core
    PRIVATE
        ${GPIOD_CFLAGS_OTHER}
        ${JSONCPP_CFLAGS_OTHER}
)

