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
    # Preserve the non-fused VMLA sequence required by the control arithmetic.
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

# Keep the firmware optimization policy independent of CMAKE_BUILD_TYPE so all
# source owners, including the common library, use the same setting.  Making it
# a cache value also allows reproducible size/performance comparison builds
# without editing the production configuration.
set(FIRMWARE_SIZE_OPTIMIZATION -Os CACHE STRING
    "Optimization level used for firmware sources")
set_property(CACHE FIRMWARE_SIZE_OPTIMIZATION PROPERTY STRINGS -Oz -Os -O2 -O3)

set(APP_PLATFORM_SOURCE
    "${CMAKE_CURRENT_SOURCE_DIR}/app/src/platform.c"
    CACHE FILEPATH "Default app board-port implementation")
set(BOARD_LED_SOURCE
    "${CMAKE_CURRENT_SOURCE_DIR}/board/src/board_led_hc32f448.c"
    CACHE FILEPATH "Default board status-LED implementation")
set(BOARD_MCAN_SOURCE
    "${CMAKE_CURRENT_SOURCE_DIR}/board/src/board_mcan_hc32f448.c"
    CACHE FILEPATH "Default board MCAN implementation")
set(BOARD_UART_SOURCE
    "${CMAKE_CURRENT_SOURCE_DIR}/board/src/board_uart_hc32f448.c"
    CACHE FILEPATH "Default board debug-UART implementation")
set(BOARD_POSITION_SOURCE
    "${CMAKE_CURRENT_SOURCE_DIR}/board/src/board_position_hc32f448.c"
    CACHE FILEPATH "Default board SPI/DMA position-sensor implementation")
set(BOARD_ADC_SOURCE
    "${CMAKE_CURRENT_SOURCE_DIR}/board/src/board_adc_hc32f448.c"
    CACHE FILEPATH "Default board synchronous-ADC implementation")
set(BOARD_SAMPLING_TIMER_SOURCE
    "${CMAKE_CURRENT_SOURCE_DIR}/board/src/board_sampling_timer_hc32f448.c"
    CACHE FILEPATH "Default board TMR4 sampling time-base implementation")
set(BOARD_CLOCK_SOURCE
    "${CMAKE_CURRENT_SOURCE_DIR}/board/src/board_clock_hc32f448.c"
    CACHE FILEPATH "Default board system-clock implementation")

function(damiao_require_existing_file VARIABLE_NAME)
    if(NOT DEFINED ${VARIABLE_NAME})
        message(FATAL_ERROR "${VARIABLE_NAME} is not defined")
    endif()
    if(NOT EXISTS "${${VARIABLE_NAME}}")
        message(FATAL_ERROR "${VARIABLE_NAME} does not exist: ${${VARIABLE_NAME}}")
    endif()
endfunction()

add_library(damiao_common STATIC
    common/src/motor_math.c
    common/generated/motor_sine_table.c
    common/src/crc32.c
    common/src/crc8_maxim.c
    common/src/aes256_ctr.c
    common/src/boot_record.c
)
target_include_directories(damiao_common
    PUBLIC common/include
    PRIVATE common/generated
)
target_compile_options(damiao_common PRIVATE
    ${FIRMWARE_COMPILE_FLAGS}
    ${FIRMWARE_SIZE_OPTIMIZATION})

set(DAMIAO_APP_SOURCES
    startup/startup_hc32f448.S
    ${DDL_DEVICE}/Source/system_hc32f448.c
    app/src/main.c
    app/src/app_config.c
    app/src/calibration_store.c
    app/src/calibration_upload.c
    app/src/commissioning.c
    app/src/commissioning_math.c
    app/src/sram_runtime.c
    app/src/runtime_compat.c
    app/src/app_commands.c
    app/src/app_state.c
    app/src/motor_control.c
    app/src/motor_lookup_storage.c
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
    ${DDL_LL}/src/hc32_ll_mcan.c
    app/generated/temperature_table.c
)

function(damiao_firmware TARGET_NAME LINKER_SCRIPT VECTOR_OFFSET)
    damiao_require_existing_file(APP_PLATFORM_SOURCE)
    damiao_require_existing_file(BOARD_LED_SOURCE)
    damiao_require_existing_file(BOARD_MCAN_SOURCE)
    damiao_require_existing_file(BOARD_UART_SOURCE)
    damiao_require_existing_file(BOARD_POSITION_SOURCE)
    damiao_require_existing_file(BOARD_ADC_SOURCE)
    damiao_require_existing_file(BOARD_SAMPLING_TIMER_SOURCE)
    damiao_require_existing_file(BOARD_CLOCK_SOURCE)

    add_executable(${TARGET_NAME}
        ${DAMIAO_APP_SOURCES}
        ${APP_PLATFORM_SOURCE}
        ${BOARD_ADC_SOURCE}
        ${BOARD_SAMPLING_TIMER_SOURCE}
        ${BOARD_MCAN_SOURCE}
        ${BOARD_POSITION_SOURCE}
        ${BOARD_UART_SOURCE}
    )
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
    foreach(definition IN LISTS DAMIAO_TARGET_COMPILE_DEFINITIONS)
        target_compile_definitions(${TARGET_NAME} PRIVATE ${definition})
    endforeach()
    target_compile_options(${TARGET_NAME} PRIVATE ${FIRMWARE_COMPILE_FLAGS})
    target_link_options(${TARGET_NAME} PRIVATE
        ${DAMIAO_TARGET_LINK_OPTIONS}
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
    target_link_libraries(${TARGET_NAME} PRIVATE damiao_common m)
    target_sources(${TARGET_NAME} PRIVATE
        common/src/syscalls.c
        board/src/board_delay_hc32f448.c
        board/src/board_clock_config.c
        board/src/board_flash_hc32f448.c
        ${BOARD_CLOCK_SOURCE}
        ${BOARD_LED_SOURCE}
    )
    set_target_properties(${TARGET_NAME} PROPERTIES SUFFIX ".elf")
    set_property(TARGET ${TARGET_NAME} APPEND PROPERTY
        LINK_DEPENDS "${LINKER_SCRIPT}")

    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O binary
            $<TARGET_FILE:${TARGET_NAME}>
            ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}.app.bin
        COMMAND ${CMAKE_OBJCOPY} -O ihex
            $<TARGET_FILE:${TARGET_NAME}>
            ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}.hex
        COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${TARGET_NAME}>
        VERBATIM
    )
endfunction()

function(damiao_add_profiled_firmware TARGET_NAME PROFILE_PATH)
    unset(DAMIAO_TARGET_COMPILE_DEFINITIONS)
    unset(DAMIAO_TARGET_LINK_OPTIONS)
    set(DAMIAO_TARGET_LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/linker/hc32f448_app.ld")
    set(DAMIAO_TARGET_VECTOR_OFFSET 0x00000000UL)
    include("${PROFILE_PATH}")
    list(APPEND DAMIAO_TARGET_COMPILE_DEFINITIONS ${ARGN})
    damiao_firmware(${TARGET_NAME}
        "${DAMIAO_TARGET_LINKER_SCRIPT}"
        ${DAMIAO_TARGET_VECTOR_OFFSET})
endfunction()

set(DAMIAO_FIRMWARE_TARGETS
    dm10010
    dm3507
    dm3507_48v
    dm4310
    dm4310_48v
    dm4340
    dm4340_48v
    dm8006
    dm8009
)
set(DAMIAO_ALL_FIRMWARE_TARGETS)
foreach(target IN LISTS DAMIAO_FIRMWARE_TARGETS)
    # Feedback behavior is orthogonal to the hardware/model profile.  Keep one
    # profile per product and derive the runtime variant matrix from it.
    damiao_add_profiled_firmware(${target}
        "${CMAKE_CURRENT_SOURCE_DIR}/config/targets/${target}.cmake")
    damiao_add_profiled_firmware(${target}_raw
        "${CMAKE_CURRENT_SOURCE_DIR}/config/targets/${target}.cmake"
        DAMIAO_FEEDBACK_VARIANT_RAW=1)
    damiao_add_profiled_firmware(${target}_no_response
        "${CMAKE_CURRENT_SOURCE_DIR}/config/targets/${target}.cmake"
        DAMIAO_COMMAND_FEEDBACK_DISABLED=1)
    damiao_add_profiled_firmware(${target}_raw_no_response
        "${CMAKE_CURRENT_SOURCE_DIR}/config/targets/${target}.cmake"
        DAMIAO_FEEDBACK_VARIANT_RAW=1
        DAMIAO_COMMAND_FEEDBACK_DISABLED=1)
    list(APPEND DAMIAO_ALL_FIRMWARE_TARGETS
        ${target}
        ${target}_raw
        ${target}_no_response
        ${target}_raw_no_response)
endforeach()

# The V3 applications occupy a fixed 64 KiB APP partition and share the same
# arithmetic/runtime implementation. Per-model constants and fixed SRAM
# placement remain selected by the target profile and linker.
set(DAMIAO_V3_RUNTIME_SOURCES
    app/src/softfloat_binary64.c
    app/src/extended_arithmetic.c
    app/src/extended_arithmetic_core.S
    app/src/decimal_power.c
    app/src/fixed_decimal.c
)
foreach(target IN LISTS DAMIAO_ALL_FIRMWARE_TARGETS)
    target_compile_options(${target} PRIVATE ${FIRMWARE_SIZE_OPTIMIZATION})
    target_sources(${target} PRIVATE ${DAMIAO_V3_RUNTIME_SOURCES})
endforeach()
