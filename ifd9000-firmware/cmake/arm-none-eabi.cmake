set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR ARM)

set(CMAKE_C_COMPILER "C:/Program Files/Arm/GNU Toolchain mingw-w64-x86_64-arm-none-eabi/bin/arm-none-eabi-gcc.exe")
set(CMAKE_CXX_COMPILER "C:/Program Files/Arm/GNU Toolchain mingw-w64-x86_64-arm-none-eabi/bin/arm-none-eabi-g++.exe")
set(CMAKE_ASM_COMPILER "C:/Program Files/Arm/GNU Toolchain mingw-w64-x86_64-arm-none-eabi/bin/arm-none-eabi-gcc.exe")
set(CMAKE_OBJCOPY "C:/Program Files/Arm/GNU Toolchain mingw-w64-x86_64-arm-none-eabi/bin/arm-none-eabi-objcopy.exe")
set(CMAKE_SIZE "C:/Program Files/Arm/GNU Toolchain mingw-w64-x86_64-arm-none-eabi/bin/arm-none-eabi-size.exe")

set(CMAKE_C_FLAGS_INIT "-mcpu=cortex-m0plus -mthumb")
set(CMAKE_EXE_LINKER_FLAGS_INIT "")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)