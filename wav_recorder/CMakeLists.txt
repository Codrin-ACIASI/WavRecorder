# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()

set(PICO_BOARD pico2_w CACHE STRING "Board type")

include(pico_sdk_import.cmake)

project(wav_recorder C CXX ASM)

pico_sdk_init()

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(lvgl)

add_subdirectory(no-OS-FatFS-SD-SDIO-SPI-RPi-Pico/src build)

add_executable(wav_recorder
    buttons.c
    wav_recorder.c
    display_port.c
    ui.c
    hw_config.c
    sd.c
)

pico_set_program_name(wav_recorder "wav_recorder")
pico_set_program_version(wav_recorder "0.1")

pico_enable_stdio_uart(wav_recorder 0)
pico_enable_stdio_usb(wav_recorder 1)

target_link_libraries(wav_recorder 
    pico_stdlib 
    hardware_spi 
    hardware_gpio 
    pico_time
    lvgl  
    no-OS-FatFS-SD-SDIO-SPI-RPi-Pico
    hardware_pio
    hardware_dma
    hardware_clocks
    hardware_irq
)

target_include_directories(wav_recorder PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
)

pico_add_extra_outputs(wav_recorder)