include_directories(..)

add_executable(showresult
  showresult.cpp)

target_link_libraries(showresult libsys-microbenchmark)
