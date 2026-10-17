# SPDX-FileCopyrightText: 2026 Project Tick
# SPDX-FileContributor: Project Tick
# SPDX-License-Identifier: Apache-2.0

######## The KDE interface (MeshMC.Kirigami) ########

# A stock Kirigami application: Kirigami.ApplicationWindow, its page stack and
# global drawer, the org.kde.desktop controls style, Breeze icons by name,
# Kirigami.Theme colours and Kirigami.Units sizes. It imports nothing from the
# custom design (MeshMC.Theme, MeshMC.Style, MeshMC.Components): those modules
# stay compiled beside it until every page has been ported, then go.
#
# Same layout rules as the other modules (see launcher/qml/CMakeLists.txt):
# RESOURCE_PREFIX by hand for the 6.4 floor, and this file beside the .qml it
# lists, so the root window sits at qrc:/qt/qml/MeshMC/Kirigami/Main.qml.
qt_add_library(MeshMC_qml_kirigami STATIC)

qt_add_qml_module(MeshMC_qml_kirigami
    URI MeshMC.Kirigami
    VERSION 1.0
    RESOURCE_PREFIX "/qt/qml"
    QML_FILES
        Main.qml
        DevRoute.qml
        Navigator.qml
        AppDrawer.qml
        PlaceholderPage.qml
        LibraryPage.qml
        InstanceGroup.qml
        InstanceCard.qml
        InstanceRow.qml
        InstanceSummary.qml
        InstanceActions.qml
        InstanceDialogs.qml
        IconPickerDialog.qml
        DuplicateDialog.qml
        ShortcutDialog.qml
        InstancePage.qml
        LaunchFooter.qml
        LaunchProgressBar.qml
        ContentTab.qml
        ContentRow.qml
        LogTab.qml
        LogView.qml
        WorldsTab.qml
        ScreenshotsTab.qml
        UiRequests.qml
        UiRequestBody.qml
        Clipboard.qml
        ListArea.qml
        format.js
        RemoteRichText.qml
        DiscoverPage.qml
        ModpackRow.qml
        ModpackFilterDialog.qml
        ModpackPage.qml
        GalleryStrip.qml
        GalleryViewer.qml
        ContentInstaller.qml
        AddContentPage.qml
        ContentResultRow.qml
        ContentFilterDialog.qml
        ContentProjectPage.qml
        InstallReviewDialog.qml
        ContentFlows.qml
        UpdatesDialog.qml
        ExportListDialog.qml
        ProgressFooter.qml
        SortAction.qml
        SearchHeading.qml
        ModpackHeader.qml
        ModpackLinks.qml
        ContentActions.qml
)

target_link_libraries(MeshMC_qml_kirigami PUBLIC
    Qt${QT_VERSION_MAJOR}::Quick
    Qt${QT_VERSION_MAJOR}::QuickControls2
)
