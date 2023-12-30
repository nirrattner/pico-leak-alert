cmake_minimum_required(VERSION 3.13)

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(leak_alert_project C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(PICO_BOARD pico_w)
set(PICO_TIME_DEFAULT_ALARM_POOL_DISABLED 1)

pico_sdk_init()

add_executable(leak_alert
    audio.c
    audio.h
    main.c
    )

pico_enable_stdio_usb(leak_alert 1)
pico_enable_stdio_uart(leak_alert 0)

target_link_libraries(leak_alert
    hardware_adc
    hardware_pwm
    pico_cyw43_arch_none
    pico_stdlib
    )

pico_add_extra_outputs(leak_alert)

