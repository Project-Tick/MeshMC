# MeshMC 8.0.0 (DRAFT)

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

## MeshMC 7.19.0

### Added

* Added new NewsViewer plugin
* Added new API hooks
