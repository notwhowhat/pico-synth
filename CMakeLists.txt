# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_BUILD_TYPE Debug)

# initalize pico_sdk from installed location
# (note this can come from environment, CMake cache etc)
# set(PICO_SDK_PATH "/YOUR_PICO_SDK_PATH/pico-sdk")
set(PICO_SDK_PATH "~/pico/pico-sdk")

# Pull in Pico SDK (must be before project)
include(pico_sdk_import.cmake)

# i have force disabled cxx in pico_sdk
project(picosyn C)

# Initialise the Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(picosyn src/picosyn.c )

pico_set_program_name(picosyn "picosyn")
pico_set_program_version(picosyn "0.1")

pico_enable_stdio_uart(picosyn 1)
pico_enable_stdio_usb(picosyn 1)

# Add the standard library to the build
target_link_libraries(picosyn pico_stdlib)

# Add any user requested libraries
target_link_libraries(picosyn
        pico_stdlib
        hardware_dma
        hardware_pio
        hardware_timer
        hardware_clocks
        hardware_pwm
        )

pico_add_extra_outputs(picosyn)

