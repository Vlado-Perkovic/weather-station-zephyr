cmake_minimum_required(VERSION 3.22)
set(BOARD esp32_devkitc_wrover)

find_package(Zephyr)
project(weather-station)

target_sources(app PRIVATE src/main.c src/zws_utils.c src/sensors/zws_temp_and_humid_sensor.c src/sensors/zws_photoresistor_sensor.c src/wifi/zws_wifi.c)