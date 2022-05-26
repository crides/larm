set(BOARD feather_nrf52840)
list(APPEND BOARD_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
cmake_minimum_required(VERSION 3.13.1)
find_package(Zephyr)
project(larm)

# Add your source file to the "app" target. This must come after
# find_package(Zephyr) which defines the target.
target_include_directories(app PRIVATE include)
target_sources(app PRIVATE src/main.c src/ble.c)
