# SPDX-FileCopyrightText: 2026 Project Tick
# SPDX-FileContributor: Project Tick
# SPDX-License-Identifier: Apache-2.0

######## QML user interface ########

# One CMakeLists per QML module, living in the directory that holds that
# module's .qml files. qt_add_qml_module places each file at
#   ${RESOURCE_PREFIX}/${URI as path}/${path relative to CMAKE_CURRENT_SOURCE_DIR}
# so a module declared from launcher/ with QML_FILES qml/Kirigami/Main.qml
# would land at qrc:/qt/qml/MeshMC/Kirigami/qml/Kirigami/Main.qml -- the source
# tree leaking into the import path. Declared from launcher/qml/Kirigami/
# instead, MeshMC.Kirigami's alias stays a bare Main.qml: the directory layout
# mirrors the URI.
#
# RESOURCE_PREFIX is spelled out rather than relying on qt_policy(QTP0001),
# which is Qt 6.5+; the floor here is 6.4 (Debian 12, Ubuntu 24.04 LTS). Setting
# it by hand gives the same qrc:/qt/qml/<URI>/ layout on every supported Qt, so
# these URLs do not move when the floor is eventually raised.

# The KDE interface QmlShell loads (see Kirigami/CMakeLists.txt).
add_subdirectory(Kirigami)

# The 3D skin preview, only when Qt Quick3D was found (see the top-level
# CMakeLists.txt's find_package(... OPTIONAL_COMPONENTS Quick3D) and the
# MeshMC_ENABLE_SKIN_PREVIEW_3D option next to it). Kept out of the
# unconditional list above so a Quick3D-less build never even sees
# Preview3D/'s CMakeLists.txt.
if(MeshMC_ENABLE_SKIN_PREVIEW_3D)
    add_subdirectory(Preview3D)
endif()

# The C++ that drives the QML: the engine owner and whatever it hands over. A
# plain library rather than a QML module of its own: it registers no QML types,
# and the QML it loads is MeshMC.Kirigami's.
qt_add_library(MeshMC_qml STATIC
    QmlShell.h
    QmlShell.cpp
    QmlUiHost.h
    QmlUiHost.cpp
    InstanceIconProvider.h
    InstanceIconProvider.cpp
    AccountFaceProvider.h
    AccountFaceProvider.cpp
    ScreenshotThumbnailProvider.h
    ScreenshotThumbnailProvider.cpp
    ColorSchemes.h
    ColorSchemes.cpp
    ModIconProvider.h
    ModIconProvider.cpp
)

# The QML side reads the core's models. It does not see QtWidgets: MeshMC_core
# cannot link them, and nothing here asks for them.
target_link_libraries(MeshMC_qml PUBLIC
    MeshMC_qml_kirigami
    MeshMC_core
)

# QmlShell's KDE application setup: the KAboutData the About page reads, the
# Breeze icons it asks for by name (BreezeIcons::initIcons()), the colour
# scheme off Plasma (ColorSchemes) and Plasma's platform plugin on a Plasma
# session (KirigamiPlatform).
target_link_libraries(MeshMC_qml PRIVATE
    KF${QT_VERSION_MAJOR}::CoreAddons
    KF${QT_VERSION_MAJOR}::BreezeIcons
    KF${QT_VERSION_MAJOR}::ColorScheme
    KF${QT_VERSION_MAJOR}::KirigamiPlatform
)

# The plugin targets, and not just the modules' backing libraries, have to reach
# the executable: a module whose plugin is missing fails at load time with
# 'plugin "..." not found' -- after a perfectly green build. Linked here, by
# name and PUBLIC, so they propagate to whatever links MeshMC_qml (the launcher
# and its tests) on every platform, without depending on qt_import_qml_plugins,
# which quietly does nothing where qmlimportscanner is not a target or libraries
# are built shared (a distribution's own Qt on Linux).
target_link_libraries(MeshMC_qml PUBLIC
    MeshMC_qml_kirigamiplugin
    Qt${QT_VERSION_MAJOR}::Quick
    Qt${QT_VERSION_MAJOR}::QuickControls2
    KF${QT_VERSION_MAJOR}::I18nQml
)

# The 3D skin preview's module and plugin, linked the same way, plus the
# compile-time flag QmlShell.cpp reads to tell QML whether
# PlayerPreview.qml has a 3D model to load at all -- see
# QmlShell::rootProperties(). Both stay entirely out of a build with
# MeshMC_ENABLE_SKIN_PREVIEW_3D off.
if(MeshMC_ENABLE_SKIN_PREVIEW_3D)
    target_link_libraries(MeshMC_qml PUBLIC
        MeshMC_qml_preview3d
        MeshMC_qml_preview3dplugin
    )
    target_compile_definitions(MeshMC_qml PRIVATE MESHMC_HAS_SKIN_PREVIEW_3D=1)
endif()
