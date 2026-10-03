# MeshMC 11.0.0 (DRAFT)

## Highlights



## Added

* Automatic icon refreshing upon theme change has been added.
* Support for reading non-ASCII paths has been added.
* A feature to view the number of days in your worlds has been added.
* The --world parameter has been added.
* A skin management system has been added.
* An option has been added to launch the instance using other accounts via the "Launch as" option.

## Changed

* The logging system has been updated. We now use categorical logging. You can disable unwanted categories using qtlogging.ini.
* We have revised the update system. Updates are handled via Sparkle for macOS and ZSync for AppImage. A setup.exe installation package is used for Windows, while a two-stage update mechanism is employed for portable Windows and Linux versions.
* The MainWindow used to have its UI drawn via .cpp code; now, Designer UI is used.
* The XDGIcon structure has been removed, and the system has switched to the internal Qt icon infrastructure.
* The logo has been revised to better align with standards for macOS Big Sur and other platforms.
* Deep branding has been completed.

## Fixed

* Fixed buttons that remained active even when no instance was selected in the MainWindow.
* An issue where the Publisher ID written to the registry during NSIS installations appeared as "MeshMC Contributors" has been resolved; it has been corrected to "Project Tick".

## Removed

* The iconfix library has been removed because it is no longer in use.

## Deprecated

## CI/CD Updates

* Linux CI images have been updated to 26.04.
* The Backport Action has been updated to version 4.7.0.
* The Developer Container base image version has been updated to the latest digest.

## Previous versions

## MeshMC 10.0.0

## A Big Change

* MeshMC is now offered under the Apache 2.0 license.

## Highlights

* It was heartbreaking, I'm not lying.
* Being alone is tough, but I like to persevere.
* This release is really full of emotion, no offense to anyone.
* Joking aside, this is the first time I'm releasing such a massive release.
* Everyone, calm down. Take a breath. Because if you try to hold it, you might choke, as I don't think you'll be able to read this release in one breath.

## Added

* Shortcut system has been added.
* A world selection feature has been added to the Shortcut system.
* Patreon link has been added.
* Title bar theming added for macOS.
* A setting to edit the Skins and Java folders has been added.
* The "Folder" tab in the top toolbar now includes a list of multiple useful folders.
* The mod installation and update system has been completely redesigned.
* Datapack installation and update system has been added.
* ShaderPack installation and update system has been completely redesigned.
* ResourcePack installation and update system has been completely redesigned.
* The ability to throw the instance in the trash has been added.
* Detailed logging feature has been added.
* Demo mode support has been added.
* The number of entries in the Instance Toolbar has been reduced and icons have been added.
* News display system has been added.
* Discord URL has been added.
* The ability to blacklist the plugin has been added.
* Backup system has been added.
* The "More News" tab has been added back.
* Toolbar locking system added.
* The ability to move two toolbars other than the instance toolbar has been added.
* A toolbar that works with the <ALT> key has been added, except for macOS.
* The backup process now shows a progress bar.
* ModPack management support has been added.
* Multiple instance folder support has been added.
* Export via MRPack and Curseforge ZIP has been added.
* A separate screen was created for loaders.
* vcpkg build system has been added.
* RPM Spec has been added.

## Changed

* GreenDark theme palette updated.
* The macOS ToolBar code has been rewritten.
* The repository structure has been rebuilt.
* Minecraft is now downloaded every time an instance is created.
* vcpkg automates bootstrap and package management.
* The `std::optional` feature, introduced in the C++17 standard, is now in use.
* Path corrections have been added for UNIX installations (excluding macOS), especially for Linux; compatibility with Debian and RedHat policies has been ensured.
* Crowdin translations have been updated.

## Fixed

* The problem of Minecraft not closing when trying to kill an instance on Windows has been solved.
* The error of not adding version entries in NSIS has been fixed.
* An issue where an extra MeshMC folder appeared inside the MeshMC folder (located in the AppData or Application Support directory on Windows and macOS) has been fixed, and a small migrator has been added to prevent migration issues.

## Removed

* The ability to directly delete instances has been removed.
* The Filelink plugin has been removed.
* MinGW aarch64 test and packaging removed.
* The NewsViewer plugin has been removed.
* The BackupSystem plugin has been removed.
* PackUpdater plugin has been removed.
* PackPortal plugin has been removed.
* The Feature Flag feature has been completely removed.
* All Rust code has been removed.
* Optional Bare library has been removed.
* Analysis collection has been removed.

## Deprecated

* MMCO API: The ability to add input to the Instance Toolbar has been deprecated and changed to no-op.

## MeshMC 9.1.0

## Highlights

* A classic backport release.
* It won't affect you much, but it can be a boon for package managers who value stability.

## Changed

* Updater has been completely revamped. [BACKPORT FROM 10.0.0]

## Fixed

* Manifest.txt generation has been fixed. [BACKPORT FROM 10.0.0]

## MeshMC 9.0.0

## Highlights

* Wow! How many months has it been?
* I was sent back to manually write the changelog.
* I wonder what changes MeshMC made for this major!

## Added

* SHA1-based mode verification has been added for Curseforge.

## Changed

* Translations are now located within the MeshMC main repository.
* The update system has been completely overhauled. [MANUAL UPDATE REQUIRED*]

## Fixed

* Errors specific to Windows in the Filelink plugin have been resolved.
* The issue of missing bundle signings on macOS has been resolved.
* The issue preventing MeshMC from opening via shortcut while in the system tray has been resolved.
* Icon rendering issues have been resolved.
* The issue of adding installed mods back to the mod upload list has been resolved.
* The issue of downloading mods simultaneously due to addiction problems has been resolved.

## MeshMC 8.2.0 (2026-07-15)

### changed (3 changes)

- [Bump version 8.1.1 -> 8.2.0](https://github.com/Project-Tick/MeshMC/commit/fe464e6e59b02e055a8acd820f4ba144eabde3da)
- [Graduate OfflineWiki from staging, serve the MeshMC wiki offline](https://github.com/Project-Tick/MeshMC/commit/52a7e2dd2c584134cb5f37355660aff545d13de7) ([merge request](https://github.com/Project-Tick/MeshMC/pull/33))
- [With Git Versioning, the snapshot feature was moved from staging to general use](https://github.com/Project-Tick/MeshMC/commit/7dc9ec3fc990f3d7d1b4c6fb1684f4c2c3cc8568) ([merge request](https://github.com/Project-Tick/MeshMC/pull/28))

### added (2 changes)

- [Add Rust Unleash feature flag engine with C++ bridge and UI](https://github.com/Project-Tick/MeshMC/commit/96f3dd6a42ee87c210ca658adf5dcfac2f99f2f8) ([merge request](https://github.com/Project-Tick/MeshMC/pull_requests/39))
- [Support for writing plugins in C has been added, and core functions have been...](https://github.com/Project-Tick/MeshMC/commit/0d7279e487437d20ac0f481312ec0887518a4f34) ([merge request](https://github.com/Project-Tick/MeshMC/pull/38))

### fixed (2 changes)

- [Fix MeshMC SystemTray Plugin bind issue and working issue](https://github.com/Project-Tick/MeshMC/commit/fb93657aa6ed97cffb46e3cabf33a2523791de4d) ([merge request](https://github.com/Project-Tick/MeshMC/pull/35))
- [Fixed MeshMC Wiki URL's to migrated Wiki pages GitHub to Project Tick GitLab Instance](https://github.com/Project-Tick/MeshMC/commit/86f0f48e6efac7202dc0e4d821230271fbe0b594) ([merge request](https://github.com/Project-Tick/MeshMC/pull/32))

## MeshMC 8.1.1 (2026-06-22)

### changed (1 change)

- [Bump version 8.1.0 -> 8.1.1](https://github.com/Project-Tick/MeshMC/commit/ba84a0160c3498dbf9d53725919e1ca87bbd994b)

## MeshMC 8.1.0 (2026-06-17)

### changed (1 change)

- [Bump version 8.0.0 -> 8.1.0](https://github.com/Project-Tick/MeshMC/commit/f24ea1e782cd3f295e9dd2c0cb0bab3d60d4fa77)

## MeshMC 8.0.0

### Highlights

* New plugins and hooks have been added.
* Numerous minor bugs have been fixed.
* That gave me quite a bit of trouble again.

Look how I summarized it in just three points! I think I should win a Nobel Prize. **:)**

### Added

* The SkinManager plugin has been added, allowing you to view and edit your skins in 3D.
* The Discord RPC plugin has been added so you can show off your MeshMC status on Discord.
* The SystemTray plugin has been added so you can now easily use MeshMC in the system tray.
* The DesktopNotifier plugin has been added to provide you with more detailed notifications.
* APIs S18, S19, and S20 have been added.
* MMCS security extension added. You can now sign your plugins.
* The dependency graph feature has been added.
* Mod metadata index and conflict analysis system added.

### Changed

* DependencyResolver used in mods has been improved.
* The update system has been completely revamped.
* The UI injector system in Plugin Manager has been improved.
* Plugins are now fully independent of the launcher binary; they build standalone, both in-tree and out-of-tree, against the SDK alone.

### Fixed

* The error of adding multiple offline accounts with the same name has been fixed.
* The mod system has been fixed to prevent the same mod from being downloaded repeatedly.

### Removed

* The old, deprecated Update system has been completely removed.

## MeshMC 7.19.2

### Fixed

* Fixed MeshMC macOS bundle issue.

## MeshMC 7.19.1

### Fixed

* Fixed MeshMC Repo and more URL's

## MeshMC 7.19.0

### Added

* Added new NewsViewer plugin
* Added new API hooks

## MeshMC 7.18.0

### Highlights

* Removed GitHub control from the MeshMC updater.
* Fixed the debug message appearance in the MeshMC Plugin System.
* Updated BuildConfig to remove the `-rSHA` suffix and related build metadata handling.
* Improved MeshMC CrashReporter by censoring sensitive regex patterns.

### Changed

* Changed BuildConfig so release metadata no longer includes the `-rSHA` suffix.

### Fixed

* Fixed MeshMC updater behavior by removing GitHub control.
* Fixed the debug message appearing during the MeshMC plugin flow.
* Fixed CrashReporter privacy handling by censoring sensitive regex matches.

### Removed

* Removed GitHub control from the MeshMC updater.
* Removed the `-rSHA` suffix from BuildConfig output.

## MeshMC 7.17.0

### Changed
* Changed bug report url and source of truth url for metainfo
* Changed search mmcmodules directories
* Changed MeshMC Plugins home dir. Please create a backup.

## MeshMC 7.16.0

### Fixed

* Fixed Flatpak MangoHUD integration by detecting the mounted runtime extension directly and using absolute wrapper paths instead of PATH-dependent lookup.

## MeshMC 7.15.0

### Fixed

* Fixed LinuxPerf plugin to improve Flatpak sandbox experience

## MeshMC 7.14.0

### Highlights

MeshMC now includes a new Linux performance integration plugin,
bringing launcher-level support for MangoHud and GameMode. This
improves the Linux gameplay launch path by allowing performance
tooling to be injected and managed more cleanly during Minecraft
startup, instead of relying on users to wire everything manually
like it is 1998 and desktop Linux is still a punishment ritual.

This release also improves launch wrapper state handling by moving
transient launch state into `LaunchTask`, making the launch flow
cleaner, less fragile, and easier to maintain. Module/version
management has also been improved through new SDK version
definitions in `mmco_sdk.h`.

And this is the final version of Project Tick before the Beta and
LTS channels are released. Just so you know.

### Added

* Added the `LinuxPerf` plugin for Linux performance tooling integration.
* Added MangoHud integration support through the new Linux performance plugin.
* Added GameMode integration support for Minecraft launches.
* Added versioning definitions to `mmco_sdk.h` to improve module management and compatibility tracking.
* Added a confirmation dialog before deleting skins in `AccountListPage`, reducing the chance of accidental skin removal.

### Changed

* Refactored launch wrapper handling to use `LaunchTask` for transient state management.
* Improved launch-related documentation after the launch wrapper refactor.

### Fixed

* Added Apple-specific install RPATH handling for `crashreporter` and `updater` targets.
* Improved platform-specific install behavior for macOS helper targets.

## MeshMC 7.13.0

### Fixed

* Improved app location handling in FilelinkPlugin for Flatpak

## MeshMC 7.12.0

### Highlights

General fixes have been implemented.
MeshMC 7.12.0 is here! The Filelink plugin
(macOS, you have nothing to do with it) has
been updated to version 2.0.0 and made more
stable.

* Offline account support with demo mode
* Initial RPM packaging support
* FilelinkPlugin improvements with customizable shortcuts and better icon handling

### Added

* Offline account support with demo mode functionality
* Initial RPM spec file for MeshMC packaging
* LICENSE file inclusion
* FilelinkPlugin support for customizable shortcut paths

### Changed

* Git URLs updated across CMakeLists, metainfo, and Flatpak manifest
* README and source URLs aligned with GitLab hosting
* Build and workflow configurations cleaned and standardized

### Fixed

* Improved icon handling in FilelinkPlugin
* Flatpak workflow path inconsistencies
* Repository path variable inconsistencies in build workflows

## MeshMC 7.11.0

### Highlights

This is a kind of return to the classic theme, but it still
includes some solid fixes. Although it might not make much
difference to humanity, it's the version that fooled me by
making me refresh the screenshots.

* Improved UI responsiveness and stability
* New Instance Settings integration
* Version 7.11.0 with new themes

### Added

* Instance Settings action in MainWindow toolbar
* GreenDark and GreenLight themes

### Changed

* Modpack detection moved to background thread
* Replaced QDesktopServices with DesktopServices
* Updated minimum CMake version to 3.20
* Updated Screenshots for new themes

### Fixed

* Crash caused by improper model reset handling in setSourceModel
* Error handling improvements in archive read/write
* Prevented unnecessary processing for empty logo URLs

## MeshMC 7.10.0

### Fixed

* Fixed regression for dont launching X11

## MeshMC 7.9.0

### Fixed

* Removed icon tag in MeshMC metainfo

## MeshMC 7.8.0

### Highlights

* Major feature expansion across CLI, plugins, and Linux integration
* First-class Wayland support for better modern Linux compatibility
* New Filelink plugin enabling cross-instance file linking and desktop shortcut creation
* Improved distribution readiness with enhanced metainfo, branding, and Flathub presence

### Added

* CLI support for instance management and export functionality
* Filelink plugin for desktop shortcut creation and cross-instance file linking
* Wayland support for instance window management and improved Linux environment handling
* Callback-based action registration system for instance toolbar
* GA4 Measurement Protocol support with new API secret and measurement ID
* Branding colors for light and dark themes in metainfo.xml
* SVG asset for MeshMC graphical resources
* Documentation for Environment Variables and Application Settings APIs
* Snapshot management system via GenerateLatestJsonCommand and SnapshotService
* mmcmodules inclusion in Windows installer

### Changed

* Updated metainfo with new features, screenshots, and licensing information
* Refactored dialog window modality and improved layout constraints
* Reformatted minecraft subdirectory structure
* Replaced zlib with zlib-ng in Arch installation script
* Improved Linux environment handling and integration details

### Fixed

* Removed internal visibility check for zlib symbols to resolve LTO incompatibility issues

### Removed

* Internal zlib symbol visibility enforcement (due to incompatibility with LTO)

## MeshMC 7.7.0

### Fixed

* Updated _256 logo
* Fixed Java search dirs

## MeshMC 7.6.0

### Highlights

* Major cleanup of build system dependencies across macOS and cross-platform targets
* Reduced optional compression and crypto surface (LZ4, ZSTD, OpenSSL disabled)
* Improved licensing compliance coverage via REUSE updates
* Continued stabilization of cross-platform build pipeline (macOS, Windows, MinGW)

### Added

* macOS support for building libarchive from source
* tomlplusplus integration into the build process

### Changed

* Refactored macOS dependency setup and streamlined CMake flags
* Adjusted dependency management for macOS and Windows builds
* Updated REUSE configuration to expand license annotation coverage
* Simplified build configuration by disabling optional components (LZ4, ZSTD, OpenSSL)

### Fixed

* Build issues in dependency handling across macOS and Windows environments
* Inconsistencies in CMake configuration related to optional libraries

### Removed

* qrencode from macOS dependency installation
* vcpkg meson tool port and related patches

## MeshMC 7.5.0

### Highlights

* Improved Java management with **auto-download vendor selection** and refined settings handling
* Added **cross-platform dependency build scripts** (Linux & macOS & Windows)
* Internal improvements to **file watcher lifecycle and cleanup stability**

### Added

* Java auto-download **vendor selection support**
* Java-related **settings management system**
* Build scripts for dependencies on:

  * Linux
  * macOS
  * Windows

### Changed

* Refactored Java auto-download configuration logic
* Improved internal handling of Java settings
* Enhanced file watcher lifecycle management and cleanup behavior

### Fixed

* Potential issues related to file watcher cleanup and resource handling

## MeshMC 7.4.0

### Highlights

* Major build system stabilization across CMake, CI, and monorepo library handling
* Full Nix integration with derivations for all Project Tick components
* Transition toward system-provided dependencies and cleaner packaging model
* Improved library bundling strategy and source archive consistency
* Compression backend migrated from zlib to neozip

### Added

* Nix derivations for all Project Tick projects
* `default.nix` and `shell.nix` for flake compatibility
* CI steps for installing **cxxtest** on Linux and macOS
* Support for building and installing monorepo libraries (including neozip and vcpkg dependencies) in CI
* CMake support for collecting binary directories from `CMAKE_PREFIX_PATH`
* Additional CMake configuration for handling monorepo library bin paths
* `ldconfig` step to refresh dynamic linker cache for shared libraries
* README documentation for bundled libraries

### Changed

* Reworked GitHub Actions to support flexible library bundling and cleanup
* Refactored artifact naming in MeshMC workflow (removed Qt6 suffix)
* Updated CMake configuration to:

  * Use dynamic build types in CI workflows
  * Improve Release build handling
  * Resolve headers from installed system packages
  * Support multi-config generator mappings
* Dependencies are now assumed to be system-provided instead of bundled by default
* Library integration model updated to allow independent compilation of subprojects
* Replaced zlib with neozip for compression
* MeshMC source tree reformatted for consistency

### Fixed

* Issues with missing binary paths for monorepo libraries during build/link stages
* CI inconsistencies related to dependency installation and build configuration
* General CMake configuration inconsistencies affecting multi-platform builds

### Removed

* Legacy assumptions around bundled dependencies in favor of system-based linkage
* Redundant or conflicting commits (cleanup via targeted revert)

## MeshMC 7.3.0

### Highlights

* Introduction of the **MMCO plugin system**, establishing a formal extension architecture with a defined SDK and lifecycle. 
* Addition of the **MMCO Module Exception 1.0**, enabling non-GPL compatible plugins under controlled conditions. 
* First-party plugins introduced, including **BackupSystem** and **NVIDIAPrime**, demonstrating real-world MMCO usage. 
* Significant improvements to **plugin management, UI integration, and runtime behavior**. 
* Initial **fuzzing infrastructure** added for multiple core libraries (cmark, json4cpp, neozip, tomlplusplus). 

### Added

* MMCO plugin system (SDK, loader, lifecycle management, metadata, hooks) 
* MeshMC MMCO Module Exception 1.0 and updated licensing across the codebase 
* Plugin SDK CMake integration and example plugin 
* BackupSystem plugin for instance backup management (initial implementation) 
* Pre-launch backup support and settings integration for BackupSystem 
* NVIDIAPrime plugin for NVIDIA GPU offloading support 
* Plugin launch modifiers and improved plugin management system
* Plugin metadata enhancements (code links, About dialog integration) 
* Plugin build and staging configuration options
* Backup-related UI assets (icons across themes)

### Changed

* Core architecture transitioned to a **plugin-first model with MMCO as a first-class subsystem** 
* Licensing model updated from plain GPL-3.0-or-later to **GPL-3.0-or-later WITH MMCO exception** 
* Plugin tab location updated in the UI 
* Backup plugin UI labels and messaging improved 
* Help menu actions refactored in MainWindow 
* LoggedProcess and ModFolderPage enhanced to support runtime resource/shader pack addition 
* CMake configuration updated for SDK and plugin staging 
* Platform compatibility improvements (conditional `unistd.h`) 

### Fixed

* Fixed launcher icon issues 
* Improved macOS icon rendering (Tahoe compatibility) 
* REUSE compliance issues resolved 
* General formatting and minor internal fixes 

### Removed

* Removed parallel execution in CI test commands for stability

## MeshMC 7.2.0

### Highlights

* Added a full **crash reporting and log management system**
* Improved the **update pipeline**, including feed parsing, version comparison, and updater integration
* Strengthened **error handling** during update checks and archive extraction
* Cleaned up launcher internals, compiler flags, and several code paths for clarity and stability
* Added initial **Pacman packaging**
* Introduced **AUR automation** through GitHub Actions
* Improved packaging directory structure and release patching workflow

### Added

* Added a new **crash reporter** application and dialog flow for handling launcher crashes
* Added **CrashReportDialog** with QR code support for full logs and paste.ee links
* Added **MeshMCLogsDialog** for viewing and managing MeshMC logs
* Added support for **log uploading to paste.ee**
* Added **UpdateProgressDialog** to show updater progress and logs
* Added `MESHMC_BINARY` to `BuildConfig` for application binary naming
* Added a shell wrapper for the `meshmc` binary in portable tarball installations
* Added unit tests for `UpdateChecker`
* Added **Pacman package** support
* Added packaging directory placeholders with `.gitkeep`
* Added `.gitignore` for Pacman packaging outputs
* Added a GitHub Actions workflow for **automatic AUR package updates** for MeshMC

### Changed

* Refactored version comparison in `UpdateChecker` to use `qint64` for better accuracy
* Refactored installer behavior to store downloaded archives in the install root
* Refactored stable feed item parsing in the update system
* Refactored process management in `ModernLauncher`
* Improved applet instantiation in `OneSixLauncher`
* Refactored `getKernelInfo` to use `QOperatingSystemVersion`
* Updated Java-related compile flags to use `--release`
* Adjusted CMake CXX flags by removing redundant warning flags for MSVC and refining non-MSVC flags
* Enhanced compiler flags for both MSVC and Unix builds to improve warning coverage and stricter handling
* Refactored multiple methods, parameters, and internal declarations for improved clarity and maintainability
* Reformatted MeshMC source code
* Updated the release workflow patching process by removing an unnecessary copy step

### Fixed

* Fixed updater binary name references in `UpdateController`
* Added error handling for update check failures in `MainWindow`
* Improved extraction error handling in the installer
* Fixed type casting issues in several areas, including:

  * `rowCount`
  * `JsonFormat.cpp`
  * `VersionList::count`
* Fixed warnings in integrated `libnbtplusplus` code
* Improved packaging-related workflow structure for release automation

### Removed

* Removed unused MeshMC documentation
* Removed unused helper functions and variables in several source files
* Removed unused parameters across multiple methods to simplify interfaces
* Removed unnecessary packaging-related copy behavior from the release workflow

## MeshMC 7.1.0

### Highlights

* Added **in-launcher installation support** for mods, resource packs, and shader packs with a dependency solver
* Improved **update system and release pipeline**, including checksum generation and updater handling
* Completed large-scale **Qt6 signal-slot modernization refactor**
* General **build system, CI, and documentation cleanup**

### Added

* In-launcher installation support for mods, resource packs, and shader packs
* Dependency solver for resolving content dependencies
* SHA-256 checksum generation in release workflow
* Updater binary discovery next to the running executable
* Improved portable mode detection logic

### Changed

* Refactored update mechanism and updater execution flow
* Updated version handling to prioritize release tags
* Migrated signal-slot connections to modern Qt syntax across the codebase
* Updated CMake configuration, including Qt version constraints

### Removed

* Removed obsolete MeshMC man page documentation
* Cleaned up unused files
* Dropped `qt5compat` dependency
* Removed unused `libnbtplusplus` inputs from flake configuration
* Removed CI tag trigger

## MeshMC 7.0.1

### Fixed

* Fixed build timestamp not being set in generated files due to CMake configure order

## MeshMC 7.0.0

Today, we aren't just talking about ~~MultiMC~~. This is not true. ~~PrismLauncher~~?
Again no. Okay, ~~ProjTLauncher~~? Again and again, no. This is **MeshMC** 7.0.0. This
release ports the project from Qt5 to Qt6, migrates from the old CurseForge API to the
new one, adds Modrinth support, and more!

### Welcome to customizable Catpacks

Catpacks are now customizable. To build your own cat empire, you can add your catpacks
to the catpacks folder located in the binary folder within `%AppData%\MeshMC` or
`.local/share/MeshMC`, or if you are using a portable binary.

### Modrinth support, now available

Now you'll be able to install Modrinth packages, and I hope this will make everyone happy.

### Say hello to a launcher that includes Qt6

MeshMC 7.0.0 has completed the Qt6 migration by default, which is especially important
for our future-proofing and for you to use the launcher more comfortably. If you encounter
a bug or problem, please start an issue using <https://github.com/Project-Tick/MeshMC/issues>.
Because this migration can cause tons of problems.

### Other Changes

- Download your Java applications easily through your launcher with **JavaDownloader**.
- **Neoforge** and **Quilt** support has been added. You can quickly install **Neoforge** and **Quilt** from the instance edit menu.
- The **libnbt++** module now comes with meticulously crafted patches from **Project Tick**.
- We are now sharing our **Client IDs** so that developers and users can get custom builds.
- Added more themes and icons.
- Now you can log in to your Microsoft account more easily with endpoint.
- Refined AboutPage.
- Changed Quazip dependency to libarchive
- Fixed more bugs.

### Note

MeshMC is a continuation of MultiMC. We've implemented AI Usage Policies and GPL
transitions to make MeshMC more free, and you can be sure our code is more open to use.
Our difference is that we aim to create a Minecraft launcher that people can compile even
20 years from now, freely package wherever they want, maintain the simplicity and iconic
appeal of MultiMC, while also adding new features and always keeping that line. We are very
happy to see you with this release, but we also want to guarantee that the problems that
befell ProjT Launcher will not occur here. The reasons we abandoned ProjT Launcher were
primarily licensing issues and problems with the codebase due to it being a PrismLauncher
fork, but we want to preserve the MultiMC foundation in MeshMC. Therefore, we shut down
ProjT Launcher. Thank you for your support. **Stay with your cats.**
