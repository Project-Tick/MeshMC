# MeshMC standalone plugin build bootstrap.
# When this plugin is built from MeshMC's main tree, the parent project
# already provides Qt, MeshMC::SDK, output directories, and signing helpers.
# When this directory is configured as its own repository, provide those
# pieces here so the plugin can be built with only an installed MeshMC SDK.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    cmake_minimum_required(VERSION 3.21)
    project(Filelink LANGUAGES CXX)

    set(CMAKE_AUTOMOC ON)
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)

    find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui Network)
    find_package(MeshMC_SDK REQUIRED)

    set(MESHMC_PLUGIN_STAGING_DIR "${CMAKE_BINARY_DIR}/mmcmodules" CACHE PATH
        "Directory where built .mmco plugins are placed")
    set(MMCO_MODULES_DEST_DIR "lib/mmcmodules" CACHE PATH
        "Install directory for .mmco plugins")
    set(MMCO_PLUGIN_DATA_DEST_DIR "share/meshmc/mmcmodules" CACHE PATH
        "Install directory for plugin data files")
endif()

# Filelink — MeshMC plugin (.mmco)
# Create desktop shortcuts (.desktop / .lnk) for instances.
#
# Supports Linux and Windows.  macOS is not supported.

set(FILELINK_SOURCES
    FilelinkPlugin.cpp
)

add_library(Filelink MODULE ${FILELINK_SOURCES})

target_link_libraries(Filelink PRIVATE
    MeshMC::SDK
)

# Output as .mmco into the staging directory
set_target_properties(Filelink PROPERTIES
    PREFIX ""
    SUFFIX ".mmco"
    LIBRARY_OUTPUT_DIRECTORY "${MESHMC_PLUGIN_STAGING_DIR}"
)

# Also copy to build dir on multi-config generators
foreach(CFG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER "${CFG}" CFG_UPPER)
    set_target_properties(Filelink PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY_${CFG_UPPER} "${MESHMC_PLUGIN_STAGING_DIR}"
    )
endforeach()

# Install the .mmco alongside the binary (bin/mmcmodules/)
install(TARGETS Filelink
    LIBRARY DESTINATION "${MMCO_MODULES_DEST_DIR}"
)

if(MeshMC_PLUGIN_SIGN_ALL)
    mmco_sign_plugin(Filelink)
endif()
