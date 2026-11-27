# Updater V2 release publishing

## Bootstrap the signing keys

Generate the first root and online signing keys on a trusted machine, not in GitHub Actions:

```sh
python3 tools/updater_v2/bootstrap.py --private-dir "$HOME/.meshmc-updater-keys"
```

Commit only `launcher/updater/v2/keys/1.json`, `1.json.sig`, and `root-keys.txt`.
Keep `root-private.pem` offline in a secure backup. Add the contents of
`online-private.pem` to the `UPDATER_ONLINE_SIGNING_KEY` secret in the protected GitHub
`Release` environment, then securely remove the generated online-key working copy after
verifying the secret.

The workflow also needs `UPDATES_REPO_TOKEN`: a fine-grained token with **Contents: read
and write** on `Project-Tick/MeshMC-Updates`. It is only exposed to the protected publish
job. Restrict the `Release` environment to trusted branches (normally `master` and
`develop`) and require approval if that matches the release policy. Set the repository
Actions variable `UPDATER_PUBLISH_ENABLED` to `true` only after the public bootstrap files,
both secrets, and the requirement floors below are ready.

The first publish initializes the otherwise-empty updates repository with a short README,
then creates one `build-N` Release per source workflow run. `N` is allocated by
querying the updates repository for the highest existing `build-N` tag and adding
one, so it is independent of GitHub's `run_number`. Each Release contains
content-addressed `file-<sha256>` assets and the complete signed metadata snapshot needed
by the client. The Release is published only after all assets are uploaded. Pushes to
`master` advance all tiers, `testing` advances Testing and Latest, and `develop`
advances Latest.

The visible version is `major.minor.series`. The first published build for a new
major/minor pair uses series number 0. Later builds of that pair increment it,
including builds published by Testing and Stable pushes. The existing 12.0 series
keeps its original numbering. Release notes record the version and workflow run ID
so CI can allocate the next series number and reuse published numbers on a rerun.
The signed build manifest also carries the visible version.

Linux Qt 6 packages use the host Qt. MeshMC's `measure_linux_abi.py` reads the
package's ELF dependencies and symbol versions with GNU readelf in CI. The
signed manifest carries the aggregate requirements in `requires.abi`. The CI
artifact `abi-details-lin64-qt6` contains the source ELF path for each dependency
and version requirement, RPATH/RUNPATH entries, undefined dynamic symbols with
weak bindings marked, and warnings for private ABI. Versioned imports include
their library and version where readelf provides the version index. To generate
both outputs from an existing package:

```sh
python3 tools/updater_v2/measure_linux_abi.py path/to/MeshMC \
  --details abi-details.json > abi.json
```

At runtime the updater
checks for the interpreter and external libraries of the right ELF architecture,
using `LD_LIBRARY_PATH`, the glibc loader cache and standard library directories.
Version checks read the libraries' ELF version definitions without loading them.

Qt 5 installations select the Qt 6 successor when the ABI check succeeds and
otherwise stay on Qt 5. An older launcher first receives the Qt 5 build with
ABI support. The check covers direct ELF dependencies and required version
nodes, so Qt platform plugins and their runtime environment still need an
actual startup test.

The Windows Qt 6 and macOS universal Qt 6 variants bundle Qt and appear as
optional variants for supported systems.

The build workflow currently describes the Linux floor as glibc 2.19 / libstdc++ 3.4.21,
Windows 10.0, and macOS 12.0. Verify these minimums against the packaged binaries before
enabling production publishing; they are part of the signed update contract.
