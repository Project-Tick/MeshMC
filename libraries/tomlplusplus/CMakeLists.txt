cmake_minimum_required(VERSION 3.24)
project(tomlplusplus)

add_library(tomlplusplus INTERFACE)
target_include_directories(tomlplusplus INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")
