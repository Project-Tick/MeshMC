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
then creates one `build-N` Release per source workflow run. Each Release contains
content-addressed `file-<sha256>` assets and the complete signed metadata snapshot needed
by the client. The Release is published only after all assets are uploaded. Pushes to
`master` advance all tiers; `develop` and `release-*` advance Testing and Latest; other
branches advance Latest only.

The build workflow currently describes the Linux floor as glibc 2.19 / libstdc++ 3.4.21,
Windows 10.0, and macOS 12.0. Verify these minimums against the packaged binaries before
enabling production publishing; they are part of the signed update contract.
