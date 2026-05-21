# MeshMC standalone plugin build bootstrap.
# When this plugin is built from MeshMC's main tree, the parent project
# already provides Qt, MeshMC::SDK, output directories, and signing helpers.
# When this directory is configured as its own repository, provide those
# pieces here so the plugin can be built with only an installed MeshMC SDK.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    cmake_minimum_required(VERSION 3.21)
    project(SkinManager LANGUAGES CXX)

    set(CMAKE_AUTOMOC ON)
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)

    find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui Network OpenGL OpenGLWidgets)
    find_package(MeshMC_SDK REQUIRED)

    set(MESHMC_PLUGIN_STAGING_DIR "${CMAKE_BINARY_DIR}/mmcmodules" CACHE PATH
        "Directory where built .mmco plugins are placed")
    set(MMCO_MODULES_DEST_DIR "lib/mmcmodules" CACHE PATH
        "Install directory for .mmco plugins")
    set(MMCO_PLUGIN_DATA_DEST_DIR "share/meshmc/mmcmodules" CACHE PATH
        "Install directory for plugin data files")
endif()

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
    LIBRARY DESTINATION "${MMCO_MODULES_DEST_DIR}"
)

if(MeshMC_PLUGIN_SIGN_ALL)
    mmco_sign_plugin(SkinManager)
endif()
