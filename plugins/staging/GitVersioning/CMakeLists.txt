# SPDX-FileCopyrightText: 2026 Project Tick
# SPDX-License-Identifier: GPL-3.0-or-later
#
# GitVersioning — MeshMC plugin (.mmco)
# Snapshots instance content as Git commits so users can roll back to
# any previous state, see a per-file history, and share patches.

set(GITVER_SOURCES
    GitVersioningPlugin.cpp
    GitRepo.h
    GitRepo.cpp
    GitVersioningPage.h
    GitVersioningPage.cpp
)

add_library(GitVersioning MODULE ${GITVER_SOURCES})

target_link_libraries(GitVersioning PRIVATE
    MeshMC_logic
    MeshMC::SDK
)

target_include_directories(GitVersioning PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
)

set_target_properties(GitVersioning PROPERTIES
    PREFIX ""
    SUFFIX ".mmco"
    LIBRARY_OUTPUT_DIRECTORY "${MESHMC_PLUGIN_STAGING_DIR}"
)

foreach(CFG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER "${CFG}" CFG_UPPER)
    set_target_properties(GitVersioning PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY_${CFG_UPPER} "${MESHMC_PLUGIN_STAGING_DIR}"
    )
endforeach()

install(TARGETS GitVersioning
    LIBRARY DESTINATION "${BINARY_DEST_DIR}/mmcmodules"
)

if(MeshMC_PLUGIN_SIGN_ALL)
    mmco_sign_plugin(GitVersioning)
endif()
