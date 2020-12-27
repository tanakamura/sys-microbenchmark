set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

set(triple aarch64-linux-gnu)
set(CMAKE_C_COMPILER ${triple}-gcc)
set(CMAKE_C_COMPILER_TARGET ${triple})
set(CMAKE_CXX_COMPILER ${triple}-g++)
set(CMAKE_CXX_COMPILER_TARGET ${triple})

set(BUILD_HWLOC_CONF_ARGS "CC=${CMAKE_C_COMPILER} --host=${triple}")

#set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
#set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
#set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
#set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
