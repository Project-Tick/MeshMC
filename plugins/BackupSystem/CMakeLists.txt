if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    cmake_minimum_required(VERSION 3.21)
    project(BackupSystem LANGUAGES CXX)

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

# BackupSystem — MeshMC plugin (.mmco)
# Creates instance backups (snapshots), restores, exports, imports.
#
# All Qt/MeshMC types are obtained through the SDK header (mmco_sdk.h).
# Plugin source files do NOT directly #include Qt or MeshMC headers.

set(BACKUP_SOURCES
    BackupPlugin.cpp
    BackupManager.h
    BackupManager.cpp
    BackupPage.h
    BackupPage.cpp
)

set(BACKUP_UIS
    BackupPage.ui
)

qt_wrap_ui(BACKUP_UI_HEADERS ${BACKUP_UIS})

add_library(BackupSystem MODULE ${BACKUP_SOURCES} ${BACKUP_UI_HEADERS})

target_link_libraries(BackupSystem PRIVATE
    MeshMC::SDK             # SDK: provides all include paths + Qt deps
)

target_include_directories(BackupSystem PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}   # for generated ui headers
)

# Output as .mmco into the staging directory
set_target_properties(BackupSystem PROPERTIES
    PREFIX ""
    SUFFIX ".mmco"
    LIBRARY_OUTPUT_DIRECTORY "${MESHMC_PLUGIN_STAGING_DIR}"
)

# Also copy to build dir on multi-config generators
foreach(CFG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER "${CFG}" CFG_UPPER)
    set_target_properties(BackupSystem PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY_${CFG_UPPER} "${MESHMC_PLUGIN_STAGING_DIR}"
    )
endforeach()

# Install the .mmco alongside the binary (bin/mmcmodules/)
install(TARGETS BackupSystem
    LIBRARY DESTINATION "${MMCO_MODULES_DEST_DIR}"
)

# Opt-in GPG signing — no-op unless MeshMC_PLUGIN_SIGN_ALL is ON and
# MeshMC_PLUGIN_SIGNING_KEY is set (see cmake/MMCOSign.cmake). The
# call must happen in *this* CMakeLists.txt (the one that owns the
# target) — modern CMake forbids add_custom_command(TARGET …) from
# a different directory.
if(MeshMC_PLUGIN_SIGN_ALL)
    mmco_sign_plugin(BackupSystem)
endif()
