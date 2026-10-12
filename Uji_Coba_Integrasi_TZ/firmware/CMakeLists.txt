cmake_minimum_required(VERSION 3.20)
project(DLMS_COSEM_Firmware C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

option(BUILD_TESTS "Build Host Unit Tests" ON)

# Pastikan assert() selalu aktif pada seluruh pengujian host bahkan jika Release dipilih
string(REPLACE "-DNDEBUG" "" CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
string(REPLACE "-DNDEBUG" "" CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO}")
add_compile_options(-UNDEBUG)

# Modern CMake Target: dlms_core
add_library(dlms_core STATIC
    protocols/dlms/src/dlms_association.c
    protocols/dlms/src/dlms_transport.c
    protocols/dlms/src/dlms_apdu.c
    protocols/dlms/src/dlms_obis.c
    protocols/dlms/src/dlms_tamper_log.c
    protocols/dlms/src/dlms_hdlc.c
    protocols/dlms/src/dlms_server.c
    protocols/dlms/src/dlms_task.c
    protocols/dlms/src/dlms_nvm.c
    drivers/bsp/src/bsp_lpuart_dma.c
    drivers/bsp/src/bsp_alarm_buzzer.c
    app/display/src/display.c
    app/display/src/display_task.c
    app/tamper/src/tamper_manager.c
    app/fsm/src/fsm_statechart.c
    app/fsm/src/power_relay_manager.c
    app/fsm/src/rtos_tasks.c
    app/profiling/src/load_profile.c
    app/profiling/src/load_profile_dlms_adapter.c
    app/profiling/src/nvram_storage.c
)

target_include_directories(dlms_core PUBLIC 
    protocols/dlms/inc
    drivers/bsp/inc
    app/display/inc
    app/tamper/inc 
    app/fsm/inc
    sys/inc
    app/profiling/inc
)

# Host Unit Testing Setup
if(BUILD_TESTS)
    enable_testing()

    set(TEST_NAMES
        test_dlms_association
        test_dlms_apdu
        test_dlms_integration
        test_dlms_tamper_log
        test_dlms_hdlc
        test_dlms_server
        test_dlms_task
        test_dlms_nvm
        test_bsp_lpuart
        test_e3_app
        test_fsm_statechart
        test_load_profile
        test_load_profile_dlms_adapter
        test_nvram_storage
        test_rtos_tasks
        test_power_relay
    )

    set(TEST_SOURCES
        tests/protocols/test_dlms_association.c
        tests/protocols/test_dlms_apdu.c
        tests/protocols/test_dlms_integration.c
        tests/protocols/test_dlms_tamper_log.c
        tests/protocols/test_dlms_hdlc.c
        tests/protocols/test_dlms_server.c
        tests/protocols/test_dlms_task.c
        tests/protocols/test_dlms_nvm.c
        tests/drivers/test_bsp_lpuart.c
        tests/app/test_e3_app.c
        tests/app/test_fsm_statechart.c
        tests/app/test_load_profile.c
        tests/app/test_load_profile_dlms_adapter.c
        tests/app/test_nvram_storage.c
        tests/app/test_rtos_tasks.c
        tests/app/test_power_relay.c
    )

    # Memasangkan nama target dan source file secara aman menggunakan ZIP_LISTS
    foreach(TEST_NAME TEST_SRC IN ZIP_LISTS TEST_NAMES TEST_SOURCES)
        add_executable(${TEST_NAME} ${TEST_SRC})
        target_link_libraries(${TEST_NAME} PRIVATE dlms_core)
        add_test(NAME ${TEST_NAME} COMMAND $<TARGET_FILE:${TEST_NAME}>)
    endforeach()

    # Integrasi Test 1: Tampilan SPLN D3.006 & Alarm Sabotase
    add_executable(test_spln_display_tamper ${CMAKE_CURRENT_SOURCE_DIR}/../tests/test_spln_display_tamper.c)
    target_link_libraries(test_spln_display_tamper PRIVATE dlms_core)
    add_test(NAME test_spln_display_tamper COMMAND $<TARGET_FILE:test_spln_display_tamper>)

    # Integrasi Test 2: E1 (Metrologi ADE9000 Mock) -> E3 (Meas) -> E2 (DLMS/COSEM OBIS)
    add_executable(test_e1_to_e2_integration
        ${CMAKE_CURRENT_SOURCE_DIR}/../tests/test_e1_to_e2_integration.c
        ${CMAKE_CURRENT_SOURCE_DIR}/../E1/Metrology/Metrology.c
        ${CMAKE_CURRENT_SOURCE_DIR}/../E1/Mock/Mock.c
        ${CMAKE_CURRENT_SOURCE_DIR}/../E1/SPI/SPI.c
    )
    target_include_directories(test_e1_to_e2_integration PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../E1/Metrology
        ${CMAKE_CURRENT_SOURCE_DIR}/../E1/Mock
        ${CMAKE_CURRENT_SOURCE_DIR}/../E1/RegisterMap
        ${CMAKE_CURRENT_SOURCE_DIR}/../E1/SPI
    )
    target_link_libraries(test_e1_to_e2_integration PRIVATE dlms_core)
    add_test(NAME test_e1_to_e2_integration COMMAND $<TARGET_FILE:test_e1_to_e2_integration>)
endif()