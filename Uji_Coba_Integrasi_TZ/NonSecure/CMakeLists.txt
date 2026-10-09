cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Compiler options
set(STM32_MCU_FLAGS  "-mcpu=cortex-m33 -mfpu=fpv5-sp-d16 -mfloat-abi=hard " )

#Linker options
set(STM32_LINKER_SCRIPT STM32U575xx_FLASH_ns.ld)
set(STM32_LINKER_OPTION  )

# Set the project name
set(CMAKE_PROJECT_NAME Uji_Coba_Integrasi_TZ_NS)
project(${CMAKE_PROJECT_NAME})

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Include mx-generated
include("mx-generated.cmake")


# In order to use CMake for cross-compiling
set(CMAKE_CXX_COMPILER_FORCED "true")

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    USE_FREERTOS
    EMBEDDED_HARDWARE_TARGET
)

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined include paths
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../firmware/sys/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../firmware/FreeRTOS/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../firmware/FreeRTOS/portable/GCC/ARM_CM33_NTZ/non_secure
    ${CMAKE_CURRENT_SOURCE_DIR}/../firmware/app/display/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../firmware/app/tamper/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../firmware/app/fsm/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../firmware/app/profiling/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../firmware/app/console/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../firmware/protocols/dlms/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../firmware/drivers/bsp/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../E1/Adapter
    ${CMAKE_CURRENT_SOURCE_DIR}/../E1/Metrology
    ${CMAKE_CURRENT_SOURCE_DIR}/../E1/Mock
    ${CMAKE_CURRENT_SOURCE_DIR}/../E1/SPI
    ${CMAKE_CURRENT_SOURCE_DIR}/../E1/RegisterMap
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user sources here
    # FreeRTOS Kernel Sources
    ${CMAKE_CURRENT_SOURCE_DIR}/../firmware/FreeRTOS/tasks.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../firmware/FreeRTOS/queue.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../firmware/FreeRTOS/list.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../firmware/FreeRTOS/timers.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../firmware/FreeRTOS/event_groups.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../firmware/FreeRTOS/stream_buffer.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../firmware/FreeRTOS/portable/MemMang/heap_4.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../firmware/FreeRTOS/portable/GCC/ARM_CM33_NTZ/non_secure/port.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../firmware/FreeRTOS/portable/GCC/ARM_CM33_NTZ/non_secure/portasm.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../firmware/sys/src/freertos_hooks.c

    # Application (E3) Sources
    ${CMAKE_CURRENT_SOURCE_DIR}/../firmware/app/display/src/display.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../firmware/app/display/src/display_task.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../firmware/app/tamper/src/tamper_manager.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../firmware/app/fsm/src/fsm_statechart.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../firmware/app/fsm/src/rtos_tasks.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../firmware/app/profiling/src/load_profile.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../firmware/app/profiling/src/load_profile_dlms_adapter.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../firmware/app/profiling/src/nvram_storage.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../firmware/app/console/src/console_io.c

    # Protocol (E2) & BSP Sources
    ${CMAKE_CURRENT_SOURCE_DIR}/../firmware/protocols/dlms/src/dlms_association.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../firmware/protocols/dlms/src/dlms_transport.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../firmware/protocols/dlms/src/dlms_apdu.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../firmware/protocols/dlms/src/dlms_obis.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../firmware/protocols/dlms/src/dlms_tamper_log.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../firmware/protocols/dlms/src/dlms_hdlc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../firmware/protocols/dlms/src/dlms_server.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../firmware/protocols/dlms/src/dlms_task.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../firmware/protocols/dlms/src/dlms_nvm.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../firmware/drivers/bsp/src/bsp_lpuart_dma.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../firmware/drivers/bsp/src/bsp_display_oled.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/ssd1306.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/ssd1306_fonts.c

    # Metrology (E1) Sources
    ${CMAKE_CURRENT_SOURCE_DIR}/../E1/Metrology/Metrology.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../E1/Mock/Mock.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../E1/SPI/SPI.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../E1/Adapter/metrology_adapter.c
)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    # Add user defined libraries
    m
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Validate that STM32CubeMX code is compatible with C standard
if((CMAKE_C_STANDARD EQUAL 90) OR (CMAKE_C_STANDARD EQUAL 99))
    message(ERROR "Generated code requires C11 or higher")
endif()

# Generate physical Flash HEX file for Non-Secure World
add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${CMAKE_PROJECT_NAME}> ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}.hex
    COMMENT "Generating physical flash HEX file for Non-Secure World (${CMAKE_PROJECT_NAME}.hex)"
)









