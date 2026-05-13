# SystemTray — MeshMC plugin (.mmco)
#
# Adds a QSystemTrayIcon for MeshMC with a right-click menu (Show / Hide /
# Quit / quick-launch instances) and an optional "minimize to tray" close
# filter on the main window.
#
# All behaviour goes through the MMCO S19/S20 API; this plugin does NOT
# touch core widgets directly.

set(SYSTEMTRAY_SOURCES
    SystemTrayPlugin.cpp
)

add_library(SystemTray MODULE ${SYSTEMTRAY_SOURCES})

target_link_libraries(SystemTray PRIVATE
    MeshMC_logic
    MeshMC::SDK
)

set_target_properties(SystemTray PROPERTIES
    PREFIX ""
    SUFFIX ".mmco"
    LIBRARY_OUTPUT_DIRECTORY "${MESHMC_PLUGIN_STAGING_DIR}"
)

foreach(CFG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER "${CFG}" CFG_UPPER)
    set_target_properties(SystemTray PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY_${CFG_UPPER} "${MESHMC_PLUGIN_STAGING_DIR}"
    )
endforeach()

install(TARGETS SystemTray
    LIBRARY DESTINATION "${BINARY_DEST_DIR}/mmcmodules"
)

if(MeshMC_PLUGIN_SIGN_ALL)
    mmco_sign_plugin(SystemTray)
endif()
