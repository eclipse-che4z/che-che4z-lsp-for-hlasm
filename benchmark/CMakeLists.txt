cmake_minimum_required (VERSION 3.10)

project(benchmark)

add_executable(benchmark ${PROJECT_SOURCE_DIR}/benchmark.cpp)

add_dependencies(benchmark json)

target_link_libraries(benchmark parser_library)
