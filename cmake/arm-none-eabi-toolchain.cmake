set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# Locked toolchain entrypoint.  The normal workspace uses the checked/pinned
# toolchain at tools/arm-gnu-toolchain.  Use DM_ARM_TOOLCHAIN_ROOT only when the
# same toolchain is intentionally stored outside this checkout.
if(DEFINED ENV{DM_ARM_TOOLCHAIN_ROOT} AND NOT "$ENV{DM_ARM_TOOLCHAIN_ROOT}" STREQUAL "")
    file(TO_CMAKE_PATH "$ENV{DM_ARM_TOOLCHAIN_ROOT}" DM_ARM_TOOLCHAIN_ROOT)
else()
    get_filename_component(DM_ARM_TOOLCHAIN_ROOT
        "${CMAKE_CURRENT_LIST_DIR}/../tools/arm-gnu-toolchain" ABSOLUTE)
endif()

set(DM_ARM_TOOLCHAIN_BIN "${DM_ARM_TOOLCHAIN_ROOT}/bin")
set(CMAKE_C_COMPILER "${DM_ARM_TOOLCHAIN_BIN}/arm-none-eabi-gcc")
set(CMAKE_ASM_COMPILER "${DM_ARM_TOOLCHAIN_BIN}/arm-none-eabi-gcc")
set(CMAKE_AR "${DM_ARM_TOOLCHAIN_BIN}/arm-none-eabi-ar")
set(CMAKE_OBJCOPY "${DM_ARM_TOOLCHAIN_BIN}/arm-none-eabi-objcopy")
set(CMAKE_OBJDUMP "${DM_ARM_TOOLCHAIN_BIN}/arm-none-eabi-objdump")
set(CMAKE_SIZE "${DM_ARM_TOOLCHAIN_BIN}/arm-none-eabi-size")
set(CMAKE_NM "${DM_ARM_TOOLCHAIN_BIN}/arm-none-eabi-nm")
set(CMAKE_READELF "${DM_ARM_TOOLCHAIN_BIN}/arm-none-eabi-readelf")

foreach(tool CMAKE_C_COMPILER CMAKE_AR CMAKE_OBJCOPY CMAKE_OBJDUMP CMAKE_SIZE CMAKE_NM CMAKE_READELF)
    if(NOT EXISTS "${${tool}}")
        message(FATAL_ERROR
            "Required ARM toolchain program not found: ${${tool}}\n"
            "Expected locked toolchain root: ${DM_ARM_TOOLCHAIN_ROOT}\n"
            "Install/extract arm-gnu-toolchain there, or set DM_ARM_TOOLCHAIN_ROOT explicitly.")
    endif()
endforeach()
