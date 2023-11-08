cmake_minimum_required(VERSION 3.0.0)
project(MrDo VERSION 0.1.0 DESCRIPTION "MrDo!" LANGUAGES C CXX)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${MrDo_BINARY_DIR}/build_results/bin)

add_subdirectory(vendor)
add_subdirectory(mrdo)

