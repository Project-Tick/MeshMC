# SkinManager — MeshMC plugin (.mmco)
#
# 3D skin viewer + skin upload / delete / cape change UI.
#
# Adds a global-settings page (registered via MMCO_HOOK_UI_GLOBAL_SETTINGS_PAGES)
# that shows a real-time 3D model of the active MSA account's skin, lets
# the user upload / delete the skin, and switch between owned capes.
#
# Shaders come from MeshMC's own resource bundle (:/shaders/...), so
# this plugin does not ship any GLSL of its own.

set(SKINMANAGER_SOURCES
    SkinManagerPlugin.cpp
    SkinManagerDialog.h
    SkinManagerDialog.cpp
    SkinViewerWidget.h
    SkinViewerWidget.cpp
    SkinModel.h
    SkinModel.cpp
)

set(SKINMANAGER_UIS
    SkinManagerDialog.ui
)

set(SKINMANAGER_RESOURCES
    skinmanager.qrc
)

qt_wrap_ui(SKINMANAGER_UI_HEADERS ${SKINMANAGER_UIS})
qt_add_resources(SKINMANAGER_RES_SOURCES ${SKINMANAGER_RESOURCES})

add_library(SkinManager MODULE
    ${SKINMANAGER_SOURCES}
    ${SKINMANAGER_UI_HEADERS}
    ${SKINMANAGER_RES_SOURCES}
)

target_link_libraries(SkinManager PRIVATE
    MeshMC_logic                # in-tree symbols: SkinUpload, SkinDelete,
                                # CapeChange, MinecraftAccount, …
    MeshMC::SDK                 # public SDK includes + Qt deps
    Qt6::OpenGL                 # QOpenGLShaderProgram / QOpenGLBuffer
    Qt6::OpenGLWidgets          # QOpenGLWidget
)

target_include_directories(SkinManager PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}   # generated ui_*.h
)

set_target_properties(SkinManager PROPERTIES
    PREFIX ""
    SUFFIX ".mmco"
    LIBRARY_OUTPUT_DIRECTORY "${MESHMC_PLUGIN_STAGING_DIR}"
)

foreach(CFG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER "${CFG}" CFG_UPPER)
    set_target_properties(SkinManager PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY_${CFG_UPPER} "${MESHMC_PLUGIN_STAGING_DIR}"
    )
endforeach()

install(TARGETS SkinManager
    LIBRARY DESTINATION "${BINARY_DEST_DIR}/mmcmodules"
)

if(MeshMC_PLUGIN_SIGN_ALL)
    mmco_sign_plugin(SkinManager)
endif()
