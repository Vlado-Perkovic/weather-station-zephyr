cmake_minimum_required(VERSION 3.22)
set(BOARD esp32_devkitc_wrover)

find_package(Zephyr)
project(weather-station)

target_sources(app PRIVATE src/main.c)
