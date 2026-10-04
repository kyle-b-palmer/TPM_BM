cmake_minimum_required(VERSION 3.20)
project(tpm_bare_metal_tools C ASM)

option(TPMBM_BUILD_HOST "Build host_sim + tests (native)" ON)
option(TPMBM_BUILD_FIRMWARE "Build Nucleo-U545RE-Q firmware" OFF)

set(TPMBM_CORE_SOURCES
    src/core/tpmbm.c
    src/core/tpmbm_cmd.c
    src/tis/tpmbm_tis_i2c.c
)

add_library(tpmbm_core STATIC ${TPMBM_CORE_SOURCES})
target_include_directories(tpmbm_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(tpmbm_core PUBLIC c_std_11)

if(TPMBM_BUILD_HOST)
    add_executable(tpmbm_host_sim
        src/ports/host_sim/main.c
        src/ports/host_sim/host_sim_hal.c
    )
    target_include_directories(tpmbm_host_sim PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ports/host_sim
    )
    target_link_libraries(tpmbm_host_sim PRIVATE tpmbm_core)

    enable_testing()
    add_executable(tpmbm_host_tests
        tests/test_host_sim.c
        src/ports/host_sim/host_sim_hal.c
    )
    target_include_directories(tpmbm_host_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ports/host_sim
    )
    target_link_libraries(tpmbm_host_tests PRIVATE tpmbm_core)
    add_test(NAME host_sim COMMAND tpmbm_host_tests)
endif()

if(TPMBM_BUILD_FIRMWARE)
    if(NOT CMAKE_CROSSCOMPILING)
        message(WARNING
            "TPMBM_BUILD_FIRMWARE=ON without a cross toolchain. "
            "Reconfigure with -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-arm-none-eabi.cmake")
    endif()

    set(TPMBM_THIRD_PARTY ${CMAKE_CURRENT_SOURCE_DIR}/third_party)
    set(CMSIS_CORE ${TPMBM_THIRD_PARTY}/cmsis_core/CMSIS/Core/Include)
    set(CMSIS_DEVICE ${TPMBM_THIRD_PARTY}/cmsis_device_u5)
    set(STM32_HAL ${TPMBM_THIRD_PARTY}/stm32u5xx_hal_driver)

    foreach(dir IN ITEMS ${CMSIS_CORE} ${CMSIS_DEVICE}/Include ${STM32_HAL}/Inc)
        if(NOT EXISTS ${dir})
            message(FATAL_ERROR
                "Missing ${dir}. Run ./scripts/fetch-deps.sh before building firmware.")
        endif()
    endforeach()

    set(HAL_SOURCES
        ${STM32_HAL}/Src/stm32u5xx_hal.c
        ${STM32_HAL}/Src/stm32u5xx_hal_cortex.c
        ${STM32_HAL}/Src/stm32u5xx_hal_gpio.c
        ${STM32_HAL}/Src/stm32u5xx_hal_i2c.c
        ${STM32_HAL}/Src/stm32u5xx_hal_i2c_ex.c
        ${STM32_HAL}/Src/stm32u5xx_hal_rcc.c
        ${STM32_HAL}/Src/stm32u5xx_hal_rcc_ex.c
        ${STM32_HAL}/Src/stm32u5xx_hal_pwr.c
        ${STM32_HAL}/Src/stm32u5xx_hal_pwr_ex.c
        ${STM32_HAL}/Src/stm32u5xx_hal_dma.c
        ${STM32_HAL}/Src/stm32u5xx_hal_dma_ex.c
        ${STM32_HAL}/Src/stm32u5xx_hal_flash.c
        ${STM32_HAL}/Src/stm32u5xx_hal_flash_ex.c
        ${STM32_HAL}/Src/stm32u5xx_hal_uart.c
        ${STM32_HAL}/Src/stm32u5xx_hal_uart_ex.c
        ${STM32_HAL}/Src/stm32u5xx_hal_icache.c
        ${CMSIS_DEVICE}/Source/Templates/system_stm32u5xx.c
        ${CMSIS_DEVICE}/Source/Templates/gcc/startup_stm32u545xx.s
    )

    add_executable(tpmbm_nucleo_u545re_q.elf
        src/ports/nucleo_u545re_q/main.c
        src/ports/nucleo_u545re_q/board.c
        ${HAL_SOURCES}
    )
    target_link_libraries(tpmbm_nucleo_u545re_q.elf PRIVATE tpmbm_core)
    target_include_directories(tpmbm_nucleo_u545re_q.elf PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ports/nucleo_u545re_q
        ${CMSIS_CORE}
        ${CMSIS_DEVICE}/Include
        ${STM32_HAL}/Inc
        ${STM32_HAL}/Inc/Legacy
    )
    target_compile_definitions(tpmbm_nucleo_u545re_q.elf PRIVATE
        STM32U545xx
        USE_HAL_DRIVER
    )
    target_link_options(tpmbm_nucleo_u545re_q.elf PRIVATE
        -T${CMSIS_DEVICE}/Source/Templates/gcc/linker/STM32U545xx_FLASH.ld
        -Wl,-Map=$<TARGET_FILE_DIR:tpmbm_nucleo_u545re_q.elf>/tpmbm_nucleo_u545re_q.map
    )

    add_custom_command(TARGET tpmbm_nucleo_u545re_q.elf POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O binary
            $<TARGET_FILE:tpmbm_nucleo_u545re_q.elf>
            $<TARGET_FILE_DIR:tpmbm_nucleo_u545re_q.elf>/tpmbm_nucleo_u545re_q.bin
        COMMAND ${CMAKE_OBJCOPY} -O ihex
            $<TARGET_FILE:tpmbm_nucleo_u545re_q.elf>
            $<TARGET_FILE_DIR:tpmbm_nucleo_u545re_q.elf>/tpmbm_nucleo_u545re_q.hex
        COMMAND ${CMAKE_SIZE} $<TARGET_FILE:tpmbm_nucleo_u545re_q.elf>
        COMMENT "Generating bin/hex and printing size"
    )
endif()
