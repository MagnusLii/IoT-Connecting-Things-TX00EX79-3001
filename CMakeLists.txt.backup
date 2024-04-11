cmake_minimum_required(VERSION 3.22.0)

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

set(PICO_BOARD pico_w)

project(ventrilation VERSION 0.1.0 LANGUAGES C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()
include_directories(${CMAKE_CURRENT_LIST_DIR}/src/pico_hw)
include_directories(${CMAKE_CURRENT_LIST_DIR}/src)

add_compile_options(-Wall)

add_executable(${PROJECT_NAME} main.cpp
        src/display/mono_vlsb.cpp
        src/display/framebuf.cpp
        src/display/ssd1306.cpp)

pico_add_extra_outputs(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME} pico_stdlib hardware_i2c)

pico_enable_stdio_usb(${PROJECT_NAME} 0)
pico_enable_stdio_uart(${PROJECT_NAME} 1)
