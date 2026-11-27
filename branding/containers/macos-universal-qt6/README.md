# macOS universal with Qt 6.2.0

This image uses the existing private MeshMC osxcross toolchain as its base.
The base must support both `x86_64` and `arm64`. Keep the SDK subject to
the same distribution rules as the base image.

From this directory, provide the universal Qt 6.2.0 macOS SDK with Qt SVG
and Core5Compat as `qt`. Provide the matching Qt 6.2.0 Linux `gcc_64` SDK
as `qthost` so osxcross can run the host versions of moc, rcc and uic.
The image installs the Linux GLib and libatomic runtimes needed by the host SDK
and checks that qmake, moc, rcc and uic can start during the image build.

```sh
docker buildx build --platform linux/amd64 --load \
  --build-context qt=/path/to/6.2.0/macos \
  --build-context qthost=/path/to/6.2.0/gcc_64 \
  --tag ghcr.io/project-tick/meshmc/macos-universal-qt6:dev .
```

MeshMC CI uses the `osx-universal-qt6` platform and checks the executable
for both Mach-O slices. The app bundles Qt and is a separate updater
variant. Verify startup on both Intel and Apple Silicon before release.
