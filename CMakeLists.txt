cmake_minimum_required(VERSION 2.8)

enable_testing()

find_package(Threads REQUIRED)

set(CMAKE_CXX_STANDARD 14)

add_library(libsys-microbenchmark

  sys-microbenchmark.cpp
  cpuset.cpp

  idiv.cpp
  syscall.cpp
  memory.cpp)

add_executable(sys-microbenchmark
  main.cpp
  )

target_link_libraries(libsys-microbenchmark Threads::Threads)

target_compile_options(libsys-microbenchmark PRIVATE -Wall)
target_link_libraries(sys-microbenchmark libsys-microbenchmark)


add_executable(timer-test
  timer-test.cpp)
target_link_libraries(timer-test libsys-microbenchmark)


add_test(NAME timer COMMAND $<TARGET_FILE:timer-test>)
