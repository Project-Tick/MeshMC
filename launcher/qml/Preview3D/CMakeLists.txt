# SPDX-FileCopyrightText: 2026 Project Tick
# SPDX-FileContributor: Project Tick
# SPDX-License-Identifier: Apache-2.0

######## MeshMC.Preview3D: the optional 3D skin preview ########

# Only ever added from launcher/qml/CMakeLists.txt's
# `if(MeshMC_ENABLE_SKIN_PREVIEW_3D) add_subdirectory(Preview3D)`, itself
# gated on the top-level CMakeLists.txt's find_package(... OPTIONAL_COMPONENTS
# Quick3D) -- so by the time this file runs, Qt::Quick3D is known to exist.
# A module of its own, apart from MeshMC.Kirigami, because it is the one part
# of the interface that needs Qt Quick3D: Kirigami/PlayerPreview.qml loads it
# by url, so MeshMC.Kirigami builds and runs without it. Same layout rules as
# the other modules (see launcher/qml/CMakeLists.txt): RESOURCE_PREFIX by hand
# for the 6.4 floor, and this file beside the .qml it lists, so the model sits
# at qrc:/qt/qml/MeshMC/Preview3D/SkinPreview3D.qml.
qt_add_library(MeshMC_qml_preview3d STATIC)

qt_add_qml_module(MeshMC_qml_preview3d
    URI MeshMC.Preview3D
    VERSION 1.0
    RESOURCE_PREFIX "/qt/qml"
    # SkinPreview3D.qml imports QtQuick3D and QtQuick3D.Helpers itself;
    # listed here as well so this module's qmldir records the runtime
    # dependency for tooling that reads it (qmlimportscanner already finds
    # both from the `import` statements regardless).
    DEPENDENCIES
        QtQuick3D
        QtQuick3D.Helpers
    QML_FILES
        SkinPreview3D.qml
)

target_link_libraries(MeshMC_qml_preview3d PUBLIC
    Qt${QT_VERSION_MAJOR}::Quick
    Qt${QT_VERSION_MAJOR}::Quick3D
)
