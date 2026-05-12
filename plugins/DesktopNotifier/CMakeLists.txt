# DesktopNotifier — MeshMC plugin (.mmco)
#
# Subscribes to lifecycle hooks and surfaces them as native desktop
# notifications using QSystemTrayIcon::showMessage() through the MMCO
# S19 tray API. Works without the SystemTray plugin: a transient hidden
# tray icon is used when the plugin doesn't own a permanent one.

set(DESKTOPNOTIFIER_SOURCES
    DesktopNotifierPlugin.cpp
)

add_library(DesktopNotifier MODULE ${DESKTOPNOTIFIER_SOURCES})

target_link_libraries(DesktopNotifier PRIVATE
    MeshMC_logic
    MeshMC::SDK
)

set_target_properties(DesktopNotifier PROPERTIES
    PREFIX ""
    SUFFIX ".mmco"
    LIBRARY_OUTPUT_DIRECTORY "${MESHMC_PLUGIN_STAGING_DIR}"
)

foreach(CFG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER "${CFG}" CFG_UPPER)
    set_target_properties(DesktopNotifier PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY_${CFG_UPPER} "${MESHMC_PLUGIN_STAGING_DIR}"
    )
endforeach()

install(TARGETS DesktopNotifier
    LIBRARY DESTINATION "${BINARY_DEST_DIR}/mmcmodules"
)
