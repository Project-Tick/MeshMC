# MeshMC standalone plugin build bootstrap.
# When this plugin is built from MeshMC's main tree, the parent project
# already provides Qt, MeshMC::SDK, output directories, and signing helpers.
# When this directory is configured as its own repository, provide those
# pieces here so the plugin can be built with only an installed MeshMC SDK.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    cmake_minimum_required(VERSION 3.21)
    project(NewsViewer LANGUAGES CXX)

    set(CMAKE_AUTOMOC ON)
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)

    find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui Network)
    find_package(MeshMC_SDK REQUIRED)
    find_package(cmark REQUIRED)

    set(MESHMC_PLUGIN_STAGING_DIR "${CMAKE_BINARY_DIR}/mmcmodules" CACHE PATH
        "Directory where built .mmco plugins are placed")
    set(MMCO_MODULES_DEST_DIR "lib/mmcmodules" CACHE PATH
        "Install directory for .mmco plugins")
    set(MMCO_PLUGIN_DATA_DEST_DIR "share/meshmc/mmcmodules" CACHE PATH
        "Install directory for plugin data files")
endif()

# NewsViewer — MeshMC plugin (.mmco)
# Displays a news viewer dialog with multi-feed RSS support.
# Content is rendered via cmark (CommonMark) for Markdown fallback.

set(NEWSVIEWER_SOURCES
    NewsViewerPlugin.cpp
    NewsViewerDialog.h
    NewsViewerDialog.cpp
)

add_library(NewsViewer MODULE ${NEWSVIEWER_SOURCES})

target_link_libraries(NewsViewer PRIVATE
    MeshMC::SDK         # SDK: Qt + MeshMC includes
    cmark::cmark         # CommonMark parser
)

target_include_directories(NewsViewer PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
)

# Output as .mmco into the staging directory
set_target_properties(NewsViewer PROPERTIES
    PREFIX ""
    SUFFIX ".mmco"
    LIBRARY_OUTPUT_DIRECTORY "${MESHMC_PLUGIN_STAGING_DIR}"
)

foreach(CFG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER "${CFG}" CFG_UPPER)
    set_target_properties(NewsViewer PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY_${CFG_UPPER} "${MESHMC_PLUGIN_STAGING_DIR}"
    )
endforeach()

install(TARGETS NewsViewer
    LIBRARY DESTINATION "${MMCO_MODULES_DEST_DIR}"
)

# Opt-in GPG signing — see cmake/MMCOSign.cmake. Called from this
# CMakeLists so the POST_BUILD command attaches to the target in the
# directory that created it (CMake 3.27+ enforces this).
if(MeshMC_PLUGIN_SIGN_ALL)
    mmco_sign_plugin(NewsViewer)
endif()
