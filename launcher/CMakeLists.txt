project(application)

################################ FILES ################################

######## Sources and headers ########

include (UnitTest)

add_unit_test(FileSystem
    SOURCES FileSystem_test.cpp
    LIBS MeshMC_logic
    DATA testdata
    )

add_unit_test(GZip
    SOURCES GZip_test.cpp
    LIBS MeshMC_logic
    )

add_unit_test(MMCZip
    SOURCES MMCZip_test.cpp
    LIBS MeshMC_logic
    )

# macOS updates go through Sparkle, which replaces the whole signed .app
# bundle. Sparkle is not packaged in vcpkg, so the upstream release archive is
# fetched and hash-checked here.
#
# Detected before the source list below, because whether MacSparkleUpdater can
# be compiled at all depends on the framework being present -- and a source
# file that needs headers we do not have is a build failure, not a fallback.
set(MeshMC_SPARKLE_ENABLED OFF)
if(APPLE AND NOT "${MACOSX_SPARKLE_UPDATE_PUBLIC_KEY}" STREQUAL "" AND NOT "${MACOSX_SPARKLE_UPDATE_FEED_URL}" STREQUAL "")
    if(NOT EXISTS "${MACOSX_SPARKLE_DIR}/Sparkle.framework")
        message(STATUS "Downloading Sparkle from ${MACOSX_SPARKLE_DOWNLOAD_URL}")
        file(DOWNLOAD "${MACOSX_SPARKLE_DOWNLOAD_URL}" "${CMAKE_BINARY_DIR}/Sparkle.tar.xz"
            EXPECTED_HASH SHA256=${MACOSX_SPARKLE_SHA256})
        file(ARCHIVE_EXTRACT INPUT "${CMAKE_BINARY_DIR}/Sparkle.tar.xz"
            DESTINATION "${MACOSX_SPARKLE_DIR}")
    endif()

    find_library(SPARKLE_FRAMEWORK Sparkle PATHS "${MACOSX_SPARKLE_DIR}" NO_DEFAULT_PATH)
    if(SPARKLE_FRAMEWORK)
        set(MeshMC_SPARKLE_ENABLED ON)
        message(STATUS "Sparkle updater: ${SPARKLE_FRAMEWORK}")
    else()
        # Not fatal: a build without Sparkle simply has no updater on macOS,
        # which is better than not building at all.
        message(WARNING "Sparkle.framework was not found under ${MACOSX_SPARKLE_DIR}; macOS updates will be disabled.")
    endif()
endif()

add_unit_test(GradleSpecifier
    SOURCES minecraft/GradleSpecifier_test.cpp
    LIBS MeshMC_logic
    )

add_executable(PackageManifest
    mojang/PackageManifest_test.cpp
)
target_link_libraries(PackageManifest
    MeshMC_logic
    Qt${QT_VERSION_MAJOR}::Test
)
target_include_directories(PackageManifest
    PRIVATE ../cmake/UnitTest/
)
add_test(
    NAME PackageManifest
    COMMAND PackageManifest
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_unit_test(MojangVersionFormat
    SOURCES minecraft/MojangVersionFormat_test.cpp
    LIBS MeshMC_logic
    DATA minecraft/testdata
    )

add_unit_test(Library
    SOURCES minecraft/Library_test.cpp
    LIBS MeshMC_logic
    )

# FIXME: shares data with FileSystem test
add_unit_test(ModFolderModel
    SOURCES minecraft/mod/ModFolderModel_test.cpp
    DATA testdata
    LIBS MeshMC_logic
    )

add_unit_test(ParseUtils
    SOURCES minecraft/ParseUtils_test.cpp
    LIBS MeshMC_logic
    )

add_unit_test(World
    SOURCES minecraft/World_test.cpp
    LIBS MeshMC_logic
    )

add_unit_test(INIFile
    SOURCES settings/INIFile_test.cpp
    LIBS MeshMC_logic
    )

add_unit_test(JavaVersion
    SOURCES java/JavaVersion_test.cpp
    LIBS MeshMC_logic
    )

add_unit_test(PackContents
    SOURCES modplatform/PackContents_test.cpp
    LIBS MeshMC_logic
    )

add_unit_test(Index
    SOURCES meta/Index_test.cpp
    LIBS MeshMC_logic
    )

################################ COMPILE ################################

# zlib, libarchive, cmark and toml++ are resolved in the top-level
# CMakeLists.txt -- from vcpkg under the presets, from the system package when
# configured without the vcpkg toolchain. nbt++ is a subtree under libraries/
# and is built here. Either way the imported target names below are the same.

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Add executable
add_library(MeshMC_logic STATIC)

target_sources(MeshMC_logic PRIVATE
    # Application base
    Application.h
    Application.cpp
    ApplicationMessage.h
    ApplicationMessage.cpp

    # GUI - general utilities
    DesktopServices.h
    DesktopServices.cpp
    VersionProxyModel.h
    VersionProxyModel.cpp
    HoeDown.h

    # Super secret!
    KonamiCode.h
    KonamiCode.cpp

    # FIXME: maybe find a better home for this.
    SkinUtils.cpp
    SkinUtils.h

    # Processes
    LaunchController.h
    LaunchController.cpp

    # page provider for instances
    InstancePageProvider.h

    # Common java checking UI
    JavaCommon.h
    JavaCommon.cpp
)

######## Windows resource files ########
if(WIN32)
    set(MESHMC_RCS ${CMAKE_BINARY_DIR}/meshmc.rc)
endif()

target_include_directories(MeshMC_logic PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_sources(MeshMC_logic PRIVATE
    # LOGIC - Base classes and infrastructure
    BaseInstaller.h
    BaseInstaller.cpp
    BaseVersionList.h
    BaseVersionList.cpp
    InstanceList.h
    InstanceList.cpp
    InstanceTask.h
    InstanceTask.cpp
    LoggedProcess.h
    LoggedProcess.cpp
    Logging.h
    Logging.cpp
    MessageLevel.cpp
    MessageLevel.h
    BaseVersion.h
    BaseInstance.h
    BaseInstance.cpp
    Sys.cpp
    Sys.h
    NullInstance.h
    QtCompat.h
    ArchiveOpen.h
    MMCZip.h
    MMCZip.cpp
    MMCStrings.h
    MMCStrings.cpp

    # Basic instance manipulation tasks (derived from InstanceTask)
    InstanceCreationTask.h
    InstanceCreationTask.cpp
    InstanceCopyTask.h
    InstanceCopyTask.cpp
    InstanceImportTask.h
    InstanceImportTask.cpp

    # Use tracking separate from memory management
    Usable.h

    # Prefix tree where node names are strings between separators
    SeparatorPrefixTree.h

    # The checkable file tree the export dialogs are built around, and the
    # icon provider that keeps pointing it at an entire instance directory
    # from costing a per-file platform lookup.
    FileIgnoreProxy.h
    FileIgnoreProxy.cpp
    FastFileIconProvider.h
    FastFileIconProvider.cpp

    # String filters
    Filter.h
    Filter.cpp

    # JSON parsing helpers
    Json.h
    Json.cpp

    FileSystem.h
    FileSystem.cpp

    Exception.h

    # RW lock protected map
    RWStorage.h

    # A variable that has an implicit default value and keeps track of changes
    DefaultVariable.h

    # a smart pointer wrapper intended for safer use with Qt signal/slot mechanisms
    QObjectPtr.h

    # Compression support
    GZip.h
    GZip.cpp

    # Command line parameter parsing
    Commandline.h
    Commandline.cpp

    # Version number string support
    Version.h
    Version.cpp

    # A Recursive file system watcher
    RecursiveFileSystemWatcher.h
    RecursiveFileSystemWatcher.cpp

    # Time
    MMCTime.h
    MMCTime.cpp
)

add_subdirectory(pathmatcher)
add_subdirectory(net)
add_subdirectory(launch)
add_subdirectory(updater)
add_subdirectory(notifications)
add_subdirectory(news)
add_subdirectory(backup)
add_subdirectory(minecraft)
add_subdirectory(mojang)
add_subdirectory(screenshots)
add_subdirectory(tasks)
add_subdirectory(settings)
add_subdirectory(java)
add_subdirectory(translations)
add_subdirectory(tools)
add_subdirectory(meta)
add_subdirectory(icons)
add_subdirectory(modplatform)
add_subdirectory(plugin)
add_subdirectory(resources)
add_subdirectory(ui)

# Declared after MeshMC_logic because it links against it.
add_unit_test(CustomTheme
    SOURCES ui/themes/CustomTheme_test.cpp
    LIBS MeshMC_logic
    )

add_unit_test(IconTheme
    SOURCES ui/themes/IconTheme_test.cpp
    LIBS MeshMC_logic
    )

target_compile_definitions(IconTheme_test PRIVATE
    MESHMC_MAINWINDOW_UI_PATH="${CMAKE_CURRENT_SOURCE_DIR}/ui/MainWindow.ui")

set_tests_properties(IconTheme PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QPA_PLATFORMTHEME="
    )

target_link_libraries(MeshMC_logic
    classparser
    nbt++
    ZLIB::ZLIB
    tomlplusplus::tomlplusplus
    BuildConfig
    Katabasis
)

if(MeshMC_DISABLE_JAVA_DOWNLOADER)
    target_compile_definitions(MeshMC_logic PUBLIC MeshMC_DISABLE_JAVA_DOWNLOADER)
endif()

# Keep the function/line of every log message in *all* configurations. Qt only
# fills the QMessageLogContext in while QT_NO_DEBUG is absent, which CMake
# defines for every config except Debug - so without this, official builds log
# no source locations at all and the log is much harder to act on. PUBLIC
# because the message logger macros expand at every call site, including the
# ones in the application target.
# Cost, knowingly accepted: __FILE__ is baked into the binary for each
# translation unit that logs, so the binary grows and carries build paths.
target_compile_definitions(MeshMC_logic PUBLIC QT_MESSAGELOGCONTEXT)
target_link_libraries(MeshMC_logic
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Xml
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::NetworkAuth
    Qt${QT_VERSION_MAJOR}::Concurrent
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    LibArchive::LibArchive
    cmark::cmark
    rainbow
    LocalPeer::LocalPeer
)

if(QT_VERSION_MAJOR EQUAL 6)
    target_link_libraries(MeshMC_logic Qt6::OpenGL)
endif()

# Plugin system needs dlopen/dlsym on Unix
if(UNIX AND NOT APPLE)
    target_link_libraries(MeshMC_logic ${CMAKE_DL_LIBS})
endif()

# GpgME (C++ bindings) — MMCO plugin signature verification.
# Only linked when MeshMC_PLUGIN_SIGNATURES is ON (default off on MSVC,
# because upstream gpgme does not support MSVC). When disabled,
# PluginSignature.cpp compiles into a stub that reports every module
# as unsigned and never tries to include any gpgme++ headers.
if(MeshMC_PLUGIN_SIGNATURES)
    # Gpgmepp or GpgmeppStatic, depending on the linkage of the installation
    # that was found; resolved in the top-level CMakeLists.
    target_link_libraries(MeshMC_logic ${MeshMC_GpgmeppTarget})
    target_compile_definitions(MeshMC_logic PRIVATE MESHMC_PLUGIN_SIGNATURES=1)
else()
    target_compile_definitions(MeshMC_logic PRIVATE MESHMC_PLUGIN_SIGNATURES=0)
endif()

if(APPLE)
    target_link_libraries(MeshMC_logic
        "-framework AppKit"
        "-framework Foundation"
        objc
    )
    set(CMAKE_MACOSX_RPATH 1)
    set(CMAKE_INSTALL_RPATH "@loader_path/../Frameworks/")
endif()

if(MeshMC_SPARKLE_ENABLED)
    # PUBLIC: Application.cpp decides which updater to construct on the
    # strength of this define, and it is compiled into the application target
    # as well as into the logic library.
    target_compile_definitions(MeshMC_logic PUBLIC MESHMC_SPARKLE_ENABLED)
    target_include_directories(MeshMC_logic PRIVATE "${MACOSX_SPARKLE_DIR}")
    target_link_libraries(MeshMC_logic ${SPARKLE_FRAMEWORK})

    # Sparkle has to ship inside the bundle: it is loaded at runtime, and it
    # is what checks the signature of the update it installs.
    install(DIRECTORY "${MACOSX_SPARKLE_DIR}/Sparkle.framework"
        DESTINATION "${FRAMEWORK_DEST_DIR}"
        USE_SOURCE_PERMISSIONS)
endif()

add_executable(MeshMC MACOSX_BUNDLE WIN32 main.cpp ${MESHMC_RCS})
target_link_libraries(MeshMC MeshMC_logic)

# Export all symbols so dlopen()'ed .mmco plugins can resolve launcher symbols at runtime
set_target_properties(MeshMC PROPERTIES ENABLE_EXPORTS ON)

if(MSVC)
    target_link_options(MeshMC PRIVATE "/MANIFEST:NO")
endif()

set_target_properties(MeshMC PROPERTIES OUTPUT_NAME "meshmc")

if(APPLE)
    set_target_properties(MeshMC PROPERTIES MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/../cmake/MacOSXBundleInfo.plist.in")
endif()
if(DEFINED MeshMC_BINARY_RPATH)
    SET_TARGET_PROPERTIES(MeshMC PROPERTIES INSTALL_RPATH "${MeshMC_BINARY_RPATH}")
endif()

if(DEFINED MeshMC_APP_BINARY_DEFS)
    target_compile_definitions(MeshMC PRIVATE ${MeshMC_APP_BINARY_DEFS})
    target_compile_definitions(MeshMC_logic PRIVATE ${MeshMC_APP_BINARY_DEFS})
endif()

if(APPLE)
    # CMake derives an app bundle's *directory* name from the target's
    # OUTPUT_NAME, and there is no property to decouple the two: with
    # OUTPUT_NAME "meshmc" the only thing install(TARGETS ... BUNDLE) can
    # produce is "meshmc.app" (RUNTIME DESTINATION is silently ignored for
    # bundle targets, and omitting BUNDLE DESTINATION is a hard error).
    #
    # Everything else, however, installs into "MeshMC.app": BINARY_DEST_DIR,
    # FRAMEWORK_DEST_DIR, RESOURCES_DEST_DIR, JARS_DEST_DIR, the MMCO_*
    # directories, the .icns/Assets.car rules in the top-level CMakeLists,
    # the qt_deploy_runtime_dependencies() call below and the macOS
    # packaging action. On a case-insensitive volume the two spellings
    # collapse into whichever directory is created first, so the mismatch
    # only shows up as odd casing; on a case-sensitive one the install
    # splits in half — a bundle without a binary next to a binary without
    # a bundle.
    #
    # Install the bundle into a staging directory (a name that cannot
    # collide with MeshMC.app under either casing rule) and merge it into
    # place. The staging hop is not cosmetic: install(TARGETS) is what
    # rewrites the build RPATH into INSTALL_RPATH via install_name_tool,
    # and it only does that for the copy it makes itself. Copying the
    # build-tree bundle directly would ship a binary still pointing at the
    # build directory.
    set(MeshMC_BundleStagingDir "_meshmc_bundle_staging")

    install(TARGETS MeshMC
        RUNTIME_DEPENDENCY_SET LAUNCHER_DEPENDENCY_SET
        BUNDLE DESTINATION "${MeshMC_BundleStagingDir}"
        LIBRARY DESTINATION ${LIBRARY_DEST_DIR}
    )

    # Runs before the qt_deploy_runtime_dependencies() script registered
    # below, which expects the binary to already sit at
    # ${BINARY_DEST_DIR}/$<TARGET_FILE_NAME>.
    install(CODE "
        set(_meshmc_prefix \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}\")
        set(_meshmc_staging \"\${_meshmc_prefix}/${MeshMC_BundleStagingDir}\")
        set(_meshmc_staged_bundle \"\${_meshmc_staging}/$<TARGET_BUNDLE_DIR_NAME:MeshMC>\")
        set(_meshmc_bundle \"\${_meshmc_prefix}/MeshMC.app\")
        if(NOT IS_DIRECTORY \"\${_meshmc_staged_bundle}\")
            message(FATAL_ERROR
                \"Staged app bundle '\${_meshmc_staged_bundle}' is missing; \"
                \"cannot assemble MeshMC.app\")
        endif()
        message(STATUS \"Merging \${_meshmc_staged_bundle} into \${_meshmc_bundle}\")
        file(COPY \"\${_meshmc_staged_bundle}/\" DESTINATION \"\${_meshmc_bundle}\")
        file(REMOVE_RECURSE \"\${_meshmc_staging}\")
    ")
else()
    install(TARGETS MeshMC
        RUNTIME_DEPENDENCY_SET LAUNCHER_DEPENDENCY_SET
        BUNDLE DESTINATION ${BUNDLE_DEST_DIR}
        LIBRARY DESTINATION ${LIBRARY_DEST_DIR}
        RUNTIME DESTINATION ${BINARY_DEST_DIR}
    )
endif()


############################## Standalone updater ##############################

# The updater is a separate executable on purpose. It has to replace the very
# libraries the launcher runs from, and a process cannot delete an image it
# has mapped -- so the code that does the replacing must not be part of the
# launcher. macOS is excluded: there Sparkle does the job from inside the
# launcher (see MacSparkleUpdater).
if(NOT APPLE)
    set(MESHUPDATER_UI_FILES
        updater/meshupdater/SelectReleaseDialog.ui
    )

    # Never the versionless wrapper, for the same reason as MESHMC_UI above.
    if(QT_VERSION_MAJOR EQUAL 6)
        qt6_wrap_ui(MESHUPDATER_UI ${MESHUPDATER_UI_FILES})
    else()
        qt5_wrap_ui(MESHUPDATER_UI ${MESHUPDATER_UI_FILES})
    endif()

    set(MESHUPDATER_SOURCES
        updater/meshupdater/MeshUpdaterApp.h
        updater/meshupdater/MeshUpdaterApp.cpp
        updater/meshupdater/AssetMatcher.h
        updater/meshupdater/AssetMatcher.cpp
        updater/meshupdater/GitHubRelease.h
        updater/meshupdater/GitHubRelease.cpp
        updater/meshupdater/ReleaseArchive.h
        updater/meshupdater/ReleaseArchive.cpp
        updater/meshupdater/ReleaseDownload.h
        updater/meshupdater/ReleaseDownload.cpp
        updater/meshupdater/UpdaterDialogs.h
        updater/meshupdater/UpdaterDialogs.cpp

        # Shared with the launcher: the marker files are an interface between
        # the two, so there is one definition of them and not two.
        updater/UpdateLockFile.h
        updater/UpdateLockFile.cpp

        # Borrowed from the launcher, all free of Application dependencies.
        ArchiveOpen.h
        Version.h
        Version.cpp
        MMCStrings.h
        MMCStrings.cpp
        HoeDown.h
        DesktopServices.h
        DesktopServices.cpp
        QObjectPtr.h

        tasks/Task.h
        tasks/Task.cpp

        # The download uses the launcher's own progress dialog, so it looks
        # like every other download MeshMC does.
        ui/dialogs/ProgressDialog.h
        ui/dialogs/ProgressDialog.cpp
        ui/dialogs/ProgressDialog.ui
        ui/widgets/TaskStepProgressBar.h
        ui/widgets/TaskStepProgressBar.cpp
        ui/widgets/TaskStepProgressBar.ui
        updater/meshupdater/SelectReleaseDialog.ui
    )

    add_library(meshmc_updater_logic STATIC ${MESHUPDATER_SOURCES} ${MESHUPDATER_UI})
    target_include_directories(meshmc_updater_logic PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
    target_compile_definitions(meshmc_updater_logic PUBLIC QT_MESSAGELOGCONTEXT)
    target_link_libraries(meshmc_updater_logic
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Network
        LibArchive::LibArchive
        cmark::cmark
        BuildConfig
    )

    # WIN32: no console window while updating. Diagnostics go to the log file,
    # and --debug borrows the parent console when there is one.
    add_executable(meshmc-updater WIN32 updater/meshupdater/updater_main.cpp)
    if(WIN32)
        target_sources(meshmc-updater PRIVATE updater/meshupdater/updater.exe.manifest)
    endif()
    target_link_libraries(meshmc-updater meshmc_updater_logic)

    if(MSVC)
        # The manifest above is embedded explicitly; letting the linker
        # generate one as well is an error.
        target_link_options(meshmc-updater PRIVATE "/MANIFEST:NO")
    endif()

    if(DEFINED MeshMC_BINARY_RPATH)
        set_target_properties(meshmc-updater PROPERTIES INSTALL_RPATH "${MeshMC_BINARY_RPATH}")
    endif()

    # Next to the launcher, which is where MeshMCExternalUpdater looks for it
    # -- and where Application::updaterEnabled() checks before offering
    # updates at all.
    install(TARGETS meshmc-updater
        RUNTIME DESTINATION "${BINARY_DEST_DIR}"
    )

    add_unit_test(AssetMatcher
        SOURCES updater/meshupdater/AssetMatcher_test.cpp
        LIBS meshmc_updater_logic
        )

    add_unit_test(ReleaseArchive
        SOURCES updater/meshupdater/ReleaseArchive_test.cpp
        LIBS meshmc_updater_logic LibArchive::LibArchive
        )

    # The file the two passes of an install hand over through.
    add_unit_test(UpdateLockFile
        SOURCES updater/UpdateLockFile_test.cpp
        LIBS meshmc_updater_logic
        )
endif()

#### The bundle mess! ####
# Bundle utilities are used to complete the portable packages - they add all the libraries that would otherwise be missing on the target system.
# NOTE: it seems that this absolutely has to be here, and nowhere else.
if(WIN32 OR (UNIX AND APPLE))
    if(WIN32)
        # Find windeployqt directly — Qt's qt_generate_deploy_script tries to find
        # windeployqt.debug.bat for Debug builds, which may not be available.
        #
        # Deriving the directory from the qmake target alone is not enough:
        # depending on how Qt was packaged the host tools can sit somewhere
        # other than next to qmake, so qmake is asked directly as well. And
        # packagings that carry Qt 5 and Qt 6 side by side suffix the tool
        # name to keep the two apart, hence the name list.
        get_target_property(_qt_qmake_loc Qt${QT_VERSION_MAJOR}::qmake IMPORTED_LOCATION)
        get_filename_component(_qt_bin_dir "${_qt_qmake_loc}" DIRECTORY)

        QUERY_QMAKE(QT_INSTALL_BINS _qt_install_bins)
        QUERY_QMAKE(QT_HOST_BINS _qt_host_bins)

        find_program(WINDEPLOYQT_EXECUTABLE
            NAMES
                windeployqt
                windeployqt-qt${QT_VERSION_MAJOR}
                windeployqt${QT_VERSION_MAJOR}
            HINTS
                "${_qt_bin_dir}"
                "${_qt_install_bins}"
                "${_qt_host_bins}"
                "${Qt${QT_VERSION_MAJOR}_DIR}/../../../bin"
        )

        if(NOT WINDEPLOYQT_EXECUTABLE)
            # Report what was actually searched. The bare "not found" this
            # used to print gave nothing to act on, which is expensive when
            # the only place it reproduces is a CI runner.
            message(FATAL_ERROR
                "windeployqt not found.\n"
                "  Names searched      : windeployqt, "
                "windeployqt-qt${QT_VERSION_MAJOR}, windeployqt${QT_VERSION_MAJOR}\n"
                "  qmake executable    : ${_qt_qmake_loc}\n"
                "  qmake directory     : ${_qt_bin_dir}\n"
                "  QT_INSTALL_BINS     : ${_qt_install_bins}\n"
                "  QT_HOST_BINS        : ${_qt_host_bins}\n"
                "  Qt CMake dir + /bin : ${Qt${QT_VERSION_MAJOR}_DIR}/../../../bin\n"
                "windeployqt ships with Qt's qtbase tools; make sure they are "
                "installed for this Qt and that the directories above look right.")
        endif()

        message(STATUS "windeployqt: ${WINDEPLOYQT_EXECUTABLE}")

        # windeployqt's option set is not stable across Qt releases:
        # --skip-plugin-types and --no-system-dxc-compiler are Qt 6 additions
        # (and the latter only appeared partway through the Qt 6 series). Qt 5's
        # windeployqt does not merely ignore them, it bails out with
        # "Unknown options: ..." and a non-zero exit code, which fails install.
        #
        # Deriving the option list from the Qt version would mean encoding which
        # release added what, so the tool is asked directly instead: anything it
        # does not advertise in its help output is left off the command line.
        execute_process(
            COMMAND "${WINDEPLOYQT_EXECUTABLE}" --help
            OUTPUT_VARIABLE _windeployqt_help_out
            ERROR_VARIABLE _windeployqt_help_err
            RESULT_VARIABLE _windeployqt_help_result
        )
        # Some builds print the usage text on stderr, so consider both streams.
        set(_windeployqt_help "${_windeployqt_help_out}${_windeployqt_help_err}")
        if(NOT _windeployqt_help_result EQUAL 0)
            # Not fatal: the optional switches below are optimisations, and
            # dropping them only means a slightly larger deployment.
            message(WARNING
                "Could not query '${WINDEPLOYQT_EXECUTABLE} --help' "
                "(exit code: ${_windeployqt_help_result}). Deploying with the "
                "baseline option set; the package may contain plugins that "
                "would otherwise have been skipped.")
            set(_windeployqt_help "")
        endif()

        # Understood by both Qt 5 and Qt 6 windeployqt.
        # install-qt-action and MSYS2 only provide release Qt, so always use
        # --release even when the app itself is built in Debug mode.
        set(WINDEPLOYQT_OPTIONS
            --release
            --no-opengl-sw
            --no-quick-import
            --no-system-d3d-compiler
            --no-translations
            --no-compiler-runtime
        )
        if(_windeployqt_help MATCHES "no-system-dxc-compiler")
            list(APPEND WINDEPLOYQT_OPTIONS --no-system-dxc-compiler)
        else()
            message(STATUS "windeployqt: --no-system-dxc-compiler unsupported, skipping it")
        endif()
        if(_windeployqt_help MATCHES "skip-plugin-types")
            list(APPEND WINDEPLOYQT_OPTIONS --skip-plugin-types generic,networkinformation)
        else()
            message(STATUS "windeployqt: --skip-plugin-types unsupported, skipping it")
        endif()

        if(_windeployqt_help MATCHES "no-angle")
            set(_qt_angle_dll "")
            foreach(_qt_angle_dir IN ITEMS
                "${_qt_bin_dir}" "${_qt_install_bins}" "${_qt_host_bins}")
                if(_qt_angle_dir AND EXISTS "${_qt_angle_dir}/libGLESv2.dll")
                    set(_qt_angle_dll "${_qt_angle_dir}/libGLESv2.dll")
                    break()
                endif()
            endforeach()
            if(_qt_angle_dll)
                message(STATUS "windeployqt: ANGLE present (${_qt_angle_dll}), deploying it")
            else()
                message(STATUS
                    "windeployqt: no libGLESv2.dll alongside Qt, passing --no-angle")
                list(APPEND WINDEPLOYQT_OPTIONS --no-angle)
            endif()
        endif()

        message(STATUS "windeployqt options: ${WINDEPLOYQT_OPTIONS}")

        # Pre-render the options as one quoted argument per line. Expanding the
        # list directly into the install(CODE) string would paste it in
        # semicolon-separated, i.e. as a single bogus argument.
        set(WINDEPLOYQT_OPTIONS_CODE "")
        foreach(_windeployqt_opt IN LISTS WINDEPLOYQT_OPTIONS)
            string(APPEND WINDEPLOYQT_OPTIONS_CODE "\n                    \"${_windeployqt_opt}\"")
        endforeach()
    endif()

    if(UNIX AND APPLE)
        # qt_generate_deploy_script/qt_deploy_runtime_dependencies are Qt 6.3+
        # only. Qt 5 has no CMake equivalent, but it ships macdeployqt, which
        # is the supported way to do the same job, so the Qt 5 branch below
        # shells out to it.
        if(QT_VERSION_MAJOR EQUAL 6)
            qt_generate_deploy_script(
                TARGET MeshMC
                OUTPUT_SCRIPT QT_DEPLOY_SCRIPT
                CONTENT "
                qt_deploy_runtime_dependencies(
                    EXECUTABLE ${BINARY_DEST_DIR}/$<TARGET_FILE_NAME:MeshMC>
                    BIN_DIR ${BINARY_DEST_DIR}
                    LIBEXEC_DIR ${BINARY_DEST_DIR}
                    LIB_DIR ${LIBRARY_DEST_DIR}
                    PLUGINS_DIR ${PLUGIN_DEST_DIR}
                    NO_OVERWRITE
                    NO_TRANSLATIONS
                    NO_COMPILER_RUNTIME
                )"
            )
            install(
                SCRIPT ${QT_DEPLOY_SCRIPT}
            )
        else()
            # Qt 5: deploy with macdeployqt. Located the same way windeployqt
            # is above, via the qmake target's directory.
            #
            # Deliberately not a configure-time hard error when the tool is
            # missing: configuring and building a Qt 5 macOS tree has to keep
            # working even where the deploy step will never run (a
            # compile-only CI job, for instance). The tool is only genuinely
            # required by `install`, so that is where it is enforced.
            get_target_property(_qt_macdeploy_qmake
                Qt${QT_VERSION_MAJOR}::qmake IMPORTED_LOCATION)
            get_filename_component(_qt_macdeploy_bin_dir
                "${_qt_macdeploy_qmake}" DIRECTORY)
            find_program(MACDEPLOYQT_EXECUTABLE macdeployqt
                HINTS "${_qt_macdeploy_bin_dir}")

            if(NOT MACDEPLOYQT_EXECUTABLE)
                message(WARNING
                    "macdeployqt was not found next to qmake "
                    "(${_qt_macdeploy_bin_dir}). Building will work, but "
                    "installing will fail: the .app would be missing its Qt "
                    "frameworks and plugins. It ships with Qt 5's qtbase.")
            endif()

            # Runs after the install(CODE) further up that merges the staged
            # bundle into ${CMAKE_INSTALL_PREFIX}/MeshMC.app, so the
            # bundle macdeployqt is pointed at already exists. This sits at
            # the same point in the install order as the Qt 6 branch's
            # install(SCRIPT) above.
            #
            # Only the main executable is handed over, matching what the Qt 6
            # branch passes as EXECUTABLE. Helper binaries in the bundle (the
            # updater, the crash reporter) and the .mmco plugins are therefore
            # not rewritten by macdeployqt on either branch.
            install(CODE "
                set(_meshmc_macdeployqt \"${MACDEPLOYQT_EXECUTABLE}\")
                set(_meshmc_bundle \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/MeshMC.app\")

                if(NOT _meshmc_macdeployqt)
                    message(FATAL_ERROR
                        \"macdeployqt was not found, so '\${_meshmc_bundle}' \"
                        \"cannot be completed. It ships with Qt 5's qtbase; \"
                        \"make sure Qt's bin directory is available.\")
                endif()
                if(NOT IS_DIRECTORY \"\${_meshmc_bundle}\")
                    message(FATAL_ERROR
                        \"App bundle '\${_meshmc_bundle}' is missing; \"
                        \"cannot run macdeployqt on it.\")
                endif()

                message(STATUS \"Deploying Qt into \${_meshmc_bundle}\")
                execute_process(
                    COMMAND \"\${_meshmc_macdeployqt}\" \"\${_meshmc_bundle}\"
                    RESULT_VARIABLE _deploy_result
                )
                if(NOT _deploy_result EQUAL 0)
                    message(FATAL_ERROR \"macdeployqt failed with exit code: \${_deploy_result}\")
                endif()
            ")
        endif()
    endif()

    # Bundle our linked dependencies
    # Collect bin/ directories from CMAKE_PREFIX_PATH (both CMake variable and
    # environment variable) so RUNTIME_DEPENDENCY_SET can locate third-party
    # DLLs that live in a dependency prefix.
    #
    # nbt++/rainbow are NOT handled here: they are our own targets in
    # this build tree, and the top-level CMakeLists.txt installs them directly
    # to the right place. Everything else under libraries/ links statically.
    set(_prefix_path_bins "")
    foreach(_prefix IN LISTS CMAKE_PREFIX_PATH)
        list(APPEND _prefix_path_bins "${_prefix}/bin")
    endforeach()
    if(DEFINED ENV{CMAKE_PREFIX_PATH})
        file(TO_CMAKE_PATH "$ENV{CMAKE_PREFIX_PATH}" _env_prefixes)
        foreach(_prefix IN LISTS _env_prefixes)
            list(APPEND _prefix_path_bins "${_prefix}/bin")
        endforeach()
    endif()
    list(REMOVE_DUPLICATES _prefix_path_bins)
    install(
        RUNTIME_DEPENDENCY_SET LAUNCHER_DEPENDENCY_SET
        DIRECTORIES
            ${CMAKE_SYSTEM_LIBRARY_PATH}
            ${QT_LIBS_DIR}
            ${QT_LIBEXECS_DIR}
            ${_prefix_path_bins}
        PRE_EXCLUDE_REGEXES
            "^(api-ms-win|ext-ms)-.*\\.dll$"
            "^azure.*\\.dll$"
            "^vcruntime.*\\.dll$"
        POST_EXCLUDE_REGEXES
            "system32"
        LIBRARY DESTINATION ${LIBRARY_DEST_DIR}
        RUNTIME DESTINATION ${BINARY_DEST_DIR}
        FRAMEWORK DESTINATION ${FRAMEWORK_DEST_DIR}
    )

    if(WIN32)
        # Deploy Qt plugins and DLLs using windeployqt. WINDEPLOYQT_OPTIONS is
        # assembled above from the switches this windeployqt actually supports.
        install(CODE "
            execute_process(
                COMMAND \"${WINDEPLOYQT_EXECUTABLE}\"${WINDEPLOYQT_OPTIONS_CODE}
                    --dir \"\${CMAKE_INSTALL_PREFIX}/${BINARY_DEST_DIR}\"
                    --libdir \"\${CMAKE_INSTALL_PREFIX}/${LIBRARY_DEST_DIR}\"
                    --plugindir \"\${CMAKE_INSTALL_PREFIX}/${PLUGIN_DEST_DIR}\"
                    \"\${CMAKE_INSTALL_PREFIX}/${BINARY_DEST_DIR}/$<TARGET_FILE_NAME:MeshMC>\"
                RESULT_VARIABLE _deploy_result
            )
            if(NOT _deploy_result EQUAL 0)
                message(FATAL_ERROR \"windeployqt failed with exit code: \${_deploy_result}\")
            endif()
        ")

        #################### OpenSSL runtime (Windows) ####################
        #
        # Qt resolves OpenSSL with dlopen/LoadLibrary rather than linking it,
        # so neither windeployqt nor install(RUNTIME_DEPENDENCY_SET) ever sees
        # it and the package ships without a TLS backend. Qt 6 hides this on
        # Windows by falling back to its Schannel plugin; Qt 5 has no such
        # fallback, so every https:// request fails with
        # QNetworkReply::SslHandshakeFailedError.
        #
        # The libraries are named rather than searched for on purpose: which
        # OpenSSL is correct depends on the *Qt build*, not on the machine
        # (official Qt 5.15 wants the 1.1.1 ABI, MSYS2's Qt wants 3.x), and
        # picking up whatever happens to sit in a bin directory is how you end
        # up shipping a DLL Qt cannot use. CI passes the exact files.
        if(MeshMC_BUNDLE_OPENSSL_LIBS)
            foreach(_openssl_lib IN LISTS MeshMC_BUNDLE_OPENSSL_LIBS)
                if(NOT EXISTS "${_openssl_lib}")
                    message(FATAL_ERROR
                        "MeshMC_BUNDLE_OPENSSL_LIBS lists '${_openssl_lib}', "
                        "which does not exist. Shipping the package without it "
                        "would leave the launcher unable to make any HTTPS "
                        "request, so this is a hard error rather than a warning.")
                endif()
                message(STATUS "Bundling OpenSSL runtime: ${_openssl_lib}")
            endforeach()
            install(FILES ${MeshMC_BUNDLE_OPENSSL_LIBS} DESTINATION ${BINARY_DEST_DIR})
        elseif(QT_VERSION_MAJOR EQUAL 5)
            # Warning, not an error: a developer build that is never packaged
            # is perfectly usable as long as an OpenSSL 1.1 happens to be on
            # PATH. It is the redistributable package that must not go out
            # like this.
            message(WARNING
                "No OpenSSL runtime is being bundled (MeshMC_BUNDLE_OPENSSL_LIBS "
                "is empty) and this is a Qt 5 build. Qt 5 on Windows has no "
                "Schannel fallback, so the resulting package will fail every "
                "HTTPS request unless a matching OpenSSL 1.1 is already on the "
                "user's PATH.")
        endif()
    endif()

    # FIXME: remove this crap once we stop using msys2
    if(MINGW)
        install(CODE "
            set(QT_PLUGINS_DIR \"${QT_PLUGINS_DIR}\")
            set(QT_LIBS_DIR \"${QT_LIBS_DIR}\")
            set(QT_LIBEXECS_DIR \"${QT_LIBEXECS_DIR}\")
            set(CMAKE_SYSTEM_LIBRARY_PATH \"${CMAKE_SYSTEM_LIBRARY_PATH}\")
            set(CMAKE_INSTALL_PREFIX \"${CMAKE_INSTALL_PREFIX}\")
            ")

        install(CODE [[
            file(GLOB QT_IMAGEFORMAT_DLLS "${QT_PLUGINS_DIR}/imageformats/*.dll")
            set(CMAKE_GET_RUNTIME_DEPENDENCIES_TOOL objdump)
            file(GET_RUNTIME_DEPENDENCIES
                RESOLVED_DEPENDENCIES_VAR imageformatdeps
                LIBRARIES ${QT_IMAGEFORMAT_DLLS}
                DIRECTORIES
                    ${CMAKE_SYSTEM_LIBRARY_PATH}
                    ${QT_PLUGINS_DIR}
                    ${QT_LIBS_DIR}
                    ${QT_LIBEXECS_DIR}
                PRE_EXCLUDE_REGEXES
                    "^(api-ms-win|ext-ms)-.*\\.dll$"
                    "^azure.*\\.dll$"
                    "^vcruntime.*\\.dll$"
                POST_EXCLUDE_REGEXES
                    "system32"
            )
            foreach(_lib ${imageformatdeps})
                file(INSTALL
                    DESTINATION ${CMAKE_INSTALL_PREFIX}
                    TYPE SHARED_LIBRARY
                    FOLLOW_SYMLINK_CHAIN
                    FILES ${_lib}
                )
            endforeach()
        ]])
    endif()

    # Add qt.conf - tell Qt where to find plugins relative to the executable
    install(
        CODE "file(WRITE \"\${CMAKE_INSTALL_PREFIX}/${RESOURCES_DEST_DIR}/qt.conf\" \"[Paths]\nPlugins = ${PLUGIN_DEST_DIR}\n\")"
    )
    # Add qtlogging.ini as a config file
    install(
        FILES "qtlogging.ini"
        DESTINATION ${CMAKE_INSTALL_PREFIX}/${RESOURCES_DEST_DIR}
    )
endif()
