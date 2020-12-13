cmake_minimum_required(VERSION 3.1)

project(sys-microbenchmark)

option(MINGW_CROSS "" OFF)

if (MINGW_CROSS)
  set(MINGW_TOOLCHAIN_PREFIX x86_64-w64-mingw32- CACHE STRING "")

  set(CMAKE_SYSTEM_NAME Windows)
  set(CMAKE_CXX_COMPILER ${MINGW_TOOLCHAIN_PREFIX}g++)
  set(CMAKE_C_COMPILER ${MINGW_TOOLCHAIN_PREFIX}g++)
else()
  enable_testing()
endif()

find_package(Threads REQUIRED)

add_library(libsys-microbenchmark

  sys-microbenchmark.cpp
  cpuset.cpp
  cpu-feature.cpp

  idiv.cpp
  syscall.cpp
  memory-latency.cpp
  memory-bandwidth.cpp

  x86-avx.cpp
  x86-avx512.cpp
  x86.cpp

  aarch64.cpp

  unknown-arch.cpp
)

set_source_files_properties(x86-avx.cpp PROPERTIES COMPILE_FLAGS -mavx)
set_source_files_properties(x86-avx512.cpp PROPERTIES COMPILE_FLAGS -mavx512f)

add_executable(sys-microbenchmark
  main.cpp
  )

target_link_libraries(libsys-microbenchmark Threads::Threads)

target_compile_options(libsys-microbenchmark PUBLIC -Wall -std=gnu++17)
target_link_libraries(sys-microbenchmark libsys-microbenchmark)


add_executable(timer-test
  timer-test.cpp)
target_link_libraries(timer-test libsys-microbenchmark)


add_test(NAME timer COMMAND $<TARGET_FILE:timer-test>)
