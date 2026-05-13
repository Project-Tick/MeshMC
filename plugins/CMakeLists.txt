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
    SystemTray
    DesktopNotifier
    DiscordRPC
    SkinManager
)

add_subdirectory(BackupSystem)
add_subdirectory(NewsViewer)
add_subdirectory(SystemTray)
add_subdirectory(DesktopNotifier)
add_subdirectory(DiscordRPC)
add_subdirectory(SkinManager)

if(UNIX AND NOT APPLE)
    add_subdirectory(NVIDIAPrime)
    add_subdirectory(LinuxPerf)
    list(APPEND MESHMC_IN_TREE_PLUGIN_TARGETS NVIDIAPrime LinuxPerf)
endif()

if(NOT APPLE)
    add_subdirectory(Filelink)
    list(APPEND MESHMC_IN_TREE_PLUGIN_TARGETS Filelink)
endif()

# Automatic signing is wired *inside each plugin's own CMakeLists.txt*
# at the end of its target definition.
#
# Why not loop here? CMake (3.27+) tightened the rules around
# add_custom_command(TARGET <name> POST_BUILD …): the call must come
# from the *same CMakeLists.txt that created the target*. Calling it
# from this aggregator file works on older CMakes but breaks the
# macOS toolchain (Xcode 26 ships a strict CMake). The per-plugin
# pattern is portable and avoids the policy change entirely.
#
# The list MESHMC_IN_TREE_PLUGIN_TARGETS is kept above for tooling /
# documentation that wants to know which targets exist, but no longer
# drives signing.
