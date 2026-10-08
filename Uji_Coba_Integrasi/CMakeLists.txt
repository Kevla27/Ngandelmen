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

# Set the project name
set(CMAKE_PROJECT_NAME Uji_Coba_Integrasi)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
    
   
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user sources here
    # FreeRTOS Kernel Sources
    firmware/FreeRTOS/tasks.c
    firmware/FreeRTOS/queue.c
    firmware/FreeRTOS/list.c
    firmware/FreeRTOS/timers.c
    firmware/FreeRTOS/event_groups.c
    firmware/FreeRTOS/stream_buffer.c
    firmware/FreeRTOS/portable/MemMang/heap_4.c
    firmware/FreeRTOS/portable/GCC/ARM_CM33_NTZ/non_secure/port.c
    firmware/FreeRTOS/portable/GCC/ARM_CM33_NTZ/non_secure/portasm.c
    firmware/sys/src/freertos_hooks.c

    # Application (E3) Sources
    firmware/app/display/src/display.c
    firmware/app/display/src/display_task.c
    firmware/app/tamper/src/tamper_manager.c
    firmware/app/fsm/src/fsm_statechart.c
    firmware/app/fsm/src/rtos_tasks.c
    firmware/app/profiling/src/load_profile.c
    firmware/app/profiling/src/load_profile_dlms_adapter.c
    firmware/app/profiling/src/nvram_storage.c
    firmware/app/console/src/console_io.c

    # Protocol (E2) & BSP Sources
    firmware/protocols/dlms/src/dlms_association.c
    firmware/protocols/dlms/src/dlms_transport.c
    firmware/protocols/dlms/src/dlms_apdu.c
    firmware/protocols/dlms/src/dlms_obis.c
    firmware/protocols/dlms/src/dlms_tamper_log.c
    firmware/protocols/dlms/src/dlms_hdlc.c
    firmware/protocols/dlms/src/dlms_server.c
    firmware/protocols/dlms/src/dlms_task.c
    firmware/protocols/dlms/src/dlms_nvm.c
    firmware/drivers/bsp/src/bsp_lpuart_dma.c
    firmware/drivers/bsp/src/bsp_display_oled.c
    Core/Src/ssd1306.c
    Core/Src/ssd1306_fonts.c

    # Metrology (E1) Sources
    E1/Metrology/Metrology.c
    E1/Mock/Mock.c
    E1/SPI/SPI.c
    E1/Adapter/metrology_adapter.c
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined include paths
    firmware/sys/inc
    firmware/FreeRTOS/include
    firmware/FreeRTOS/portable/GCC/ARM_CM33_NTZ/non_secure
    firmware/app/display/inc
    firmware/app/tamper/inc
    firmware/app/fsm/inc
    firmware/app/profiling/inc
    firmware/app/console/inc
    firmware/protocols/dlms/inc
    firmware/drivers/bsp/inc
    Core/Inc
    E1/Metrology
    E1/Mock
    E1/RegisterMap
    E1/SPI
    E1/Adapter
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
    USE_FREERTOS
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
)

add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${CMAKE_PROJECT_NAME}> ${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${CMAKE_PROJECT_NAME}> ${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}.bin
    COMMENT "Generating .hex and .bin firmware images..."
)

