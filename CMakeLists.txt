cmake_minimum_required(VERSION 3.22)
set(BOARD esp32_devkitc_wrover)

find_package(Zephyr)
project(weather-station)

FILE(GLOB app_sources
    src/*.c
    src/wifi/*.c
    src/sensors/*.c
    src/telemetry/*.c
    )

target_sources(app PRIVATE ${app_sources})