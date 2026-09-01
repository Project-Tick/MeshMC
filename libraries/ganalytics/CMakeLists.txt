cmake_minimum_required(VERSION 3.25)
project(ganalytics VERSION 1.0.0 LANGUAGES CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(CMAKE_AUTOMOC ON)

find_package(Qt6 COMPONENTS Core Gui Network REQUIRED)

# In the MeshMC tree systeminfo is a sibling subproject, not an installed package.
if(NOT TARGET systeminfo::systeminfo)
    find_package(systeminfo REQUIRED)
endif()

set(ganalytics_SOURCES
src/ganalytics.cpp
src/ganalytics_worker.cpp
src/ganalytics_worker.h
include/ganalytics.h
)

add_library(ganalytics ${ganalytics_SOURCES})
set_target_properties(ganalytics PROPERTIES POSITION_INDEPENDENT_CODE ON)
add_library(ganalytics::ganalytics ALIAS ganalytics)
target_link_libraries(ganalytics PUBLIC Qt6::Core Qt6::Gui Qt6::Network systeminfo::systeminfo)
target_include_directories(ganalytics
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/ganalytics>)
