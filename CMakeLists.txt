cmake_minimum_required(VERSION 3.1)

project(sys-microbenchmark)

set(MINGW_CROSS OFF CACHE BOOL "")

if (MINGW_CROSS)
  set(MINGW_TOOLCHAIN_PREFIX x86_64-w64-mingw32 CACHE STRING "")

  set(CMAKE_SYSTEM_NAME Windows)
  set(CMAKE_CXX_COMPILER ${MINGW_TOOLCHAIN_PREFIX}-g++ CACHE STRING "" FORCE)
  set(CMAKE_C_COMPILER ${MINGW_TOOLCHAIN_PREFIX}-gcc CACHE STRING "" FORCE)

  set(BUILD_HWLOC_CONF_ARGS CC=${CMAKE_C_COMPILER} --host=${MINGW_TOOLCHAIN_PREFIX})

  set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")
  set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")
  set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")
else()
  enable_testing()
endif()


#find_package(PkgConfig)
#pkg_check_modules(HWLOC REQUIRED hwloc)

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

set(HWLOC_INCLUDE_DIRS ${CMAKE_CURRENT_BINARY_DIR}/hwloc-build/include ${CMAKE_CURRENT_BINARY_DIR}/hwloc-2.4.0/include)

set(HWLOC_LIB_DIR ${CMAKE_CURRENT_BINARY_DIR}/hwloc-build/hwloc/.libs)

add_custom_command(OUTPUT ${HWLOC_LIB_DIR}/libhwloc_embedded.a
  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/build-hwloc.sh ${BUILD_HWLOC_CONF_ARGS})

add_custom_target(hwloc
  SOURCES ${HWLOC_LIB_DIR}/libhwloc_embedded.a
  )

target_include_directories(libsys-microbenchmark SYSTEM PUBLIC ${HWLOC_INCLUDE_DIRS})
link_directories(${HWLOC_LIB_DIR})

if (${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64")
  set_source_files_properties(x86-avx.cpp PROPERTIES COMPILE_FLAGS -mavx)
  set_source_files_properties(x86-avx512.cpp PROPERTIES COMPILE_FLAGS -mavx512f)
endif()

add_executable(sys-microbenchmark
  main.cpp
  )

target_link_libraries(libsys-microbenchmark
  PUBLIC Threads::Threads
  PRIVATE ${HWLOC_LIB_DIR}/libhwloc_embedded.a)

add_dependencies(libsys-microbenchmark hwloc)

target_compile_options(libsys-microbenchmark PUBLIC -Wall -std=gnu++17)
target_link_libraries(sys-microbenchmark libsys-microbenchmark)


add_executable(timer-test
  timer-test.cpp)
target_link_libraries(timer-test libsys-microbenchmark)


add_test(NAME timer COMMAND $<TARGET_FILE:timer-test>)
