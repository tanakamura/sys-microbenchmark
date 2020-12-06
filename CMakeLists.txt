cmake_minimum_required(VERSION 2.8)

add_executable(sys-microbenchmark
  main.cpp
  
  sys-microbenchmark.cpp
  idiv.cpp)

target_compile_options(sys-microbenchmark PRIVATE -Wall)
