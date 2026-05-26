cmake_minimum_required(VERSION 3.20)

project(ifd9000-firmware C ASM)

set(CMAKE_C_STANDARD 11)

set(MCU_FLAGS
    -mcpu=cortex-m0plus
    -mthumb
    -mfloat-abi=soft
)

add_subdirectory(boards/payload_ctrl)