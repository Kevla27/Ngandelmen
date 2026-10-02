/**
 * @file FreeRTOSConfig.h
 * @brief Konfigurasi Kernel FreeRTOS untuk STM32U575 / STM32U585 (Arm Cortex-M33 @ 160 MHz)
 * @details Disesuaikan dengan Laporan Rekayasa Master Smart Meter 3-Fasa (E3/ENG-3).
 */

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#include <stdint.h>

#if defined(__ICCARM__) || defined(__ARMCC_VERSION) || defined(__GNUC__)
extern uint32_t SystemCoreClock;
#endif

#ifndef CMSIS_device_header
#define CMSIS_device_header "stm32u5xx.h"
#endif


/*-------------------- STM32U5 Cortex-M33 Specific Settings -------------------*/
#define configENABLE_TRUSTZONE                   0
#define configRUN_FREERTOS_SECURE_ONLY           0
#define configENABLE_FPU                         1
#define configENABLE_MPU                         0
#define configCHECK_HANDLER_INSTALLATION         0

#define configUSE_PREEMPTION                     1
#define configUSE_TIME_SLICING                   1
#define configSUPPORT_STATIC_ALLOCATION          1
#define configSUPPORT_DYNAMIC_ALLOCATION         1
#define configUSE_IDLE_HOOK                      0
#define configUSE_TICK_HOOK                      1
#define configCPU_CLOCK_HZ                       ( SystemCoreClock )
#define configTICK_RATE_HZ                       ( ( TickType_t ) 1000 ) /* 1 ms tick */
#define configMAX_PRIORITIES                     ( 7 )
#define configMINIMAL_STACK_SIZE                 ( ( uint16_t ) 128 )
#define configMINIMAL_STACK_DEPTH                configMINIMAL_STACK_SIZE
#define configTOTAL_HEAP_SIZE                    ( ( size_t ) ( 64 * 1024 ) ) /* 64 KB Heap */
#define configMAX_TASK_NAME_LEN                  ( 16 )
#define configUSE_TRACE_FACILITY                 1
#define configUSE_16_BIT_TICKS                   0
#define configUSE_MUTEXES                        1
#define configQUEUE_REGISTRY_SIZE                8
#define configUSE_RECURSIVE_MUTEXES              1
#define configUSE_COUNTING_SEMAPHORES            1
#define configUSE_PORT_OPTIMISED_TASK_SELECTION  0
#define configUSE_TASK_NOTIFICATIONS             1
#define configUSE_EVENT_GROUPS                   1
#define configUSE_STREAM_BUFFERS                 1

/* --- Cortex-M33 NVIC Priority Settings --- */
#ifdef __NVIC_PRIO_BITS
 #define configPRIO_BITS                         __NVIC_PRIO_BITS
#else
 #define configPRIO_BITS                         4
#endif

#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY  15
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 5

#define configKERNEL_INTERRUPT_PRIORITY          ( configLIBRARY_LOWEST_INTERRUPT_PRIORITY << ( 8 - configPRIO_BITS ) )
#define configMAX_SYSCALL_INTERRUPT_PRIORITY     ( configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << ( 8 - configPRIO_BITS ) )

#define configASSERT( x ) if ((x) == 0) { taskDISABLE_INTERRUPTS(); for( ;; ); }

/* --- Software Timer Definitions --- */
#define configUSE_TIMERS                         1
#define configTIMER_TASK_PRIORITY                ( 6 )
#define configTIMER_QUEUE_LENGTH                 10
#define configTIMER_TASK_STACK_DEPTH             128

/* --- FreeRTOS API Includes --- */
#define INCLUDE_vTaskPrioritySet                 1
#define INCLUDE_uxTaskPriorityGet                1
#define INCLUDE_vTaskDelete                      1
#define INCLUDE_vTaskCleanUpResources            0
#define INCLUDE_vTaskSuspend                     1
#define INCLUDE_vTaskDelayUntil                  1
#define INCLUDE_vTaskDelay                       1
#define INCLUDE_xTaskGetSchedulerState           1
#define INCLUDE_xTimerPendFunctionCall           1
#define INCLUDE_xQueueGetMutexHolder             1
#define INCLUDE_xSemaphoreGetMutexHolder         1
#define INCLUDE_uxTaskGetStackHighWaterMark      1
#define INCLUDE_xTaskGetCurrentTaskHandle        1
#define INCLUDE_eTaskGetState                    1
#define INCLUDE_xTaskAbortDelay                  1

/* --- Pemetaan Prioritas Task Sub-sistem 3 (Host MCU) --- */
#define TASK_PRIO_METROLOGY_READER               ( configMAX_PRIORITIES - 1 ) /* Prioritas 6 (Tinggi): ADE9078 SPI 1s */
#define TASK_PRIO_TAMPER_UI                      ( configMAX_PRIORITIES - 2 ) /* Prioritas 5 (Sedang-Tinggi): LCD & EXTI IRQ */
#define TASK_PRIO_ACTUATION                      ( configMAX_PRIORITIES - 2 ) /* Prioritas 5 (Sedang-Tinggi): Pulsa Relai */
#define TASK_PRIO_DLMS_COMM                      ( configMAX_PRIORITIES - 3 ) /* Prioritas 4 (Normal): Engine DLMS/COSEM */
#define TASK_PRIO_PROFILING_LOGGER               ( configMAX_PRIORITIES - 4 ) /* Prioritas 3 (Rendah): Load Profile Flash 2MB */

#endif /* FREERTOS_CONFIG_H */