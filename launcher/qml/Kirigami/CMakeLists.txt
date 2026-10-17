# SPDX-FileCopyrightText: 2026 Project Tick
# SPDX-FileContributor: Project Tick
# SPDX-License-Identifier: Apache-2.0

######## The KDE interface (MeshMC.Kirigami) ########

# A stock Kirigami application: Kirigami.ApplicationWindow, its page stack and
# global drawer, the org.kde.desktop controls style, Breeze icons by name,
# Kirigami.Theme colours and Kirigami.Units sizes.
#
# Same layout rules as the other modules (see launcher/qml/CMakeLists.txt):
# RESOURCE_PREFIX by hand for the 6.4 floor, and this file beside the .qml it
# lists, so the root window sits at qrc:/qt/qml/MeshMC/Kirigami/Main.qml.
qt_add_library(MeshMC_qml_kirigami STATIC)

# The pictures artwork.js hands out: 32 rendered scenes (16:9) for an
# instance's card when it has no pack art or screenshot of its own, and a
# picture per biome for a world without an icon.png. Globbed rather than
# listed line by line, with CONFIGURE_DEPENDS so a changed set is picked up.
file(GLOB MESHMC_KIRIGAMI_ART RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} CONFIGURE_DEPENDS
    art/scenes/*.jpg art/worlds/*.jpg)

qt_add_qml_module(MeshMC_qml_kirigami
    URI MeshMC.Kirigami
    VERSION 1.0
    RESOURCE_PREFIX "/qt/qml"
    QML_FILES
        Main.qml
        DevRoute.qml
        Navigator.qml
        AppDrawer.qml
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
        combos.js
        RemoteRichText.qml
        DiscoverPage.qml
        DiscoverFeatured.qml
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
        brands.js
        artwork.js
        BrandIcon.qml
        LoaderLine.qml
        LastPlayedLabel.qml
        HomePage.qml
        HomeHero.qml
        WorldCard.qml
        ProjectHeader.qml
        ProjectLogo.qml
        InstanceTitle.qml
        InstanceBanner.qml
        # K3: Settings (a ConfigurationView window), Accounts, skins and an
        # instance's own settings.
        SettingsView.qml
        SettingsPage.qml
        SettingsSource.qml
        SettingSwitch.qml
        SettingNumber.qml
        SettingText.qml
        SettingChoice.qml
        SettingPath.qml
        paths.js
        memory.js
        MemoryText.qml
        MaxMemoryDelegate.qml
        StartMemoryDelegate.qml
        JavaPathDelegate.qml
        JavaPickerDialog.qml
        JvmArgsDelegate.qml
        JavaVendorDelegate.qml
        JavaDownloadDialog.qml
        ManagedJavaCard.qml
        FolderDelegate.qml
        OverrideSwitch.qml
        OverridableCard.qml
        GameWindowCard.qml
        ConsoleCard.qml
        CommandsCard.qml
        NativeLibrariesCard.qml
        SettingsGeneralPage.qml
        SettingsJavaPage.qml
        SettingsMinecraftPage.qml
        SettingsConsolePage.qml
        SettingsAppearancePage.qml
        SettingsCommandsPage.qml
        SettingsPluginsPage.qml
        SettingsProxyPage.qml
        SettingsToolsPage.qml
        SettingsLogUploadPage.qml
        SettingsLauncherLogsPage.qml
        LauncherLogsActions.qml
        SettingsUpdatesPage.qml
        AboutMeshMCPage.qml
        AccountsPage.qml
        AccountDelegate.qml
        AccountHeaderCard.qml
        AccountMenu.qml
        MicrosoftLoginDialog.qml
        OfflineAccountDialog.qml
        TextPromptDialog.qml
        AccountSkinPage.qml
        SkinEditorState.qml
        SkinLibraryCards.qml
        SkinLibraryDialogs.qml
        SkinTile.qml
        CapeTile.qml
        PlayerPreview.qml
        InstanceSettingsTab.qml
        InstanceJavaCards.qml
        TabStrip.qml
        TabToolBar.qml
        WatcherMessage.qml
        ConfirmActionDialog.qml
        PluginNode.qml
        PluginSurfaces.qml
        ComponentRow.qml
        MinecraftVersionDialog.qml
        LoaderInstallDialog.qml
        NewComponentDialog.qml
        VersionTab.qml
        DataPacksTab.qml
        ServerDialog.qml
        ServersTab.qml
        BackupsTab.qml
        FormTab.qml
        GameOptionDelegate.qml
        GameOptionsTab.qml
        NotesTab.qml
        ManagedPackTab.qml
        PluginsTab.qml
        # K4a: creating, importing and exporting instances; first run.
        NewInstancePage.qml
        ImportInstancePage.qml
        InstanceIdentityCard.qml
        MinecraftVersionList.qml
        LoaderPickerCard.qml
        ImportDropArea.qml
        ExportInstanceDialog.qml
        ExportPackForm.qml
        ExportFileTree.qml
        OnboardingView.qml
        OnboardingStep.qml
        OnboardingJavaStep.qml
    RESOURCES
        ${MESHMC_KIRIGAMI_ART}
)

target_link_libraries(MeshMC_qml_kirigami PUBLIC
    Qt${QT_VERSION_MAJOR}::Quick
    Qt${QT_VERSION_MAJOR}::QuickControls2
)
