# setup-kf6

A general purpose composite action that builds and caches
[KDE Frameworks 6](https://api.kde.org/frameworks/) from the official release
tarballs against a Qt 6 you already installed (for example with
[`jurplel/install-qt-action`](https://github.com/jurplel/install-qt-action) /
aqtinstall). It does not depend on vcpkg, Conan, distro packages or anything
else in this repository, so it can be used from other projects as
`uses: <owner>/<repo>/.github/actions/setup-kf6@<ref>`.

Works on GitHub-hosted Linux, macOS and Windows (MSVC) runners.

## Usage

```yaml
- uses: jurplel/install-qt-action@v4
  with:
    version: 6.12.0
    modules: qtshadertools qt5compat   # see "Qt requirements"

# Everything KDE Frameworks offers on this platform:
- uses: ./.github/actions/setup-kf6

# ...or only what you need. Dependencies are found and added for you:
- uses: ./.github/actions/setup-kf6
  with:
    frameworks: kconfig kcoreaddons ki18n kio
```

On Windows enter the Visual Studio developer environment first
(`ilammy/msvc-dev-cmd`); the action checks that `cl` is on `PATH`.

Afterwards `CMAKE_PREFIX_PATH`, `PATH`, `QT_PLUGIN_PATH`, `QML_IMPORT_PATH`,
`XDG_DATA_DIRS` and `KF6_PREFIX` are exported (turn off with
`export-env: false`), so a plain `find_package(KF6Config REQUIRED)` works.

## How it works

1. **Plan.** The release listing for the requested version is read from
   download.kde.org, tarballs are downloaded and verified against the published
   SHA-256. Nothing is hard-coded per framework; the plan is derived from what
   ships in each tarball:

   | source | used for |
   |---|---|
   | the CMake files (`find_package(KF6... REQUIRED)`) | **hard** dependencies: what the framework really needs |
   | `.kde-ci.yml` and optional `find_package`s | **soft** dependencies: only affect build order |
   | `metainfo.yaml` | declared tier, supported platforms |

   `.kde-ci.yml` alone is not reliable: it only lists what KDE's CI exercises per
   platform, so it sometimes lists more than is needed (documentation tooling
   such as `kdoctools` for `kpackage`) and sometimes less (`kcmutils` has no
   macOS entry although it requires KIO there). Hard dependencies are pulled
   in and a failure blocks whatever needs them; a failed soft dependency does
   not. A hard dependency that does not exist on the platform is treated as soft
   (it is nearly always guarded by a platform check).

   * No `frameworks` input: every framework supported on the runner's platform
     is built (frameworks KDE does not support there, e.g. `baloo` on Windows,
     are skipped, as is anything that hard-depends on a skipped framework).
   * `frameworks: a b c`: the hard dependency tree is walked from those and only
     the closure is built. Asking for something unsupported on the platform is
     an error with a message saying who needed it.
2. **Tiers.** A framework's tier is the one KDE declares, raised to the tier of
   anything it depends on if the declaration is missing or inconsistent. Tier 1
   is built before tier 2 before tier 3 (`frameworkintegration` is the only
   tier 4 framework). Within a tier frameworks are ordered by dependency depth,
   so every dependency is always installed before it is needed.
3. **System packages.** Build tools and libraries needed by the planned
   frameworks (gperf, gettext, bison, X11/Wayland headers, libical, ...) are
   installed with apt / brew / choco. Only packages for frameworks in the plan
   are installed. Disable with `system-deps: false`.
4. **Build.** Each framework is configured, built and installed into a single
   prefix, tier by tier, with `BUILD_TESTING=OFF`, `BUILD_QCH=OFF`, shared libs
   (`linkage: shared`) and `Release`.
5. **Cache.** Every tier is cached on its own as the *files that tier added* to
   the prefix, together with a manifest saying which frameworks it contains and
   how each was built (a *fingerprint*: tarball, its CMake arguments and the
   fingerprints of what it depends on). The key of tier N covers KF version,
   Qt version, OS/arch, build type, linkage, CMake arguments, the install prefix
   and exactly the frameworks of tiers 1..N. Consequences:
   * if tier 3 fails, tiers 1 and 2 are already saved and the next run starts at
     tier 3;
   * **a tier in which something failed is saved too** (under a `-partial-` key),
     and the next run restores it and builds only what is missing. A tier can take
     half an hour on a Windows runner, so fixing one framework does not redo the rest;
   * the cache is found even if settings changed (restore-keys), but a framework
     is only trusted if its fingerprint still matches, anything else is rebuilt.
     Changing the arguments of one framework therefore only rebuilds it and what
     depends on it;
   * asking for a different subset of frameworks reuses every tier that is
     identical.

## Inputs

| input | default | |
|---|---|---|
| `version` | `6.31.0` | KDE Frameworks release, or `latest` |
| `frameworks` | _(all)_ | what to build; names like `kio`, `KF6KIO`, `KF6::KIO`; dependencies are added |
| `exclude` | | frameworks to leave out |
| `qt-prefix` | `$QT_ROOT_DIR` / `$Qt6_DIR` | Qt 6 install (set by install-qt-action) |
| `qt-host-path` | | host Qt for cross builds (`QT_HOST_PATH`) |
| `install-prefix` | `<runner temp>/kf6` | where KDE Frameworks is installed |
| `build-type` | `Release` | |
| `linkage` | `shared` | `shared` or `static` |
| `generator` | `Ninja` | |
| `osx-architectures` | | e.g. `x86_64;arm64` (see Limitations) |
| `cmake-args` | | extra arguments for every framework (come after the defaults) |
| `framework-cmake-args` | | `<framework>: <args>` per line |
| `system-deps` | `true` | install needed system packages |
| `extra-system-packages` | | additional native package names |
| `cache` | `true` | per-tier caching |
| `cache-key-suffix` | | change to force a rebuild |
| `keep-going` | `false` | build everything that does not depend on a failure, report all failures at the end |
| `ignore-failures` | `false` | downgrade failures to a warning |
| `export-env` | `true` | export `CMAKE_PREFIX_PATH`, `PATH`, ... |
| `jobs` | CPU count | parallel jobs |
| `verbose` | `false` | stream full build output into the job log |
| `upload-logs` | `true` | upload per-framework logs as an artifact on failure |
| `base-url`, `work-dir` | | mirror and scratch directory |

Outputs: `prefix`, `frameworks` (what was built), `cache-hit`, `failed`,
`kf-version`.

The job summary shows the plan and a per-framework result table
(built / cached / failed / skipped). When something fails the CMake/compiler
error lines and the end of the log are printed in the job log, and all logs are
uploaded as an artifact.

> While bringing up a new platform set `keep-going: true`: one run then tells
> you every framework that fails instead of only the first.

## Qt requirements

The frameworks need these Qt modules; with aqtinstall the base install covers
most of them, add the rest with `modules:`:

* the base install: Core, Gui, Widgets, Network, DBus, Xml, Concurrent, Sql, Test,
  PrintSupport, Qml, Quick, QuickControls2, QuickWidgets, Svg, LinguistTools
* `qtshadertools`: `kirigami`, `kdeclarative`, `kquickcharts` compile Qt Quick shaders
* `qtspeech` (Qt TextToSpeech): required by `ktexteditor`; enables speech support
  in `ktextwidgets`
* `qtmultimedia`: only enables barcode *scanning* in `prison`

Optional features that need a Qt module are switched off automatically when that
module is not installed (`ktextwidgets` without `qtspeech`, `prison` without
`qtmultimedia`). If a framework cannot be configured because a Qt module is
missing, the log says which one (with its aqtinstall name).

## Limitations / things to know

* **macOS universal builds do not work with Homebrew libraries.** They are
  single-architecture (`ld: ignoring file libintl.dylib: found architecture 'arm64',
  required architecture 'x86_64'`), so with `osx-architectures: x86_64;arm64` every
  framework that links one fails: `ki18n` (libintl), `kcalendarcore` (libical),
  `prison` (qrencode, libdmtx, zxing-cpp), `karchive` (zstd). Build those libraries as
  universal binaries yourself and point `CMAKE_PREFIX_PATH` at them, or build native.
* **macOS: use Homebrew's libxml2/libxslt, not the SDK's.** With both Xcode and the Command
  Line Tools installed (GitHub's runners) CMake finds the libraries in the CLT SDK and adds
  `-I<CLT SDK>/usr/include` in front of Xcode's libc++, which breaks every C++ standard header
  (`<cmath> tried including <math.h> but didn't find libc++'s <math.h>`). The action installs
  and prefers Homebrew's (keg-only, outside any SDK) for `kdoctools` and `kio`.
* **macOS install names** embed the absolute install prefix; bundling tools
  (macdeployqt, dylibbundler) take care of that when you package an app.
* **Source patches.** A few upstream bugs are fixed in the unpacked sources (`SOURCE_PATCHES` in
  `kf6.py`; each is idempotent and warns if the code it expects has changed upstream). Currently:
  `solid` calls the x86-only `__cpuid` on every MSVC target, which fails on Windows ARM64.
* **Windows (MSVC).** The libraries are not found by name, so the caller has to provide
  them and put their prefix into `CMAKE_PREFIX_PATH` (an exported `CMAKE_PREFIX_PATH` is
  honoured; `PATH` alone is *not* enough for CMake to find libraries). Needed:
  gettext/`libintl`, gperf, bison/flex, libical, libqrencode + libdmtx + zxing-cpp
  (for `prison`), libxml2 + libxslt, and the DocBook DTD/XSL (for `kdoctools`; pass
  their location with `framework-cmake-args`). `knotifications` needs
  [SnoreToast](https://github.com/KDE/snoretoast) (`LibSnoreToast`) there, and `kio`,
  `kjobwidgets` and `kwallet` need `knotifications`. The action itself handles the
  Windows specifics it can: breeze-icons' symlinks are unpacked the way Git for Windows
  checks them out without symlink support (small files holding the target's name, which
  is what its tools expect; real links and copies both break them), other tarballs get
  copies, `<prefix>\bin` is put on `PATH` while building (programs run during the
  build, e.g. `meinproc6`, need the DLLs of the frameworks installed so far), Windows'
  own `tar` is used for app templates (Git's GNU tar mistakes `D:/...` for a host), and
  zstd support in `karchive` is switched off when pkg-config cannot see `libzstd`. The
  installed breeze theme then lacks the alias icons (the placeholder files are not
  icons); the embedded icon library does include them.
* Frameworks whose CMake project needs libraries that are not packaged on the
  runner will fail in `all` mode; use `exclude`, `frameworks`, or
  `extra-system-packages`.
* Optional features of a framework (image formats, spell checkers, ...) are
  enabled only if their libraries are present when it is configured.
