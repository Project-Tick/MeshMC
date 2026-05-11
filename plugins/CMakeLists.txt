# MeshMC In-Tree Plugin Registry
#
# Each subdirectory here builds a .mmco shared module.
# Built modules are placed in ${MESHMC_PLUGIN_STAGING_DIR} so the
# launcher can discover them at runtime from the app-relative
# mmcmodules/ directory.

set(MESHMC_PLUGIN_STAGING_DIR "${CMAKE_BINARY_DIR}/mmcmodules" CACHE PATH
    "Directory where built .mmco plugins are placed for runtime discovery")

file(MAKE_DIRECTORY "${MESHMC_PLUGIN_STAGING_DIR}")

# Bring in the signing helper so per-plugin CMakeLists.txt may opt in to
# mmco_sign_plugin() explicitly, and so we can sign every plugin in one
# shot when MeshMC_PLUGIN_SIGN_ALL is ON.
include(MMCOSign)

####### Staging plugins
if(MeshMC_STAGING_PLUGINS)
    add_subdirectory(staging)
endif()

#################################### Register plugins here ####################################

# Track every in-tree plugin target name so we can drive automatic
# signing at the end of this listfile.
set(MESHMC_IN_TREE_PLUGIN_TARGETS
    BackupSystem
    NewsViewer
)

add_subdirectory(BackupSystem)
add_subdirectory(NewsViewer)

if(UNIX AND NOT APPLE)
    add_subdirectory(NVIDIAPrime)
    add_subdirectory(LinuxPerf)
    list(APPEND MESHMC_IN_TREE_PLUGIN_TARGETS NVIDIAPrime LinuxPerf)
endif()

if(NOT APPLE)
    add_subdirectory(Filelink)
    list(APPEND MESHMC_IN_TREE_PLUGIN_TARGETS Filelink)
endif()

# Automatic signing pass — runs only when both a key is configured and
# the user explicitly opted in to "sign every in-tree plugin".
if(MeshMC_PLUGIN_SIGN_ALL AND MeshMC_PLUGIN_SIGNING_KEY)
    foreach(_plugin IN LISTS MESHMC_IN_TREE_PLUGIN_TARGETS)
        if(TARGET ${_plugin})
            mmco_sign_plugin(${_plugin})
        endif()
    endforeach()
endif()
