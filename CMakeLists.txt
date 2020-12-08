cmake_minimum_required(VERSION 2.8)

find_package(Threads REQUIRED)

set(CMAKE_CXX_STANDARD 14)

add_executable(sys-microbenchmark
  main.cpp
  
  sys-microbenchmark.cpp
  idiv.cpp
  syscall.cpp
  memory.cpp
  )

target_compile_options(sys-microbenchmark PRIVATE -Wall)
target_link_libraries(sys-microbenchmark Threads::Threads)
