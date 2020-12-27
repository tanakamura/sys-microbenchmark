cmake_minimum_required(VERSION 3.1)

project(sys-microbenchmark)

enable_testing()

#find_package(PkgConfig)
#pkg_check_modules(HWLOC REQUIRED hwloc)

find_package(Threads)
if (Threads::Threads)
  set(THRAEDS_LIBNAME Threads::Threads)
endif()


find_package(OpenMP)
if (${OpenMP_FOUND})
  set(OMP_LIBNAME OpenMP::OpenMP_C)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
endif()


add_library(libsys-microbenchmark

  sys-microbenchmark.cpp
  cpuset.cpp
  cpu-feature.cpp
  sysinfo.cpp

  idiv.cpp
  syscall.cpp
  memory-latency.cpp
  memory-bandwidth.cpp
  omp.cpp
  actual-freq.cpp
  ipc.cpp
  branch.cpp
  instruction.cpp
  pipe.cpp
  libc.cpp
  libcxx.cpp
  fpu.cpp

  x86-avx.cpp
  x86-avx512.cpp
  x86.cpp

  aarch64.cpp

  unknown-arch.cpp

  )

if (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  set(HAVE_HWLOC ON)
  set(HAVE_CPUINFO ON)
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Android")
  set(HAVE_HWLOC ON)
  set(HAVE_CPUINFO ON)
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
  set(HAVE_HWLOC ON)
  set(HAVE_CPUINFO ON)
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
  set(HAVE_HWLOC ON)
  set(HAVE_CPUINFO ON)
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Emscripten")
  set(HAVE_HWLOC OFF)
  set(HAVE_CPUINFO ON)
else()
  set(HAVE_HWLOC OFF)
  set(HAVE_CPUINFO OFF)
endif()

if (HAVE_CPUINFO)
  OPTION(CPUINFO_BUILD_UNIT_TESTS "Build cpuinfo unit tests" OFF)
  OPTION(CPUINFO_BUILD_MOCK_TESTS "Build cpuinfo mock tests" OFF)
  OPTION(CPUINFO_BUILD_BENCHMARKS "Build cpuinfo micro-benchmarks" OFF)

  add_subdirectory(cpuinfo EXCLUDE_FROM_ALL)
  set(CPUINFO_LIBNAME cpuinfo)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DHAVE_CPUINFO")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DHAVE_CPUINFO")
  set(CPUINFO_INCLUDE_DIRS cpuinfo/include)
endif()



if (HAVE_HWLOC)
  set(HWLOC_INCLUDE_DIRS ${CMAKE_CURRENT_BINARY_DIR}/hwloc-build/include ${CMAKE_CURRENT_SOURCE_DIR}/hwloc/include)
  set(HWLOC_LIB_DIR ${CMAKE_CURRENT_BINARY_DIR}/hwloc-build/hwloc/.libs)

  add_custom_command(OUTPUT ${HWLOC_LIB_DIR}/libhwloc_embedded.a
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/build-hwloc.sh ${CMAKE_CURRENT_SOURCE_DIR}/hwloc ${BUILD_HWLOC_CONF_ARGS})

  add_custom_target(hwloc
    SOURCES ${HWLOC_LIB_DIR}/libhwloc_embedded.a
    )

  link_directories(${HWLOC_LIB_DIR})

  set(HWLOC_LIBNAME ${HWLOC_LIB_DIR}/libhwloc_embedded.a)

  add_dependencies(libsys-microbenchmark hwloc)

endif()

target_include_directories(libsys-microbenchmark SYSTEM PUBLIC ${HWLOC_INCLUDE_DIRS} ${CPUINFO_INCLUDE_DIRS})

if (${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64")
  set_source_files_properties(x86-avx.cpp PROPERTIES COMPILE_FLAGS "-mavx2 -mfma")
  set_source_files_properties(x86-avx512.cpp PROPERTIES COMPILE_FLAGS -mavx512f)
endif()

add_executable(sys-microbenchmark
  main.cpp
  )

target_link_libraries(libsys-microbenchmark
  ${THREADS_LIBNAME}
  ${OMP_LIBNAME}
  ${HWLOC_LIBNAME}
  ${CPUINFO_LIBNAME}
  )

target_compile_options(libsys-microbenchmark PUBLIC -Wall -std=c++17)
target_link_libraries(sys-microbenchmark libsys-microbenchmark)


add_executable(timer-test
  timer-test.cpp)
target_link_libraries(timer-test libsys-microbenchmark)


add_test(NAME timer COMMAND $<TARGET_FILE:timer-test>)
