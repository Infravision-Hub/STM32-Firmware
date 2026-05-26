set(TARGET payload_ctrl)
set(LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/bsp/STM32C092CBTx_FLASH.ld)
set(SOURCES
    app/main.c
    bsp/startup_stm32c092xx.s
    ${CMAKE_SOURCE_DIR}/common/libcanard/canard.c
)
set(INCLUDE_DIRS
    bsp
    app
    ${CMAKE_SOURCE_DIR}/common/cmsis/CMSIS_6/CMSIS/Core/Include
    ${CMAKE_SOURCE_DIR}/common/cmsis/cmsis_device_c0/Include
    ${CMAKE_SOURCE_DIR}/common/libcanard
)
add_executable(${TARGET} ${SOURCES})
target_include_directories(${TARGET} PRIVATE ${INCLUDE_DIRS})
target_compile_options(${TARGET} PRIVATE
    ${MCU_FLAGS}
    -Wall
    -fdata-sections
    -ffunction-sections
    -DSTM32C092xx
)
target_link_options(${TARGET} PRIVATE
    ${MCU_FLAGS}
    -T${LINKER_SCRIPT}
    -Wl,--gc-sections
    -Wl,-Map=${TARGET}.map
    --specs=rdimon.specs
    --specs=nano.specs
)
add_custom_command(TARGET ${TARGET} POST_BUILD
    COMMAND arm-none-eabi-objcopy -O binary
            $<TARGET_FILE:${TARGET}> ${TARGET}.bin
    COMMAND arm-none-eabi-size $<TARGET_FILE:${TARGET}>
)