cmake_minimum_required(VERSION 3.13)

set(PICO_SDK_PATH "~/pico/pico-sdk")
option(build_tests OFF)

if(build_tests STREQUAL "OFF")
    include(${PICO_SDK_PATH}/pico_sdk_init.cmake)
endif()

project(picosyn C)

set(CMAKE_C_STANDARD 11)

include_directories(include)

if(build_tests STREQUAL "ON")
    add_subdirectory(tests)
endif()

if(build_tests STREQUAL "OFF")
    add_subdirectory(src)
endif()

