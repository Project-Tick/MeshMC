cmake_minimum_required(VERSION 3.25)
project(optional-bare VERSION 1.0.0)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

add_library(optional-bare INTERFACE)
add_library(optional-bare::optional-bare ALIAS optional-bare)
target_include_directories(optional-bare
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/optional-bare>)
