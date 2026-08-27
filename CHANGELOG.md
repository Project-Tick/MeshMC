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

## Previous versions

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
