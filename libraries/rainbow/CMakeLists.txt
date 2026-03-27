cmake_minimum_required(VERSION 3.25)
project(rainbow)

find_package(Qt6Core REQUIRED QUIET)
find_package(Qt6Gui REQUIRED QUIET)

set(RAINBOW_SOURCES
src/rainbow.cpp
)

add_definitions(-DRAINBOW_LIBRARY)
add_library(MeshMC_rainbow SHARED ${RAINBOW_SOURCES})
target_include_directories(MeshMC_rainbow PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

target_link_libraries(MeshMC_rainbow Qt6::Core Qt6::Gui)

# Install it
install(
    TARGETS MeshMC_rainbow
    RUNTIME DESTINATION ${LIBRARY_DEST_DIR}
    LIBRARY DESTINATION ${LIBRARY_DEST_DIR}
)
