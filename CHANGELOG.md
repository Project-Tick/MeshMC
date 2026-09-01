# MeshMC 10.0.0 (!DRAFT!)

## Highlights

* It was heartbreaking, I'm not lying.
* Being alone is tough, but I like to persevere.
* This release is really full of emotion, no offense to anyone.

## Added

* Shortcut system added.
* A world selection feature has been added to the Shortcut system.
* Patreon link added.
* Title bar theming added for macOS.
* A setting to edit the Skins and Java folders has been added.
* The "Folder" tab in the top toolbar now includes a list of multiple useful folders.
* The mod installation and update system has been completely redesigned.
* Datapack installation and update system added.
* ShaderPack installation and update system has been completely redesigned.
* ResourcePack installation and update system has been completely redesigned.
* The ability to throw the instance in the trash has been added.
* Detailed logging feature added.
* Demo mode support has been added.
* The number of entries in the Instance Toolbar has been reduced and icons have been added.
* News display system added.
* Discord URL added.
* The ability to blacklist the plugin has been added.
* Backup system added.
* The "More News" tab has been added back.
* Toolbar locking system added.
* The ability to move two toolbars other than the instance toolbar has been added.
* A toolbar that works with the <ALT> key has been added, except for macOS.
* The backup process now shows a progress bar.
* Mod Pack management support added.
* Multiple instance folder support has been added.
* Export via MRPack and Curseforge ZIP has been added.
* A separate screen was created for loaders.
* vcpkg build has been added.
* RPM Spec has been added.

## Changed

* GreenDark theme palette updated.
* The macOS ToolBar code has been rewritten.
* The repository structure has been rebuilt.
* Minecraft is now downloaded every time an instance is created.
* vcpkg automates bootstrap and package management.
* The `std::optional` feature, introduced in the C++17 standard, is now in use.
* Path corrections have been added for UNIX installations (excluding macOS),
  especially for Linux; compatibility with Debian and RedHat policies has been ensured.

## Fixed

* The problem of Minecraft not closing when trying to kill an instance on Windows has been solved.
* The error of not adding version entries in NSIS has been fixed.

## Removed

* The ability to directly delete instances has been removed.
* The Filelink plugin has been removed.
* MinGW aarch64 test and packaging removed.
* The NewsViewer plugin has been removed.
* The BackupSystem plugin has been removed.
* PackUpdater plugin has been removed
* PackPortal plugin has been removed
* The Feature Flag feature has been completely removed.
* All Rust code has been removed.
* Optional Bare library has been removed.
* Analysis collection has been removed.

## Deprecated

* MMCO API: The ability to add input to the Instance Toolbar has been deprecated and changed to no-op.

## Previous versions

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

## Previous versions

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
