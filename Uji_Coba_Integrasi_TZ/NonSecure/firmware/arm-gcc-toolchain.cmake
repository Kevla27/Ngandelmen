set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

# Mencegah CMake gagal saat pengujian compiler
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# Cross-Compiler ARM
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
set(CMAKE_OBJCOPY arm-none-eabi-objcopy)
set(CMAKE_SIZE arm-none-eabi-size)

# Flag Hardware CPU Cortex-M33 STM32U575
set(CPU_FLAGS "-mcpu=cortex-m33 -mthumb -mfpu=fpv5-sp-d16 -mfloat-abi=hard")

# Flag Kompilasi C & Assembly
set(CMAKE_C_FLAGS "${CPU_FLAGS} -Wall -Wextra" CACHE STRING "")
set(CMAKE_ASM_FLAGS "${CPU_FLAGS}" CACHE STRING "")

# Flag Linker (Taruh --specs HANYA di sini agar tidak terduplikasi)
set(CMAKE_EXE_LINKER_FLAGS "${CPU_FLAGS} --specs=nosys.specs --specs=nano.specs" CACHE STRING "")