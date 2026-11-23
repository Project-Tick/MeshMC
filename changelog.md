# MeshMC 12.0

## Highlights

Hello everyone! It’s been a little while not too long, but some time has passed.
MeshMC is starting fresh! We’ve caught up to the latest point in the MultiMC
codebase; we are now fully synced with the most recent commit. Furthermore, we’ve
overhauled the update system. From now on, I’ll only be posting "highlights" here,
as we’ve shifted to a continuous update model meaning we are now fully integrated
with MultiMC’s own updater. It’s a bit wild, but that’s how it is! You can view
every individual change directly within the launcher. I’ll only be posting these
kinds of tags for stable releases, distribution-specific builds, and platforms
like Flathub. Please, if possible, don’t expect any more tags from me in the
future. Rest assured, I’m working on speeding up the Flathub deployment process.
Also, a quick note: this version is more about the new underlying foundation,
which means some features may have been lost in the transition. You’re all
the best take care, and give your cats some love!

## Previous versions

## MeshMC 11.0.2

## CI Removes

* Removed GHA MinGW Qt5 Clang64 CI.

## MeshMC 11.0.1

### CI Fixes

* Fixed GHA MinGW Clang64 CI.

## MeshMC 11.0.0

### Highlights

* We’ve made some general improvements!
* We hope MeshMC now offers a better experience for you.

### Added

* Automatic icon refreshing upon theme change has been added.
* Support for reading non-ASCII paths has been added.
* A feature to view the number of days in your worlds has been added.
* The --world parameter has been added.
* A skin management system has been added.
* An option has been added to launch the instance using other accounts via the "Launch as" option.

### Changed

* The logging system has been updated. We now use categorical logging. You can disable unwanted categories using qtlogging.ini.
* We have revised the update system. Updates are handled via Sparkle for macOS and ZSync for AppImage. A setup.exe installation package is used for Windows, while a two-stage update mechanism is employed for portable Windows and Linux versions.
* The MainWindow used to have its UI drawn via .cpp code; now, Designer UI is used.
* The XDGIcon structure has been removed, and the system has switched to the internal Qt icon infrastructure.
* The logo has been revised to better align with standards for macOS Big Sur and other platforms.
* Deep branding has been completed.
* The URL on the translation page has been corrected.

### Fixed

* Fixed buttons that remained active even when no instance was selected in the MainWindow.
* An issue where the Publisher ID written to the registry during NSIS installations appeared as "MeshMC Contributors" has been resolved; it has been corrected to "Project Tick".

### Removed

* The iconfix library has been removed because it is no longer in use.

## MeshMC 10.1.1

### CI Fixes

* Fixed GHA MinGW Clang64 CI.

## MeshMC 10.1.0

### Changed

* The URL on the translation page has been corrected.

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

## MeshMC 9.2.1

### CI Fixes

* Fixed GHA MinGW Clang64 CI.

## MeshMC 9.2.0

### Changed

* The URL on the translation page has been corrected.

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

## MultiMC 0.6.16

This brings some good and bad changes to modpack platforms, along with various fixes.

We've added Modrinth as a directly supported modpack platform. It's nice, and the list of packs available from it is slowly growing.

We can no longer directly import FTB modpacks from the FTB platform. Instead, this has been replaced by import from the [FTB App](https://www.feed-the-beast.com/app). You can get FTB modpacks through the FTB App and then import them into MultiMC using the `FTB App Import` page in `Add Instance`.

We can also no longer directly download CurseForge modpacks, although this should be only temporary (once the issues we have with the CurseForge API ToS are resolved, it will make a comeback). Currently, you can get CurseForge packs using the `FTB App Import`, just make sure you switch the FTB App to the CurseForge mode. The button for that isn't too obvious, look for the CurseForge logo.

Aside from these changes, the release brings some general fixes.

#### Changes, itemized.

- Modrinth platform has been added, along with importing `.mrpack` files.
- FTB App Import has been added as a way to automate copying over modpacks installed using the FTB App.
- Curseforge platform and importing `.zip` files from the platform has been removed (replaced by FTB App Import).
- FTB platform has been removed (replaced by FTB App Import).
- MultiMC is now blocking passing the `-version` argument to the Java Runtime. This should stop some really strange (unaccounted for) things from happening.
- GH-3742: Added Quilt modloader installation button and support for importing Quilt-using modpacks in general.
- When the game crashes on Windows, MultiMC gives you a nicer representation of what the exit code could mean.
- Some odd corner cases when importing ATLauncher modpacks are now handled better.
- Custom player name for offline mode is now saved for your convenience.
- GH-3012: Added command line options to launch instances offline.

    Used like this:
    ```
    ./MultiMC --launch 1.17.1 --offline --name Steve
    ```

## MultiMC 0.6.15

This is mostly a bugfix release.

#### Accounts

- Scan for changes to Minecraft entitlements on account refresh (to make sure we detect changes to entitlements).
- Add some more logging to interaction with Minecraft services and authentication (profile fetching, etc.).
- Add support for demo mode for accounts that don't have a Minecraft profile.

#### UI

- Fixed how 'español de Latinoamérica' appears in the language selector.
- Added a way to open loader mods for an instance from the main window.
- Improved error messages when the data folder cannot be accessed correctly.
- Fixed window title of the Java checker failure dialog.
- Updated layout of various modpack import pages to be more consistent and better in general.

#### Java

- GH-4000: add detection for Adoptium Java runtimes.
- Changed minimum Java version to 7 - it was too hard to maintain compatibility with 6 in development environments.
- GH-4125: Add a workaround for Java interacting badly with some features of Bedrock Linux.

#### Screenshots

- GH-4299: Fix crash when uploading screenshots.
- Added the capability to copypaste screenshots as raw images (to image editors) and files (to file explorers).

## MultiMC 0.6.14

This further refines Microsoft account support, along with small fixes related to modpack platforms and Java runtime detection.

It's also been 10 years since the first release of MultiMC. All background cats are now ready to party!

#### Microsoft accounts

The account system now refreshes accounts in the background while the application is running.

- GH-4071: Errors encountered while refreshing account tokens no longer always result in the tokens expiring:
  - Network errors encountered when refreshing the main account tokens result in the account being **Offline**.
  - **Hard** errors are produced by the main tokens becoming provably invalid.
  - Errors encountered later are treated as **Soft** - they do make the account unusable, but still recoverable by trying again.
  - **Soft** errors are treated as **Hard** errors when adding the account initially.

In general, this should make MultiMC much more forgiving towards various temporary and non-fatal errors.

- GH-4217: Added support for GamePass accounts and Minecraft profile setup:
  - The new endpoint for logging in with Microsoft is now used (`/launcher/login`), enabling compatibility with GamePass.
  - Game ownership is checked instead of only relying on Minecraft profile presence.
  - Accounts can now be added even when they do not have a profile.
  - The launcher should guide you through selecting a Minecraft name if you don't have one yet.

#### Modpack platform changes

- GH-4055: MultiMC now tries to avoid downloading multiple files to the same path for FTB modpacks.

- Search as you type is now used for FTB.

- GH-4185: Version of the modpack is now included in the name of the instance by default.

- The modpack platform UIs now include text field clear buttons.

#### Other changes

- Adjusted warnings about Java runtime required for Minecraft 1.18 (it's not Java 16, it's Java 17).

- GH-3490: Instance sorting is now aware of numbers (and sorts 99 before 100).

- GH-4164: Reimplemented assigning instances to groups using drag & drop.

- GH-1795: Added terminal launch option to use a specific Minecraft profile (in-game player name).

    Used like this:
    ```
    ./MultiMC --launch 1.17.1 --profile MultiMCTest --server mc.hypixel.net
    ```

- GH-4227: Fix crash related to invalid Forge mod metadata.

- GH-4200: Search for the *Eclipse Foundation* and *Adoptium* Java runtimes in the Windows Registry.

- Added shader packs page to instances.

- Removed Mojang services status information from the main window - the status is no longer provided by Mojang.

- It is now possible to turn of global tracking of play time.

#### Technical changes

- Debranding is mostly finished. You may see some changes in the logo being used in less places.

## MultiMC 0.6.13

This release brings initial support for Microsoft accounts, along with a nice pile of modpack platform support changes and improved Java runtime detection.

Java runtimes still need an overhaul, so we're staying on the 0.6 version for a little longer.

Next release should also tackle the current Forge 1.17.x issues in a systematic way.

#### Microsoft accounts

This is the first release with Microsoft accounts in.

Implementation is loosely based on documentation available from [wiki.vg](https://wiki.vg/Microsoft_Authentication_Scheme) with some notable changes:

- More complete implementation including getting and displaying GamerTags [(see XR-046)](https://docs.microsoft.com/en-us/gaming/gdk/_content/gc/policies/pc/live-policies-pc#xr-046-display-name-and-gamerpic-).

- Using the OAuth Device Flow instead of closely integrating with a browser engine.

    MultiMC asks you to open a Microsoft login web page and put in a code that lets MultiMC authenticate.

    This lets you authenticate on a completely separate device like your phone, leaving code we ship and the computer you may not even trust out of the picture.

As part of this, the skin fetching no longer uses a third party service and instead gets skins directly from Mojang.

Capes can also be selected in MultiMC now. With how many people will now get one for migrating their accounts, it only makes sense.

#### macOS update

Because of issues with the Microsoft accounts, we now have two builds on macOS:

- The old build with Qt 5.6 that does not work with Microsoft accounts, but can run on macOS older than 10.13.

- A new build with Qt 5.15.2 that does work with Microsoft accounts, can use the new macOS dark theme and highlight colors, but requires at least macOS 10.13.

MultiMC will update to the 5.15.2 builds when it detects that this is possible. **It may look like it is updating twice, just let it do its thing.**

Similar approach got attempted on Windows, aiming to fix various display scaling and theming issues, but it ran into too many problems and will be attempted later, with more caution.

#### Modpack platforms

In general, the modpack platform pages have been made more consistent with each other (GH-3118, GH-3720, GH-3731).

- FTB improvements:

    - Modpack file downloads are now checked with checksums and cached.

    - GH-1949: Allow Legacy FTB and FTB pack downloads to be aborted.

- CurseForge improvements:

    - CurseForge modpack platform is now presented as CurseForge, not Twitch.

    - UI has been updated to match other platforms

    - Added sorting

    - GH-3667: Added version selection

    - GH-3611: Added ability to install beta versions

    - GH-3633: When a CurseForge pack is available for multiple Minecraft versions, we assume the latest one.

- ATLauncher improvements:

    - Handling latest/custom/recommended mod loader versions.

    - Fabric loader packs should now work.

    - GH-3764: Only client mods are installed now for ATL packs.

    - Improved error handling

    - Optional mods are supported.

    - GH-1949: Allow ATLauncher pack downloads to be aborted


- Fixed bugs in FTB platform search.

#### Other changes

- Forge installation is disabled on Minecraft 1.17+ because of incompatible/unresolved changes on the Forge side.

    We're going to aim for fixing it in time for 1.18. Thankfully, 1.17 is more of a in-between release, so go play some 1.16.x packs!

- GH-2529: On macOS, MultiMC will ask to move all the instance data to a new `Data` folder in order to fix long load times caused by macOS checking all files.

- Detection of a large amount of various Java runtime flavors have been added.

- It is now possible to join servers when starting an instance:

    - From command line via the `--launch` and `--server` arguments.

    - Or by setting this up in the instance settings page.

    This may not work correctly in some cases, because it is a rarely used feature and modders do not test with it.

- MultiMC now prints resolved IP addresses of Minecraft services into the game log for diagnostic purposes.

- Updated instance icons based on Minecraft textures.

- Forge `mods.toml` files are now used for displaying mods in the UI.

- Datapack button is now disabled when no world is selected.

- MultiMC warns about GLFW and OpenAL workarounds being enabled in the game log.

- Languages in the translations list are now sorted by their two/three letter key

- GH-3450: Displaying and recording gameplay time is now optional and can be turned off.

- GH-3930: MultiMC can now track the gameplay time of the last session.

- GH-3033: The version pages of instances now have a filter bar.

- GH-2971: UI descriptions of texture and resource packs no longer mention mods.

- Quick and dirty minimum Java runtime versions checks have been added. This needs to be expanded in the future.

#### Technical changes

- The codebase continues to move towards being debranded and harder to build as 'MultiMC' for third parties.


## MultiMC 0.6.12

After roughly one year of maintenance and development work by various contributors, we're just calling it a good time to release.

What got added since the last time? Quite a bit! But in general, this is more of a spring cleaning before the major changes that we need to make come in.

#### Modpack platforms

We've added a whole bunch of new modpack platforms to pick from right into the new instance dialog. If you run into any unusual issues with the imported packs, report them on the bug tracker.

- Added a CurseForge pack browser

- GH-3095: Added an FTB pack browser

  Temporarily, MultiMC ignores download failures for FTB packs (GH-3304). This is because the platform has consistency issues.

- GH-469: Added a Technic/Solder pack browser

- GH-405: Added a ATLauncher pack browser

#### Other changes

- Added the option to not use OpenAL and/or GLFW libraries bundled with the game.

  This is interesting if you have ones that come with your system and work better.

- Skins (the part used for account icons) are now rendered with the overlay on.

- GH-3130: Skin upload has been switched over to the new Mojang API and should have less issues.

- MultiMC now shows world icons and allows resetting world icons in `View Worlds`.

- GH-3229: Copy seed button has been updated to be compatible with newer versions of the game.

- GH-3427: `View Worlds` now has a very simple `Datapacks` button - it just opens the system file browser.

- GH-3189: Updated nbt library - this makes `View Worlds` work properly again for newer versions of the game.

- Fixed online saving in Classic versions.

- GH-3131: Fixed not working with proxy ports greater than 32767.

- Proxy login details are no longer logged in files.

- GH-3467: The launch could stall in the ScanModFolders task if the mod folders didn't exist yet.

- GH-3602: Pre-launch commands could fail on first launch of the instance because the .minecraft folder has not been created yet.

#### Technical changes

- GH-3234: At build time, the meta URL can be changed.

- Removed some hacks previously required to get Forge working

  MultiMC no longer contains pack200 and the custom lzma format support used by Forge only.

- Some preparations have been done to allow downloading Java runtimes from Mojang - support for the Piston repository.

- Compatibility with unusual build environments has been increased

## MultiMC 0.6.11

This adds Forge 1.13+ support using [ForgeWrapper](https://github.com/ZekerZhayard/ForgeWrapper) by ZekerZhayard.

#### New or changed features

- GH-2988: You can now import instances and curse modpacks from the command line with the `--import` option followed by either an URL or a local file path.

- GH-2544: MultiMC now supports downloading library files without including them on the Java classpath.

  This is done by adding them to the `mavenFiles` list instead of the `libraries` list.

  Such downloads are not deduplicated or version upgraded like libraries are.

  This enables ForgeWrapper to work - MultiMC downloads all the files, and ForgeWrapper runs the Forge installer during instance start when needed.

## MultiMC 0.6.8

This is mostly about removal of the 'curse URL' related features, because they were of low quality and generally unreliable.

There are some bug fixes included.

MultiMC also migrated to a new continuous deployment system, which makes everything that much smoother.

### New or changed features

- GH-852: Instance group expansion status now saves/loads as expected.

- The bees have invaded the launcher. We now have a bee icon.

- Translations have been overhauled, yet again...

    - We now have a [crowdin site](https://translate.multimc.org/) for all the translation work.

    - Translations are made based on the development version, and for the development version.

    - Many strings have been tweaked to make translating the application easier.

    - When selecting languages, European Portuguese is now displaying properly.

- Accessibility has been further improved - the main window reads as `MultiMC`, not a long string of nonsensical version numbers, when announced by a screen reader.

- Removed the unimplemented Technic page from instance creation dialog.

- GH-2859: Broken twitch URL import method was removed.

- GH-2819: Filter bar in mod lists now also works with descriptions and author lists.

- GH-2832: Version page now has buttons for opening the Minecraft and internal libraries folders of the instance.

- GH-2769: When copying an instance, there's now an option to keep or remove the total play time from the copy.

### Bugfixes

- GH-2880: Clicking the service status indicators now opens a valid site again, instead of going nowhere.

- GH-2853: When collapsing groups in instance view, the action no longer becomes 'sticky' and doesn't apply to items clicked afterwards.

- GH-2787: "Download All" button works again.

- When a component is customized, the launcher will not try to update it in an infinite loop when something else requires a different version.


## MultiMC 0.6.7

The previous release introduced some extra buttons that made the instance window way too big for some displays. This release is aimed at fixing that, along with other UI and performance improvements.

There are some accessibility fixes thrown in too.

### New or changed features

- Mod lists are now asynchronous and heavily threaded.

  Basically, both faster and more responsive.

  The changes necessary for this also pave the way for having other sources of mod metadata, and for adding more mod-related features support in general.

- Mod list printed in log has been improved.

  It now also shows disabled mods, and has prefix and suffix that shows if the mod is enabled, and if it is a folder.

- You can now enable and disable mods with the keyboard.

  Toggle with enter.

- Enabling and disabling mods no longer makes the list forget what was selected.

- GH-358: Switched all the dialog pages from using buttons in layouts to toolbars.

  Toolbar buttons are smaller, and the toolbars can overflow buttons into an overflow space. This allows requiring a lot less space for the windows.

  All of the relevant pages now also have context menus to offset the issues toolbars create when using screen readers.

- Main window instance list is now compatible with screen readers.

  If you have poor or no eyesight, this makes MultiMC usable.

- More instance pages are now visible when the instance is running.

  Mods, version and the like should now be visible, but most of the controls are disabled until the game closes.

- GH-2550, GH-2722, GH-2762: Mod list sorting is much improved.

  You can now sort mods by enabled status.

  Sorting by version actually looks at the versions as versions, not words.

  Sorting by name ignores 'The' prefixes in mod names. For example, 'The Betweenlands' will be sorted as 'Betweenlands'.

  Sorting cascades from 'Enabled' to 'Name' and then 'Version'. This means that if you sort 'Enabled', the enabled and disabled mods are still sorted
  by name and mods with the same name will be also sorted by version.

## MultiMC 0.6.6

This release is mostly the smaller things that have accumulated over time, along with a big change in linux packaging.

No 1.13+ Forge news yet. That's going to be a major overhaul of many of the internals of MultiMC.

### **IMPORTANT**

On linux, MultiMC no longer bundles the Qt libraries. This fixes many issues, but it might not run after the update unless you have the required libraries installed.

Make sure you have the following packages before you update:

- Arch: `qt5-base`
- Debian/Ubuntu: `qt5-default`
- CentOS/RHEL/Fedora: `qt5-qtbase-gui`
- Suse: `libqt5-qtbase`

MultiMC on linux is built with Qt 5.4 and older versions of Qt will not work.

This should be a massive improvement to system integration on linux and resolves GH-1784, GH-2605, GH-1979, GH-2271, GH-1992, GH-1816 and their many duplicates.

### New or changed features

- GH-2487: No is now the default button when deleting instances.

- It is now possible to launch with profilers in offline mode.

- Massively improved support for icon formats when importing and exporting instances.

  All of the formats MultiMC supports are now supported in exported instances too, instead of just PNG.

- Added the pocket fox icon.

  We still have the big one under the staircase. It's cute. Just hide your chickens.

- Global settings can be opened from instance settings where appropriate.

  Many people use the instance overrides where using the global settings would be more appropriate. Hopefully this makes it clearer that the instance settings are overrides for the global settings.

- Added direct Fabric loader support.

  Much overdue. It's good. Fabric mod metadata is also supported in the mod pages.

- MultiMC now recognizes the new `experimental` Minecraft versions.

  Go mess with the combat experiment. It's interesting.

- Added Twitch URL as an option to the Add Instance dialog.

  You can now drag the purple download buttons from CurseForge into MultiMC and get a modpack out of it. Much easier!

### Bugfixes

- Translation folder is now created sooner, making first launch translation fetch work again.

- GH-2716: MultiMC will no longer try to censor values shorter than 4 characters in logs.

  It was actually leaking information and destroying the logs instead of helping.

- GH-2551: Trim server name and IP before saving them.

- GH-2591: Fix multiple potential memory leaks and crashes related to destroying objects with Qt memory lifecycle model.

- `run.sh` on linux now passes all arguments to MultiMC.

- Adding a disabled mod duplicate now replaces the existing mod.

- GH-2592: Newly created instances are now selected again. This was a very old regression.

- GH-689: MultiMC no longer creates an imgur album for single screenshot uploads.

- GH-1813: `#` is now saved properly when used in instance notes.

- GH-2515: Deleting an instance externally while the delete dialog is open no longer leads to some other instance being deleted when you click OK.

- GH-2499: Proxy settings are applied immediately and no longer need an application restart.

- GH-1701: When downloading updates, the text now reflects the number of downloaded files better.

- Icon scaling issues on macOS should now be fixed.

## MultiMC 0.6.5

Finalizing the translation workflow improvements and adding fixes for sounds missing in old game versions.

### New or changed features

- UI for the language settings has been unified across the application

- GH-2209: Sounds in old (pre-1.6) versions should now work again

  The launcher now downloads the correct assets and reconstructs the `resources` folder inside instances. This mirrors the same fix implemented in vanilla.

  Also, a minor issue with the reconstruction being done twice per launch has been fixed.


## MultiMC 0.6.4

Update for a better translation workflow, and new FTB API location.

### New or changed features

- FTB API location has changed

  MultiMC now uses the new location and should keep working.

- Translations have been overhauled, again

  It is now possible to put the translation source `.po` files into the `translations` folder and see changes in MultiMC immediately.

  The new translation workflow is like this:
  * Get a `.po` file from here the [translations repository](https://github.com/MultiMC/MultiMC5-translate).
  * Alternatively, get the `template.pot` and start a new translation based on it.
  * Put it in the `translations` folder.
  * Edit it with [POEdit](https://poedit.net/).
  * See the changes in real time.
  * When done, post the changed files on discord, or github.

  When using a `.po` file, MultiMC logs which strings are missing from the translation on the currently displayed UI screen(s), and which one are marked as fuzzy. This should make it easy to determine what's important.

## MultiMC 0.6.3

This is a release mostly aimed at getting all the small changes and fixes out of the door.

### Potentially breaking changes

- Local libraries are only loaded from inside the instances now.

  Before, MultiMC allowed loading local libraries from the main `libraries` folder.
  This in turn allowed existence of instances which could not be transported from one installation of MultiMC to another.

  GH-2475: A bug that allowed the launch to continue with missing local libraries has also been fixed.

  Effectively, you will get errors from launching such instances. You can fix the errors by copying the libraries to the locations indicated in the error log.

### New or changed features

- FTB import now has support for third party modpack codes.

  Better late than never?

- Instance creation can now be interrupted / aborted.

- GH-2053: You can now inspect and change the `servers.dat` file from MultiMC.

- MultiMC now uses the https protocol for many more network requests.

- GH-2352: There is now a button to open the `.minecraft` folder inside the selected instance.

- GH-2232: MultiMC can now use `.gif` icons (not animated).

- GH-2101: Instance renaming is now done inline, in the actual instance list.

- GH-2452: When deleting a group, MultiMC asks for confirmation.

- GH-1552: PermGen is no longer shown when it's not appropriate (java 8 and up).

- GH-2144: When changing versions of a component like Forge, the current version is marked with `(installed)`.

- GH-2374: World list has been improved:

  - Alternating line background colors have been added.
  - The world game type is now shown in a column.

- GH-2384: When installing a mod, existing mod with the same file name will be replaced.

- The background cat sometimes wears a silly hat.

### Bugfixes

- GH-2252: Fixed odd drag and drop behaviour on Windows

  Drag and drop of URLs from a browser locked up the browser. This needs further fixes on macOS.

- Instance naming fixes:

  - GH-2355: Whitespace prefix or suffix is no longer allowed.
  - GH-2238: Newlines in instance names are no longer allowed either.

- GH-2412: MultiMC no longer leaves behind zombie processes after launch on linux.

- GH-2382: Version filter for the forge/liteloader version lists was not matching the whole version name.

- GH-2488: More issues with broken relative URL redirection in Qt have been fixed.

- Some memory leaks of downloaded data have been fixed.

- MultiMC now handles instance groups and instance group saving better.

  Long deleted groups no longer persist in the group list.

- GH-2467: Broken (and nonsensical) sorting indicators have been removed from the versions page header.

## MultiMC 0.6.2

### New or changed features

- MultiMC now has FTB integration:

  - Official and third-party modpacks work.
  - Codes for private modpacks are not implemented yet.

- Version lists now show release dates where available.

- New instance dialog:

  - It has been completely overhauled and now uses the same kind of paged dialog design as the rest of MultiMC.
  - Vanilla version list now has a filter for version types.
  - FTB integration gets a page here, along with zip import and vanilla.
  - Technic integration is a definite possibility in the future.
  - If there is a decent way to list Twitch modpacks, proper Twitch modpack integration is a possibility too.
  - There still is no modpack updating. Much more work is needed for that.

- Other Logs page:

  - Now has a search bar, just like the main log page.
  - GH-604: Uses the same font settings as the main log.

- Icon selection dialog now has a button for opening the icons folder.
- MultiMC now has a shinier, updated logo.
- GH-2150: Custom commands have been split from the java settings into a new page.

  The use of variables in custom commands is now better documented.
  The label shows that they need to be prefixed by `$`.

- Player name is no longer censored in logs.
- MultiMC now probes the system for the name of the linux distribution as part of analytics. This will be used to focus future packaging efforts.
- Secret cheat code has been added... What does it do?

### Bugfixes

- VisualVM integration now works when VisualVM is bundled inside the MultiMC folder (uses a relative path).
- When reinstalling a component, or changing a component version, the custom version is now removed first.
- GH-2134: Fix multiple issues with the skin upload:

  - When uploading a skin, the model selection now works correctly again.
  - When the new skin file is specified using the `file://` URL scheme, it will now work correctly.

- GH-2143: Mojang services status display now reflects the current set of services.
- GH-2154: MultiMC now ignores the `hidden` flag of instance folders and they should show up correctly.
- When migrating Legacy instances, custom `minecraft.jar` will be preserved.

## MultiMC 0.6.1

### New or changed features

- GH-2087: The version page now has a button that will download all the necessary files without launching the game.

### Bugfixes

- Several issues related to bad URLs returned by the Curse servers have been fixed.

  The Curse platform does not use valid URLs according to [RFC 3986, section 2](https://tools.ietf.org/html/rfc3986#section-2) by including spaces and UTF-8 characters without percent encoding them.
  MultiMC has been improved to handle these invalid URLs and report errors in case other invalid URLs are encountered.
  This affected pretty much all modpack imports. You may want to reimport them if you were affected by this.

- GH-1780, GH-2102, GH-2103: Multiple issues with the build system and packaging on linux have been fixed.

  - Installed libraries now no longer have `RPATH` set and have the correct file permissions when using the `lin-system` layout.
  - Installation using the `lin-bundle` layout has been fixed on platforms that use position independent code.
  - `CMAKE_INSTALL_PREFIX` and `DESTDIR` now behave as expected on linux platforms.

- MultiMC no longer logs the process environment and launch scripts to its log files.

- GH-2089: Mention of instance tracking has been removed from the deletion confirmation dialog.

- GH-2087: The obsolete 'revert to vanilla' logic that was previously applied to versions has been removed.

  This should remove some confusing situations that could happen while changing and manipulation instance versions.

- The temporary `Minecraft.jar` is now removed from the instance after it stops running.

- GH-2119: The main instance view scrollbar now correctly updates when the window is resized without changing the number of icons that can fit into it horizontally.

## MultiMC 0.6.0

### New or changed features

- Contact with Mojang, Forge and LiteLoader servers is no longer handled by MultiMC, but a metadata server. Instead of generating and storing the files at the point of installation, they are updated hourly on the server and can be fixed when something goes wrong.

  This goes along with some changes to the instance format and to the metadata format.

  Instead of including the metadata JSON files directly in the instances, the instances now contain a new `mmc-pack.json` file that specifies versions to be used.

  The metadata can be found at [v1.meta.multimc.org](https://v1.meta.multimc.org), the [meta.multimc.org](https://meta.multimc.org) endpoint that was used during development will be replaced by documentation.

  This should be a much more reliable solution going forward, because it allows fixing issues without releasing new versions of MultiMC or reinstalling Forge/LiteLoader/others.

- Tracking of FTB launcher instances has been replaced with direct import of Curse modpacks.

  You can import the modpack zip files from CurseForge and FTB:
  - Get the zip, for example from [here](https://www.feed-the-beast.com/projects/ftb-retro-ssp/files/2219693).
  - Drag & Drop it on top of the main window, or select it in the new instance dialog.
  - Let the magic happen.

  If you need help moving over your old instances or worlds from the FTB launcher, stop by in the MultiMC discord server.

  The Curse import functionality is there thanks to the work [@Dries007](https://twitter.com/driesk007) and [@NikkiAI](https://twitter.com/NikkyAI) have done on [CurseMeta](https://cursemeta.dries007.net/).

- GH-1314: MultiMC now allows replacing the main jar in an instance without having to mod the Mojang jars.

  This goes along with changing the wording of the jar mod button to make it clear that it adds files to the main Minecraft jar instead of installing mod files with the `.jar` extension.

- Because the current instance format can now handle replacing the main jar, Legacy format instances are no longer directly supported.

  Instead of launching, you will be prompted to convert them to the current instance format.
  If the automated process fails, stop by in the MultiMC discord server and ask for help.

- Main window UI has been changed for increased clarity.

  Many people had issues finding the settings and instead ended up using the per-instance overrides. The main toolbar now has labels and the per-instance overrides have been deemphasized by removing the direct path to them from the main window. In general, it should be easier to find the right settings menu without getting things completely wrong on the first try.

- GH-1997: MultiMC now supports Java 9.

  This does not mean that the current mod loaders and mods do, but you can run Vanilla Minecraft with Java 9 now.

  However, Java 9 will come up last in the lists when multiple versions are installed and its use is strongly discouraged.

- GH-2026: You can launch Minecraft 1.13 snapshots - and hopefully also 1.13 once it is released.

  The bare minimum of changes needed for 1.13 to launch has been done.

  This does not mean support for modded 1.13!

  It is not yet clear what it will even look like and what exactly will be needed for Forge to be able to install properly.

- Bundled Qt libraries have been updated to version 5.6.3 on macOS and Windows

  This means less issues with SSL encryption on macOS and better support for HiDPI/retina displays, along with many bug fixes.
  The workarounds for SSL problems on macOS have been removed thanks to this.

- Linux builds were moved to a newer version of Ubuntu (14.04)

  This means better support on newer distribution releases, and dropping support for older distributions.

- Bundled OpenSSL library on Windows no longer requires Visual Studio runtime libraries.

  This should avoid issues with missing runtime libraries.

- GH-1855: The instance window now has an offline launch button.

- GH-1886: UI now clarifies that MultiMC proxy settings do not apply to the game.

- It is now possible to package MultiMC on linux without hacks.

  The build system has a concept of 'install layouts'. Example Arch linux package that uses this (multimc-git) is [available in the AUR](https://aur.archlinux.org/packages/multimc-git).

- Wrapper commands now support arguments.

  Previously, they would be treated as a single command -- spaces and all.

- UI elements that set maximum JVM memory are now limited to the amount of system memory present.

  Before, they were hardcoded.



  This is to accommodate the needs of some new mods for ancient Minecraft versions that do not work well with the applet wrapper.

- On instance launch, the used GPU and graphics driver are reported - but only on linux.

  Other platforms will hopefully get this in the future.

- There are some under the hood improvements for ancient Minecraft versions and versions not provided by Mojang.

    - The `haspaid` parameter is set for the applet wrapper.
    - MultiMC will prefer to use `.minecraft` instead of `minecraft` folder inside the instances now.
    - There is some preliminary support for classic multiplayer - see [this workflowy list](https://workflowy.com/s/2EyDMcp7CU#/1cbfc198cf28) for details.
    - A new `noapplet` trait has been added to allow running legacy Minecraft versions without the applet wrapper.

- Mods without changed metadata (Example Mod) are now listed under their filename instead.

- Tweaker list in metadata now overrides the order of already present tweakers.

  This allows running [Vivecraft](http://www.vivecraft.org/). Official support will hopefully follow.

- Instance icons can now be in the SVG format. Also, aspect ratio of SVG icons is now preserved in the instance toolbar.

- GH-1082: It is now possible to disable and enable version components (packages) similarly to mods.

- A new material design / flat icon theme has been added.

- When changing instance component versions, the present version is selected first.

### Bugfixes

- paste.ee upload now works again.

  MultiMC now uses its new API. If you used a custom API key before, you will need to generate a new one.

- GH-1873, GH-1873, GH-1875 : The main window can now be closed regardless of running instances and running instances directly will not create a main window.

- GH-1854: MultiMC should no longer crash when the instance is closed while the kill confirmation dialog is open.

- GH-1956: Launch will abort sooner when important files are missing.

- GH-1874: Instance launching and updating MultiMC are now mutually exclusive.

  It was possible to do both at the same time, with undefined results.

- GH-1864: imgur album creation now works again.

- GH-1876: Various included libraries have been changed to satisfy their license terms.

  Namely:
  - pack200 (GPL with classpath exception, now a shared library)
  - iconfix (LGPL, now a shared library)
  - quazip (LGPL, now a shared library)
  - ColumnResizer (replaced with a BSD-3 version).

- GH-1882: Update dialog will now save its location and size.

- GH-1885: MultiMC will now correctly download zero-byte files.

  No content does not equal no file and a presence of a file can mean the difference between something working or not.

- When importing modpacks, file permissions from the pack archive will no longer be preserved.

  The archives are sometimes broken and have invalid permissions, especially when coming from sources other than MultiMC.

- Instance export filter has been fixed.

  The filtering logic was picking and ignoring incorrect files under some conditions. Also, hidden files were ignored.

- Download progress bars are now less jumpy.

  Instead of tracking the total size of all downloads, each download gets a fixed share of the progress bar.
  In many cases, the size of files is unknown before a download starts. The change means that the total progress bar size cannot increase as new downloads start and file sizes are discovered.

- GH-1927: fix crash bugs related to FML library downloads succeeding multiple times.

- Rare problems with error 201 during Mojang authentication have been fixed.

- GH-1971: MultiMC will now no longer check path prefixes when importing instances.

  This has caused more issues than it solved. Now it will simply try to move the files instead of giving up early.

- Instance import and creation have been overhauled in general for increased reliability.

- Hardcoded link colors in various dialogs and dialog pages have been fixed and now should follow theme palettes.

- GH-1993: Minimum and maximum JVM memory settings will now get swapped if set the wrong way.

  The values self-correct on both settings save and load now.

- GH-2050: Fixed behavior of cancel buttons when browsing for paths.

  This affected various settings dialogs and pages, setting the paths to an invalid value when the dialogs were closed with the `Cancel` button.

- The checkboxes in the accounts settings page now have the correct appearance.

- MultiMC responds to account manipulation better.

    - Setting and resetting default account will update the account list properly.
    - Removing the active account will now also reset it (previously, it would 'stay around').
    - The accounts model is no longer reset by every action.

- When closing and reopening the instance window, the log settings are preserved.

- In the instance export dialog, the sorting order has been changed to go from `a` to `z`, not backwards.

## MultiMC 0.5.1

### Improvements

- Log uploads now use HTTPS because the [paste.ee](https://paste.ee) site is switching to HTTPS only.

- Console now has the line limit and overflow settings properly set when hidden.

  Before, if you didn't have the console set to show up on launch, it would have some low default values set.
  This meant that you wouldn't get enough of the log when the game crashed.

- GH-1802: Log resize is now handled properly.

  The log could end up with many empty lines because the wrong maximum size was used during the resize, potentially losing some lines.

- GH-1807: Fixed 'loggging' typo in console overflow notification.

- GH-1801: Launch script is no longer dumped into MultiMC's log on instance launch.

- GH-1065: Use of 'folder' and 'directory' in the UI has been unified to 'folder'.

- GH-1788: A problem with missing translation templates in the setup wizard pages has been fixed.

  It should be possible to translate everything again.

- GH-1789: Deletion of custom icon has been fixed.

  It wasn't possible to do it from the MultiMC icon selection dialog.

- GH-1790: While using the system theme on macOS, dialogs had wrong colors.

  The wrong colors are now only visible immediately after changing the theme to 'System'. An application restart will fix the colors.

  The underlying issue cannot be easily fixed.

  Upstream bug: https://bugreports.qt.io/browse/QTBUG-58268

- GH-1793: The Java wizard page did not show up as expected when moving MultiMC between different computers.

  The page should now show up as expected.

- GH-1794: Copied FTB instances did not work properly.

  The instance type of the copy was not set, causing it to not be usable.

## MultiMC 0.5.0

### New or changed features

- GH-338, GH-513, GH-700: Edit instance dialog and Console window have been unified

  The resulting instance window can be closed or reopened at any point, it does not matter if the instance is running or not. The list of available pages in the instance window changes with instance state.

  Multiple instances can now run from the same MultiMC - It's even more **multi** now.

  On launch, the main window is kept open and running instances are marked with a badge. Opening the instance window is no longer the default action. Second activation of a running instance opens the instance window.

  MultiMC can be entirely closed, keeping Minecraft instances running. However, if you close MultiMC, play time tracking, logging and crash reporting will not work.

  Accounts which are in use are marked as such. If you plan to run multiple instances with multiple accounts, it is advisable to not set a default account to make it ask which one to use on launch.

- It is no longer possible to run multiple copies of MultiMC from a single folder

  This generally caused strange configuration and Mojang login issues because the running MultiMC copies did not know about each other.

  With the ability to launch multiple instances with different accounts, it is no longer needed.

  Trying to run a second copy will focus the existing window. If MultiMC was started without a main window, a new main window will be opened. If the second copy is launching an instance from the command line, it will launch in the first copy instead.

  This feature is also used for better checking of correct update completion (GH-1726). It should no longer be possible for MultiMC to end up in a state when it is unable to start - the old version checks that the new one can start and respond to liveness checks by writing a file.

- GH-903: MultiMC now supports theming

  By default, it comes with a Dark, Bright, System (the old default) and Custom theme.

  The Custom theme can change all of the colors, change the Qt widget theme and style the whole UI with CSS rules.
  Files you can customize are created in `themes/custom/`. The CSS theming is similar to what TeamSpeak uses.

  Ultimately, this is a start, not a final solution. If you are interested in making custom themes and would like to shape the direction this takes in the future, ask on Discord. :)

- Translations have been overhauled

  You no longer need to restart MultiMC to change its active translation. MultiMC also asks which translation to use on the first start.

  There is a lot that has to be done with translations, but at least now it should be easier to work with them and use them.

- MultiMC now includes Google Analytics

  The purpose of this is to determine where to focus future effort. Generally, only basic technical information is collected:

  - OS name, version, and architecture
  - Java version, architecture and memory settings
  - MultiMC version
  - System RAM size

  It does not activate until you agree with it. It may be expanded upon later, in which case you will be asked to agree again.

- Java selection on start has been replaced with a more robust solution

  You can select from the list as before, but also provide your own Java and set the basic memory sizes - Heap and PermGen (for java < 8).

  It is checking the configuration and selected Java on the fly and provides more or less instant feedback.

- Java detection has been improved

  MultiMC will prefer looking for `javaw.exe` on Windows and now can scan most, if not all the usual Linux java paths.

- Java memory settings now allow running with less memory

  The minimum has been changed to 128 MB.

- There is now an initial setup wizard

  So far, it is used for selecting the translation to use, the analytics agreement and initial Java setup.

- Existing MCEdit integration has been replaced by the Worlds page in the Instance/Console window

  It supports renaming, copying, and deleting worlds, opening them in MCEdit and copying the world seed without the need to launch Minecraft.

  The Linux version of MCEdit is now also started from the shell script, fixing some compatibility issues.

- GH-767: Minecraft skin upload

  The `Upload Skin` button is located on the Accounts page.

- It is now possible to turn off line wrapping in the Minecraft log
- Groups now have a proper context menu

  You can delete groups and create instances in them using the context menu. Just right click anywhere inside a group that's not an instance.

- Exporting of tracked FTB instances has been disabled

  It did not produce viable instances.

- Added support for Liteloader snapshots

  Requested many times, it's finally available.

- GH-1635, GH-1273, GH-589, GH-842, GH-901, GH-1117: Mod lists have been improved heavily

  - There is filter bar to allow finding mods in large packs quickly.
  - Extended selection is allowed (does not have to be continuous).
  - You can enable and disable many mods at the same time.
  - Sorting by clicking on the column headers is now possible.
  - Mod lists have a column for when a mod was changed last time (or added using the mod list).
  - You can open the `config` folder from the mods list now.

- GH-352: It is now possible to cancel an instance update.

- Instance launch button now has a drop-down arrow instead of click and hold.

  This should make launching with profilers more discoverable.

- When instances do not exit properly (crash), they get a badge

  This should make it easier to spot what crashed if you have multiple running.

- Instances can now contain libraries

  Any libraries stored in `$instanceroot/libraries/` will override the libraries from MultiMC's global folders, as long as they are marked `local` in the JSON patch.

  This should make installing library-based mods easier in the future, and allow to include them in modpacks.

### Improvements

- GH-1433: The account selection dialog no longer shows e-mail addresses when no default account is selected.

  Instead, it shows Minecraft profile names.

- GH-1643: The preferred language property is no longer being censored in logs.

  Because the values are often very short (`en` for example), it was simply not usable.

- GH-1521: JSON editor now works when customized.

- GH-1560: Leading whitespace is now removed from instance names on creation and renaming

  Leading and trailing spaces in names can confuse Windows Explorer and Java.

- GH-1586: MultiMC now prints to command line on Windows, so you can review the command line options.

- GH-1699: Linux builds no longer contain the XCB library

  This caused many compatibility issues on with certain Linux graphics drivers and prevented MultiMC from starting.

- GH-1731: it was possible for the Screenshots page to show a list of all system drives.

  Trying to delete said system drives obviously lead to data loss. Additional checks have been added to prevent this from happening.

- GH-1670: "Instance update failed because: Too soon! Let the LWJGL list load :)." has been fixed.

  This fixes launching of legacy (and legacy FTB) instances.

- GH-1778: Jar modded Minecraft.jar location breaks mod assumptions

  Some ancient mods require the modded `Minecraft.jar` to be in `.minecraft/bin`, inside the instance. Now it is placed there.

### Internals

- Full support for the current Mojang downloads JSON format.

  This includes checksum verification, when available.

- Minecraft logging has been overhauled

  The log now persists after the instance/console window is closed.

- GH-575: Mod lists got a refactor

  The original issue is about adding sub-folder listings to mod lists. However, this is simply a refactor that separates the old Jar mod list from the less complex Loader mods. It allowed all of the mod list improvements to happen.

- The network code has been heavily reworked

  Most issues related to slow networks and failing downloads should be a thing of the past.
  This also includes post-download validation of the download - like using SHA1 checksums.

- Minecraft launching has been reworked

  It is now a lot of tiny reusable tasks that chain together.

  MultiMC now also has a separate launch method that works more like the Mojang launcher (not using a launcher part, but running Java directly).

## MultiMC 0.4.11

This release contains mainly a workaround for Minecraft 1.9 support and returned support for OSX 10.7.

### **IMPORTANT**

- GH-1410: MultiMC crashes on launch on OSX 10.7

  MultiMC didn't work on OSX 10.7 because of an oversight in the build server setup. This has been fixed.

- GH-1453: Minecraft 1.9 snapshots didn't download and launch properly

  This has been caused by a change on Mojang servers - the data is now stored in a different location and the files describing the releases have a different format. The required changes on MultiMC side aren't complete yet, but it's enough to get snapshots working.

  Full support for the new version file format will come in the next release.

- MultiMC version file format was simplified

  Some undocumented and unused features were removed from the format. Mostly version patches that removed libraries, advanced library application, and merging rules, and things of similar nature. If you used them, you used an undocumented feature that is impossible to reach from the UI.

### Improvements

- GH-1502: When the locally cached Minecraft version was deleted, the instance that needed it would have to be started twice

  This was caused by generating the list of launch instructions before the update. It is now fixed.

- Version file issues are now reported in the instance's `Version` page.

  This doesn't apply to every possible issue yet and will be expanded upon in the next release.

## MultiMC 0.4.10

Second hotfix for issues with wifi connections.

### **IMPORTANT**

- GH-1422: Huge ping spikes while using MultiMC

  Another day, another fix. The bearer plugins added in 0.4.9 didn't really help and we ran into more bugs.

  This time, the presence of the network bearer plugins caused a lot of network lag for people on wifi connections.

  Because this wasn't a problem on the previous version of Qt MultiMC used (5.4.2), I ended up reverting to that. This is a temporary solution until the Qt framework can be rebuilt and retested for every platform without this broken feature.

  The upstream bug is [QTBUG-40332](https://bugreports.qt.io/browse/QTBUG-40332) and despite being closed, it is far from fixed.

Because of the reverted Qt version, OSX 10.7 *might* work again. If it does, please do tell, it would help with figuring out what went wrong there :)


## MultiMC 0.4.9

Hotfix for issues with wifi connections.

### **IMPORTANT**

- GH-1408: MultiMC 0.4.8 doesn't work on wireless connections.

  This is especially the case on Windows. If you already updated to 0.4.8, you will need to do a manual update, or use a wired connection to do the update.

  The issue was caused by a change in the underlying framework (Qt), and MultiMC not including the network bearer plugins. This made it think that the connection is always down and not try to contact any servers because of that.

  The upstream bug is [QTBUG-49267](https://bugreports.qt.io/browse/QTBUG-49267).

- GH-1410: MultiMC crashes on launch on OS X 10.7.5

  OSX 10.7.x is no longer supported by Apple and I do not have a system to test and fix this.

  So, this is likely **NOT** going to be fixed - please update your OS if you are still running 10.7.

### Improvements

- GH-1362: When uploading or copying the Minecraft log, the action is logged, including a full timestamp.

## MultiMC 0.4.8

Fluffy and functional!

### **IMPORTANT**

- GH-1402: MultiMC will keep its binary filename after an update if you rename it.

  Note that this doesn't happen with this (0.4.8) update yet, because the old update method is still used.

  If you renamed `MultiMC.exe` for any reason, you will have to manually remove the renamed file after the update and rename the new `MultiMC.exe`.

  Future updates should no longer have this issue.


### New features

- GH-1047, GH-1233: MultiMC now includes basic Minecraft world management.

  This is a new page in the console/edit instance window.

  You can:
  - Copy worlds
  - Delete worlds
  - Copy the world seed value
  - Run MCEdit - the MCEdit feature has been moved here.

- GH-1217: MultiMC now tracks instance play time and displays it when the instance is selected.

- New buttons on the top toolbar:
  - GH-1238: button for the [MultiMC subreddit](https://www.reddit.com/r/MultiMC/).
  - GH-1397: button for joining the [MultiMC discord voice/chat server](https://discord.gg/0k2zsXGNHs0fE4Wm).

    Both are there for you to interact with other MultiMC users and us.

- GH-253, GH-1300: MultiMC can now be started with the `-l "Instance ID"` parameter, launching the specified instance directly.

### Improvements

- Instance list
  - GH-1121: Instances are now selected after you create them.
  - GH-93: When copying an instance, you can tell MultiMC to not copy the worlds.

- Mod and resource pack lists
  - GH-1237: Mod info is now clickable and selectable.
  - GH-1322: Mod description `...` link will no longer pop up multiple dialogs.
  - GH-1178: When dragged and dropped, folder based mods and resource packs will be copied properly on OSX.

- MCEdit integration:
  - GH-1009: MCEdit Unified on linux is now recognized properly.

- Mojang login and accounts:
  - GH-1158: A unique ID is generated on the MultiMC side before login, instead of letting the server decide.
  - When a password is required, the user login is partially obscured.
  - The dropdown menu on the main window now lists profiles, not accounts.

- Modpacks:
  - GH-1140: Modpack downloads now check for update on the server even if the file is already locally available.
  - GH-1148: When creating an instance from modpack, the instance name will be guessed based on the modpack file or URL (unless you set it yourself).
  - GH-1280: While importing modpacks, the progress dialog now says what is happening.
  - When selecting the modpack field in the new instance dialog, the contents are selected for easy replacement.

- Instance settings
  - Wrapper commands now use the proper UI field and do not get replaced with pre-launch commands.

- Minecraft launching:
  - GH-1053, GH-1338: Minecraft launching has been completely redone.
  - GH-1275: Server resource pack folder is created on launch.
    - This is a workaround for Minecraft bug MCL-3732.
  - GH-1320: Improve compatibility with non-Oracle Java.
  - GH-1355: LAUNCHER environment will no longer leak into Minecraft after MultiMC updates itself.

- Minecraft log:
  - Java exception detection in Minecraft logs has been improved.
  - GH-719: You can now use your own [paste.ee](https://paste.ee/) account for uploading logs.
    - New [paste.ee](https://paste.ee/) settings page has been added to the global settings dialog.
  - GH-1197: Text colors in log window now adapt to the background color.
  - GH-1164: The censor filter could be initialized with empty values, leading to unreadable log.
  - GH-1008, GH-1046, GH-1067: Log size limiting.

    The log window now has a configurable limit for the number of lines remembered. You can also specify whether it stops logging or forgets on the fly once the limit is breached.

    This prevents the MultiMC log window from using too much memory on logging. The default limit is 100000 lines and the logging stops.

    Minecraft logging this much is a sign of a problem that needs to be fixed. Any complaints should be addressed to the responsible mod authors.

- Screenshot upload
  - GH-1339: While uploading screenshots, the console window will not close (prevents a crash).

- Other logs:
  - GH-926: 'Other Logs' now has a button for removing all log files.
  - Hidden log files are shown in 'Other logs'.

- User skins:
  - MultiMC now uses [crafatar.com](https://crafatar.com/) for skin downloads instead of the Mojang servers.

- Java:
  - GH-1365: MultiMC now supports Java 9 and its new version numbering.
  - GH-1262: Java can now be placed in a folder relative to MultiMC's folder. This allows bundling of JREs with MultiMC.

- Translations:
  - GH-1313: Some parts of the MultiMC user interface have been marked as 'not for translation'.

### Internals and internal bug fixes

- GH-1052: All the dependencies were rebuilt and the build environment upgraded to the latest compiler versions.
- GH-1051: The CDPATH environment variable is now ignored.
- GH-77, GH-1059, GH-1060: The MultiMC updater is no longer used or necessary.

  It is only present to preserve compatibility with previous versions.
  Updates now work properly on Windows systems when you have Unicode (like ❄, Ǣ or Ω) characters in the path.

- GH-1069, GH-1100: `LD_LIBRARY_PATH` and `LD_PRELOAD` environment variables supplied to MultiMC now don't affect MultiMC but affect the launched game.

  This means you can use something like the Steam overlay in MultiMC instances on Linux.
  If you need to use these variables for MultiMC itself, you can use `LAUNCHER_LIBRARY_PATH` and `LAUNCHER_PRELOAD` instead.

- GH-1389: External processes (like folder views, editors, etc.) are now started in a way that prevents the MultiMC environment to be reused by them.
- GH-1355: If `LD_LIBRARY_PATH` contains any of MultiMC's internal folders, this will not propagate to started processes.
- GH-1231, GH-1378: libpng is now included with the Linux version of MultiMC
- GH-1202: SSL certificates are now rebuilt on start on OSX.

- GH-1303: Translations and notification cache are stored in the normal data folder now, not alongside the binaries. This only affects third party Linux packaging.
- GH-1266, GH-1301: Linux runner scripts has been improved.
- GH-1360: Development and other unstable versions of MultiMC now uses GitHub commits instead of this manually maintained changelog.

## MultiMC 0.4.7

This is what 0.4.6 should have been. Oh well, at least it's here now!

### Functional changes
- GH-974: A copy of the libstdc++ library is now included in Linux releases, improving compatibility
- GH-985: Jar mods are now movable and removable after adding
- GH-983: Use `minecraft.jar` as the main jar when using jar mods - fixes NEI in Legacy Minecraft versions
- GH-977: Fix FTB paths on Windows

   This removes some very old compatibility code. If you get any issues, make sure you run the FTB Launcher and let it update its files.
- GH-992 and GH-1003: Improved performance when saving settings:
  - Bad performance was caused by improved data consistency
  - Each config file is now saved only once, not once for every setting
  - When loading FTB instances, there are no writes to config files anymore
- GH-991: Implemented wrapper command functionality:

  There is an extra field in the MultiMC Java settings that allows running Java inside a wrapper program or script. This means you can run Minecraft with wrappers like `optirun` and get better performance with hybrid graphics on Linux without workarounds.
- GH-997: Fixed saving of multi-line settings. This fixes notes.
- GH-967: It is now possible to add patches (Forge and LiteLoader) to tracked FTB instances properly.

  Libraries added by the patches will be taken from MultiMC's `libraries` folder, while the tracked patches will use FTB's folders.

- GH-1011 and GH-1015: Fixed various issues when the patch versions aren't complete

  This applies when Minecraft versions are missing or when patches are broken and the profile is manipulated by adding, moving, removing, customizing and reverting patches.

- GH-1021: Built in legacy Minecraft versions aren't customizable anymore

   The internal format for Legacy Minecraft versions does not translate to the external patch format and would cause crashes
- GH-1016: MultiMC prints a list of mods, core mods (contents of the core mods folder) and jar mods to the log on instance start. This should help with troubleshooting.
- GH-1031: Icons are exported and imported along with instances

    This only applies if the icon was custom (not built-in) when exporting and the user doesn't choose an icon while importing the pack.

### UI changes
- GH-970: Fixed help button for the External tools and Accounts dialog pages not linking to the proper wiki places
  - Same for the Versions dialog page

- GH-994: Rearranged the buttons on the Versions page to make jar mods less prominent

  Using the `Add jar mods` button will also show a nag dialog until it's been used successfully

## MultiMC 0.4.6

Long time coming, this release brought a lot of incremental improvements and fixes.

### Functional changes
- Old version.json and custom.json version files will be transformed into a Minecraft version patch:
  - The process is automated
  - LWJGL entries are stripped from the original file - you may have to re-do LWJGL version customizations
  - Old files will be renamed - .old extension is added
- It's now possible to:
  - Customize, edit and revert built in version patches (Minecraft, LWJGL)
  - Edit custom version patches (Forge, LiteLoader, other)
- Blocked various environment variables from affecting Minecraft:
  - `JAVA_ARGS`
  - `CLASSPATH`
  - `CONFIGPATH`
  - `JAVA_HOME`
  - `JRE_HOME`
  - `_JAVA_OPTIONS`
  - `JAVA_OPTIONS`
  - `JAVA_TOOL_OPTIONS`
  - If you rely on those in any way, now would be a time to fix that
- Improved handling of LWJGL on OSX (.dylib vs. .jnilib extensions)
- Jar mods are now always put into a generated temporary Minecraft jar instead of being put on the classpath
- PermGen settings:
  - Changed default PermGen value to 128M because of many issues from new users
  - MultiMC now recognizes the Java version used and will not add PermGen settings to Java >= 1.8
- Implemented simple modpack import and export feature:
  - Export allows selecting which files go into the resulting zip archive
  - Only MultiMC instances for now, other pack formats are planned
  - Import is either from local file or URL, URL can't have ad/click/pay gates
- Instance copy doesn't follow symlinks on Linux anymore
  - Still does on Windows because copying symlinks requires Administrator level access
- Instance delete doesn't follow symlinks anymore - anywhere
- MCEdit tool now recognizes MCEdit2.exe as a valid file to runtime
- Log uploads now follow the maximum allowed paste sizes of paste.ee and are encoded properly
- MultiMC now doesn't use a proxy by default
- Running profilers now works on Windows
- MultiMC will warn you if you run it from WinRAR or temporary folders
- Minecraft process ID is printed in the log on start
- SSL certificates are fixed on OSX 10.10.3 and newer - see [explanation](http://www.infoworld.com/article/2911209/mac-os-x/yosemite-10103-breaks-some-applications-and-https-sites.html).

### UI changes
- Version lists:
  - All version lists now include latest and recommended versions - recommended are pre-selected
  - Java version list now sorts versions based on suitability - best on top
  - Forge version list includes the development branch the version came from
  - Minecraft list marks the latest release as 'recommended' and latest snapshot as 'latest' if it is newer than the release
- Mod lists:
  - Are updated and sorted after adding mods
  - Browse buttons now properly open the central mods folder
  - Are no longer watching for updates when the user doesn't look at them
  - Loader mod list now recognizes .litemod files as valid mod files
- Improved wording of instance delete dialog
- Icon themes:
  - Can be changed without restarting
  - Added a workaround for icon themes broken in KDE Plasma 5 (only relevant for custom builds)
- Status icons:
  - Included a 'yellow' one
  - Are clickable and link to [help.mojang.com](https://help.mojang.com/)
  - Refresh when the icon theme does
- Changed default console font to Courier 10pt on Windows
- Description text in the main window status bar now updates when Minecraft version is changed
- Inserted blatant self-promotion (Only Minecraft 1.8 and up)
  - This adds a bit of unobtrusive flavor text to the Minecraft F3 screen
- Log page now has a button to scroll to bottom
- Errors are reported while updating the instance on the Version page
- Fixed typos (forge -> Forge)

### Internals
- Massive internal restructuring (ongoing)
- Downloads now follow redirects
- Minecraft window size is now always at least 1x1 pixel (prevents crash from bad settings)
- Better handling of Forge downloads (obviously invalid/broken files are redownloaded)
- All download tasks now only start 6 downloads, using a queue (fixes issues with assets downloads)
- Fixed bugs related to corrupted settings files (settings and patch order file saves are now atomic)
- Updated zip manipulation library - files inside newly written zip/jar files should have proper access rights and timestamps
- Made Minecraft resource downloads more resilient (throwing away invalid/broken index files)
- Minecraft asset import from old format has been removed
- Generally improved MultiMC logging:
  - More error logging for network tasks
  - Added timestamps relative to application start
- Fixed issue with the application getting stuck in a modal dialog when screenshot uploads fail
- Instance profiles and patches are now loaded lazily (speeds up MultiMC start)
- Groups are saved after copying an instance
- MultiMC launcher part will now exit cleanly when MultiMC crashes or is closed during instance launch


## MultiMC 0.4.5
- Copies of FTB instances should work again (GH-619)
- Fixed OSX version not including the hotfix number
- If the currently used java version goes missing, it now triggers auto-detect (GH-608)
- Improved 'refresh' and 'update check' icons of the dark and bright simple icon themes (GH-618)
- Fixed console window hiding - it no longer results in windowless/unusable MultiMC

## MultiMC 0.4.4
- Other logs larger than 10MB will not load to prevent logs eating the whole available memory
- Translations are now updated independently from MultiMC
- Added new and reworked the old simple icon themes
- LWJGL on OSX should no longer clash with Java 8
- Update to newer Qt version
  - Look and feel updated for latest OSX
- Fixed issues caused by Minecraft inheriting the environment variables from MultiMC
- Minecraft log improvements:
  - Implemented search and pause
  - Automated coloring is updated for log format used by Minecraft 1.7+
  - Added settings for the font used in the console, using sensible defaults for the OS
- Removed MultiMC crash handler, it will be replaced by a better one in the future

## MultiMC 0.4.3
- Fix for issues with Minecraft version file updates
- Fix for console window related memory leak
- Fix for travis.ci build

## MultiMC 0.4.2
- Show a warning in the log if a library is missing
- Fixes for relocating instances to other MultiMC installs:
  - Libraries now use full Gradle dependency specifiers
  - Rework of forge installer (forge can reinstall itself using only the information already in the instance)
  - Fixed bugs in rarely used library insertion rules
- Make the global settings dialog into a page dialog
- Check if the Java binary can be found before launch
- Show a warning for paths containing a '!' (Java can't handle that properly)
- Many smaller fixes

## MultiMC 0.4.1
- Fix LWJGL version list (SourceForge has changed the download API)

## MultiMC 0.4.0
- Jar support in 1.6+
- Deprecated legacy instances
  - Legacy instances can still be used but not created
  - All Minecraft versions are supported in the new instance format
- All instance editing and settings dialogs were turned into pages
  - The edit instance dialog contains pages relevant to editing and settings
  - The console window contains pages useful when playing the game
- Redone the screenshot management and upload (page)
- Added a way to display and manage log files and crash reports generated by Minecraft (page)
- Added measures to prevent corruption of version files
  - Minecraft version files are no longer part of the instances by default
- Added help for the newly added dialog pages
- Made logs uploaded to paste.ee expire after a month
- Fixed a few bugs related to liteloader and forge (1.7.10 issues)
- Icon themes. Two new themes were added (work in progress)
- Changelog and update channel are now visible in the update dialog
- Several performance improvements to the group view
- Added keyboard navigation to the group view

## MultiMC 0.3.9
- Workaround for 1.7.10 Forge

## MultiMC 0.3.8
- Workaround for performance issues with Intel integrated graphics chips

## MultiMC 0.3.7
- Fixed forge for 1.7.10-pre4 (and any future prereleases)

## MultiMC 0.3.6
- New server status - now with more color
- Fix for FTB tracking issues
- Fix for translations on OSX not working
- Screenshot dialog should be harder to lose track of when used from the console window
- A crash handler implementation has been added.

## MultiMC 0.3.5
- More versions are now selectable when changing instance versions
- Fix for Forge/FML changing its mcmod.info metadata format

## MultiMC 0.3.4
- Show a list of Patreon patrons in credits section of the about dialog
- Make the console window raise itself after Minecraft closes
- Add Control/Command+q shortcut to quit from the main window
- Add french translation
- Download and cache FML libs for legacy versions
- Update the OS X icon
- Fix FTB libraries not being used properly

## MultiMC 0.3.3
- Tweak context menu to prevent accidental clicks
- Fix adding icons to custom icon directories
- Added a Patreon button to the toolbar
- Minecraft authentication tasks now provide better error reports

## MultiMC 0.3.2
- Fix issues with libraries not getting replaced properly (fixes instance startup for new instances)
- Fix April fools

## MultiMC 0.3.1
- Fix copying of FTB instances (instance type is changed properly now)
- Customizing FTB pack versions will remove the FTB pack patch file

## MultiMC 0.3
- Improved instance view
- Overhauled 1.6+ version loading
- Added a patch system for instance modification
  - There is no longer a single `custom.json` file that overrides `version.json`
  - Instead, there are now "patch" files in `<instance>/patches/`, one for each main tweaker (forge, liteloader etc.)
  - These patches are applied after `version.json` in a customizable order,
  - A list of these files is shown in the leftmost tab in the Edit Mods dialog, where a list of libraries was shown before.
  - `custom.json` can still be used for overriding everything.
- Offline mode can be used even when online
- Show an "empty" message in version selector dialogs
- Fix FTB paths on windows
- Tooling support
  - JProfiler
  - JVisualVM
  - MCEdit
- Don't assume forge in FTB instances and allow other libraries (liteloader, mc patcher, etc.) in FTB instances
- Screenshot uploading/managing
- Instance badges
- Some pre/post command stuff (remove the timeout, variable substitution)
- Fix logging when the system language is not en_US
- Setting PermGen to 64 will now omit the java parameter because it is the default
- Fix encoding of escape sequences (tabs and newlines) in config files

## MultiMC 0.2.1
- Hotfix - move the native library extraction into the onesix launcher part.

## MultiMC 0.2
- Java memory settings have MB added to the number to make the units obvious.
- Complete rework of the launcher part. No more sensitive information in the process arguments.
- Cached downloads now do not destroy files on failure.
- Mojang service status is now on the MultiMC status bar.
- Java checker is no longer needed/used on instance launch.
- Support for private FTB packs.
- Fixed instance ID issues related to copying FTB packs without changing the instance name.
- Forge versions are better sorted (build numbers above 999 were sorted wrong).
- Fixed crash related to the MultiMC update channel picker in offline mode.
- Started using icon themes for the application icons, fixing many OSX graphical glitches.
- Icon sources have been located, along with icon licenses.
- Update to the German translation.

## MultiMC 0.1.1
- Hotfix - Changed the issue tracker URL to [GitHub issues](https://github.com/MultiMC/Launcher/issues).

## MultiMC 0.1
- Reworked the version numbering system to support our [new Git workflow](http://nvie.com/posts/a-successful-git-branching-model/).
- Added a tray icon for the console window.
- Fixed instances getting deselected after FTB instances are loaded (or whenever the model is reset).
- Implemented proxy settings.
- Fixed sorting of Java installations in the Java list.
- Jar files are now distributed separately, rather than being extracted from the binary at runtime.
- Added additional information to the about dialog.

## MultiMC 0.0
- Initial release.
