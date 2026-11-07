######## Configure the file with build properties ########

# Where the build ran. Only BuildConfig reports these, so they are set here and
# not in the top-level CMakeLists.txt.
set(MeshMC_COMPILER_NAME ${CMAKE_CXX_COMPILER_ID})
set(MeshMC_COMPILER_VERSION ${CMAKE_CXX_COMPILER_VERSION})
set(MeshMC_COMPILER_TARGET_SYSTEM ${CMAKE_SYSTEM_NAME})
set(MeshMC_COMPILER_TARGET_SYSTEM_VERSION ${CMAKE_SYSTEM_VERSION})
set(MeshMC_COMPILER_TARGET_PROCESSOR ${CMAKE_SYSTEM_PROCESSOR})

# @ONLY: the template is C++ and must not have its ${...} expanded.
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/BuildConfig.cpp.in" "${CMAKE_CURRENT_BINARY_DIR}/BuildConfig.cpp" @ONLY)

add_library(BuildConfig STATIC
    BuildConfig.h
    ${CMAKE_CURRENT_BINARY_DIR}/BuildConfig.cpp
)

# QString is part of BuildConfig.h, so Qt::Core is a public dependency.
target_link_libraries(BuildConfig PUBLIC Qt6::Core)
target_include_directories(BuildConfig PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
