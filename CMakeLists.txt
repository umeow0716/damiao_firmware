cmake_minimum_required(VERSION 3.20)

project(damiao_firmware LANGUAGES C ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(DDL_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/third_party/HC32F448_DDL_Rev1.3.0")
set(DDL_CMSIS "${DDL_ROOT}/drivers/cmsis")
set(DDL_DEVICE "${DDL_CMSIS}/Device/HDSC/hc32f4xx")
set(DDL_LL "${DDL_ROOT}/drivers/hc32_ll_driver")

if(NOT EXISTS "${DDL_DEVICE}/Include/hc32f448.h")
    message(FATAL_ERROR "HC32F448 DDL not found at ${DDL_ROOT}")
endif()

set(HC32F448_CPU_FLAGS
    -mcpu=cortex-m4
    -mthumb
    -mfpu=fpv4-sp-d16
    -mfloat-abi=hard
)

set(FIRMWARE_COMPILE_FLAGS
    ${HC32F448_CPU_FLAGS}
    -ffunction-sections
    -fdata-sections
    -fno-common
    -fno-strict-aliasing
    # The official ARM/Keil image uses the legacy non-fused VMLA sequence.
    # GCC otherwise emits VFMA and diverges after sustained control ticks.
    -ffp-contract=off
    -Wall
    -Wextra
    -Wshadow
    -Wdouble-promotion
    -Wformat=2
    -Wformat-overflow=2
    -Wformat-truncation=2
    -Warray-bounds=2
    -Warray-parameter=2
    -Wstringop-overflow=4
    -Wstringop-truncation
    -Wnull-dereference
    -Wconversion
    -Wsign-conversion
    -Wfloat-conversion
    -Wenum-conversion
    -Wswitch-enum
    -Wshift-overflow=2
    -Wshift-negative-value
    -Wcast-align=strict
    -Wcast-qual
    -Wcast-function-type
    -Wpointer-arith
    -Wwrite-strings
    -Wmissing-prototypes
    -Wmissing-declarations
    -Wstrict-prototypes
    -Wold-style-definition
    -Wredundant-decls
    -Wjump-misses-init
    -Wlogical-op
    -Wlogical-not-parentheses
    -Wduplicated-cond
    -Wduplicated-branches
    -Wundef
    -Wvla
    -Walloca
    -Walloc-zero
    -Walloc-size
    -Wtrampolines
    -Wstrict-overflow=2
    -Wunsafe-loop-optimizations
    -Wvolatile-register-var
    -Wzero-length-bounds
    -Wstack-usage=1024
    -Wframe-larger-than=1024
    # A warning in firmware code is a build failure.  This keeps the normal
    # development artifact under the same warning policy as the static audit.
    -Werror
)

set(APP_PLATFORM_SOURCE
    "${CMAKE_CURRENT_SOURCE_DIR}/app/src/platform.c"
    CACHE FILEPATH "App board-port implementation")
set(BOARD_LED_SOURCE
    "${CMAKE_CURRENT_SOURCE_DIR}/board/src/board_led_hc32f448.c"
    CACHE FILEPATH "Board status-LED implementation")
set(BOARD_MCAN_SOURCE
    "${CMAKE_CURRENT_SOURCE_DIR}/board/src/board_mcan_hc32f448.c"
    CACHE FILEPATH "Board MCAN implementation")
set(BOARD_UART_SOURCE
    "${CMAKE_CURRENT_SOURCE_DIR}/board/src/board_uart_hc32f448.c"
    CACHE FILEPATH "Board debug-UART implementation")
set(BOARD_POSITION_SOURCE
    "${CMAKE_CURRENT_SOURCE_DIR}/board/src/board_position_hc32f448.c"
    CACHE FILEPATH "Board SPI/DMA position-sensor implementation")
set(BOARD_ADC_SOURCE
    "${CMAKE_CURRENT_SOURCE_DIR}/board/src/board_adc_hc32f448.c"
    CACHE FILEPATH "Board synchronous-ADC implementation")
set(BOARD_SAMPLING_TIMER_SOURCE
    "${CMAKE_CURRENT_SOURCE_DIR}/board/src/board_sampling_timer_hc32f448.c"
    CACHE FILEPATH "Board TMR4 sampling time-base implementation")
set(BOARD_CLOCK_SOURCE
    "${CMAKE_CURRENT_SOURCE_DIR}/board/src/board_clock_hc32f448.c"
    CACHE FILEPATH "Board system-clock implementation")
if(NOT EXISTS "${APP_PLATFORM_SOURCE}")
    message(FATAL_ERROR "APP_PLATFORM_SOURCE does not exist")
endif()
if(NOT EXISTS "${BOARD_LED_SOURCE}")
    message(FATAL_ERROR "BOARD_LED_SOURCE does not exist")
endif()
if(NOT EXISTS "${BOARD_MCAN_SOURCE}")
    message(FATAL_ERROR "BOARD_MCAN_SOURCE does not exist")
endif()
if(NOT EXISTS "${BOARD_UART_SOURCE}")
    message(FATAL_ERROR "BOARD_UART_SOURCE does not exist")
endif()
if(NOT EXISTS "${BOARD_POSITION_SOURCE}")
    message(FATAL_ERROR "BOARD_POSITION_SOURCE does not exist")
endif()
if(NOT EXISTS "${BOARD_ADC_SOURCE}")
    message(FATAL_ERROR "BOARD_ADC_SOURCE does not exist")
endif()
if(NOT EXISTS "${BOARD_SAMPLING_TIMER_SOURCE}")
    message(FATAL_ERROR "BOARD_SAMPLING_TIMER_SOURCE does not exist")
endif()
if(NOT EXISTS "${BOARD_CLOCK_SOURCE}")
    message(FATAL_ERROR "BOARD_CLOCK_SOURCE does not exist")
endif()

add_library(dm4310_common STATIC
    common/src/motor_math.c
    common/generated/motor_sine_table.c
    common/src/crc32.c
    common/src/crc8_maxim.c
    common/src/aes256_ctr.c
    common/src/boot_record.c
)
target_include_directories(dm4310_common
    PUBLIC common/include
    PRIVATE common/generated
)
target_compile_options(dm4310_common PRIVATE ${FIRMWARE_COMPILE_FLAGS})

function(dm4310_firmware TARGET_NAME LINKER_SCRIPT VECTOR_OFFSET)
    if(NOT TARGET_NAME STREQUAL "dm4310_app")
        message(FATAL_ERROR "Only dm4310_app is supported in this app-only workspace")
    endif()
    target_include_directories(${TARGET_NAME} PRIVATE
        common/include
        app/include
        app/generated
        board/include
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${DDL_CMSIS}/Include
        ${DDL_DEVICE}/Include
        ${DDL_LL}/inc
    )
    target_compile_definitions(${TARGET_NAME} PRIVATE
        HC32F448
        VECT_TAB_OFFSET=${VECTOR_OFFSET}
    )
    target_compile_options(${TARGET_NAME} PRIVATE ${FIRMWARE_COMPILE_FLAGS})
    target_link_options(${TARGET_NAME} PRIVATE
        ${HC32F448_CPU_FLAGS}
        -nostartfiles
        --specs=nano.specs
        --specs=nosys.specs
        -Wl,--gc-sections
        -Wl,-z,noexecstack
        -Wl,--no-warn-rwx-segments
        -Wl,--cref
        -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}.map
        -T${LINKER_SCRIPT}
    )
    target_link_libraries(${TARGET_NAME} PRIVATE dm4310_common m)
    target_sources(${TARGET_NAME} PRIVATE
        common/src/syscalls.c
        board/src/board_delay_hc32f448.c
        board/src/board_clock_config.c
        board/src/board_flash_hc32f448.c
        ${BOARD_CLOCK_SOURCE}
        ${BOARD_LED_SOURCE}
    )
    set_target_properties(${TARGET_NAME} PROPERTIES SUFFIX ".elf")

    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O binary
            $<TARGET_FILE:${TARGET_NAME}>
            ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}.bin
        COMMAND ${CMAKE_OBJCOPY} -O ihex
            $<TARGET_FILE:${TARGET_NAME}>
            ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}.hex
        COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${TARGET_NAME}>
        VERBATIM
    )
endfunction()

add_executable(dm4310_app
    startup/startup_hc32f448.S
    ${DDL_DEVICE}/Source/system_hc32f448.c
    app/src/main.c
    app/src/app_config.c
    app/src/calibration_store.c
    app/src/calibration_upload.c
    app/src/commissioning.c
    app/src/commissioning_math.c
    app/src/app_commands.c
    app/src/app_state.c
    app/src/motor_control.c
    app/src/output_sensor.c
    app/src/position_sensor.c
    app/src/sensor_calibration.c
    app/src/safety.c
    app/src/can_protocol.c
    app/src/parameter_protocol.c
    app/src/debug_console.c
    app/src/firmware_control.c
    app/src/interrupts.c
    app/src/device_auth.c
    ${APP_PLATFORM_SOURCE}
    board/src/board_mcan_config.c
    board/src/board_uart_config.c
    board/src/board_position_config.c
    board/src/board_adc_config.c
    board/src/board_sampling_timer_config.c
    board/src/board_power_stage_config.c
    board/src/board_power_stage_hc32f448.c
    board/src/board_crc_hc32f448.c
    board/src/board_identity_config.c
    board/src/board_identity_hc32f448.c
    ${BOARD_ADC_SOURCE}
    ${BOARD_SAMPLING_TIMER_SOURCE}
    ${BOARD_MCAN_SOURCE}
    ${BOARD_POSITION_SOURCE}
    ${BOARD_UART_SOURCE}
    ${DDL_LL}/src/hc32_ll_mcan.c
    app/generated/temperature_table.c
)
dm4310_firmware(dm4310_app
    "${CMAKE_CURRENT_SOURCE_DIR}/linker/hc32f448_app.ld"
    0x00000000UL)

