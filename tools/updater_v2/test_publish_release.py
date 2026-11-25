import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tarfile
import tempfile
import unittest
from unittest.mock import patch

from tools.updater_v2.publish_release import (
    _add_metadata_assets,
    _check_asset_limit,
    _check_variant_continuity,
    _download_manifest,
    _create_draft_release,
    _initialise_empty_repository,
    _next_tags,
    _publish_draft,
    _resume_published_release,
    _reuse_draft_release,
    _retained_builds,
    _sync_historical_builds,
)
from tools.updater_v2.publish_release import make_build, public_key, verify_signature


ROOT = Path(__file__).resolve().parents[2]


class PreparePayloadTest(unittest.TestCase):
    def test_build_manifest_uses_content_assets_and_preserves_links(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            source = root / "install"
            source.mkdir()
            executable = source / "meshmc"
            executable.write_bytes(b"launcher")
            executable.chmod(0o755)
            signature_marker = (
                source / "MeshMC.app/Contents/Frameworks/QtCore.framework/"
                "_CodeSignature/CodeSignature"
            )
            signature_marker.parent.mkdir(parents=True)
            signature_marker.touch()
            (source / "current").symlink_to("meshmc")

            payload_root = root / "artifacts"
            payload = payload_root / "updater-linux"
            subprocess.run(
                [
                    sys.executable,
                    str(ROOT / "tools/updater_v2/prepare_payload.py"),
                    "--source", str(source),
                    "--output", str(payload),
                    "--variant", "linux-x86_64-qt5.6",
                    "--name", "Linux x86_64 Qt 5",
                    "--supersedes", "linux-x86_64-qt5",
                    "--requirements",
                    '{"os":"linux","arch":"x86_64","glibc":"2.19","glibcxx":"3.4.21","qt":"5.6.3"}',
                ],
                check=True,
            )

            output = root / "output"
            output.mkdir()
            data, assets = make_build(payload_root, 42, "a" * 40, "2026-11-22T00:00:00Z", output)
            manifest = json.loads(data)
            variant = manifest["variants"]["linux-x86_64-qt5.6"]
            self.assertEqual(variant["supersedes"], ["linux-x86_64-qt5"])
            self.assertEqual(variant["files"][0]["sha256"], hashlib.sha256(b"launcher").hexdigest())
            self.assertTrue(variant["files"][0]["executable"])
            self.assertEqual(variant["files"][0]["asset_url"],
                             "https://github.com/Project-Tick/MeshMC-Updates/releases/download/"
                             "build-42/file-" + hashlib.sha256(b"launcher").hexdigest())
            self.assertEqual(variant["links"], [{"path": "current", "target": "meshmc"}])
            self.assertEqual((assets / ("file-" + hashlib.sha256(b"launcher").hexdigest())).read_bytes(), b"launcher")
            self.assertNotIn("MeshMC.app/Contents/Frameworks/QtCore.framework/_CodeSignature/CodeSignature",
                             {entry["path"] for entry in variant["files"]})
            self.assertNotIn("file-" + hashlib.sha256(b"").hexdigest(),
                             {asset.name for asset in assets.iterdir()})

    def test_rejects_unexpected_zero_byte_file(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            source = root / "install"
            source.mkdir()
            (source / "unexpected-empty").touch()

            payload_root = root / "artifacts"
            payload = payload_root / "updater-linux"
            subprocess.run(
                [
                    sys.executable,
                    str(ROOT / "tools/updater_v2/prepare_payload.py"),
                    "--source", str(source),
                    "--output", str(payload),
                    "--variant", "linux-x86_64-qt5",
                    "--name", "Linux x86_64 Qt 5",
                    "--requirements",
                    '{"os":"linux","arch":"x86_64","glibc":"2.19","glibcxx":"3.4.21","qt":"5.15.2"}',
                ],
                check=True,
            )

            output = root / "output"
            output.mkdir()
            with self.assertRaisesRegex(ValueError, "zero-byte updater file"):
                make_build(payload_root, 42, "a" * 40, "2026-11-22T00:00:00Z", output)

    def test_rejects_archive_path_traversal(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            payload_root = root / "artifacts"
            payload = payload_root / "updater-linux"
            payload.mkdir(parents=True)
            (payload / "variant.json").write_text(
                json.dumps({
                    "id": "linux-x86_64-qt5",
                    "name": "Linux",
                    "supersedes": [],
                    "requires": {"os": "linux", "arch": "x86_64", "glibc": "2.19",
                                 "glibcxx": "3.4.21", "qt": "5.15.2"},
                }),
                encoding="utf-8",
            )
            with tarfile.open(payload / "payload.tar.gz", "w:gz") as archive:
                info = tarfile.TarInfo("../outside")
                info.size = 1
                import io
                archive.addfile(info, io.BytesIO(b"x"))

            output = root / "output"
            output.mkdir()
            with self.assertRaisesRegex(ValueError, "unsafe or unsupported updater path"):
                make_build(payload_root, 1, "b" * 40, "2026-11-22T00:00:00Z", output)


class BootstrapTest(unittest.TestCase):
    def test_bootstrap_outputs_root_signed_public_floor_and_private_online_key(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            private_dir = root / "private"
            public_dir = root / "public"
            subprocess.run(
                [
                    sys.executable,
                    str(ROOT / "tools/updater_v2/bootstrap.py"),
                    "--private-dir", str(private_dir),
                    "--public-dir", str(public_dir),
                ],
                check=True,
                stdout=subprocess.PIPE,
                text=True,
            )
            floor_data = (public_dir / "1.json").read_bytes()
            floor = json.loads(floor_data)
            self.assertEqual(floor["type"], "keys")
            verify_signature(
                floor_data,
                (public_dir / "1.json.sig").read_bytes(),
                floor["root_keys"],
                root,
            )
            self.assertEqual(public_key(private_dir / "online-private.pem").hex(), floor["online_keys"][0])
            self.assertEqual((public_dir / "root-keys.txt").read_text().strip(), floor["root_keys"][0])

            metadata = root / "metadata"
            metadata.mkdir()
            history = root / "history"
            history.mkdir()
            build_data = b'{"type":"build","format":1,"build":1}\n'
            assets = _add_metadata_assets(
                metadata,
                floor_data,
                (public_dir / "1.json.sig").read_bytes(),
                build_data,
                1,
                "master",
                floor["online_keys"][0],
                private_dir / "online-private.pem",
                history,
                None,
                True,
            )
            for name in (
                "updater--mirrors.json",
                "updater--tags.json",
                "updater--builds--1.json",
            ):
                document = metadata / name
                signature = Path(str(document) + ".sig")
                verify_signature(
                    document.read_bytes(),
                    signature.read_bytes(),
                    floor["online_keys"],
                    root,
                )
                self.assertIn(document, assets)
                self.assertIn(signature, assets)


class TierTagsTest(unittest.TestCase):
    def test_develop_advances_only_latest(self):
        previous = {"latest": 10, "testing": 9, "stable": 7}
        self.assertEqual(
            _next_tags(11, "develop", previous),
            {"latest": 11, "testing": 9, "stable": 7},
        )

    def test_testing_advances_latest_and_testing(self):
        previous = {"latest": 11, "testing": 9, "stable": 7}
        self.assertEqual(
            _next_tags(12, "testing", previous),
            {"latest": 12, "testing": 12, "stable": 7},
        )

    def test_master_advances_all_tiers(self):
        previous = {"latest": 12, "testing": 12, "stable": 7}
        self.assertEqual(
            _next_tags(13, "master", previous),
            {"latest": 13, "testing": 13, "stable": 13},
        )

    def test_rejects_unsupported_branches(self):
        previous = {"latest": 10, "testing": 9, "stable": 7}
        for branch in ("feature-example", "release-12.0", "main"):
            with self.subTest(branch=branch):
                with self.assertRaisesRegex(ValueError, "unsupported branch"):
                    _next_tags(11, branch, previous)

    def test_preserves_tiers_when_advance_is_disabled(self):
        previous = {"latest": 10, "testing": 9, "stable": 7}
        for branch in ("master", "testing", "develop"):
            with self.subTest(branch=branch):
                self.assertEqual(
                    _next_tags(11, branch, previous, advance=False),
                    previous,
                )


class ManifestRetentionTest(unittest.TestCase):
    def test_keeps_newest_builds_and_reserves_slots_for_active_tiers(self):
        retained = _retained_builds(set(range(1, 101)), {1, 2, 101})
        self.assertEqual(retained, {1, 2, *range(54, 102)})

    def test_asset_limit_includes_payload_and_metadata(self):
        _check_asset_limit("build-101", 1000)
        with self.assertRaisesRegex(ValueError, "1001 assets"):
            _check_asset_limit("build-101", 1001)

    @patch("tools.updater_v2.publish_release.verify_signature")
    @patch("tools.updater_v2.publish_release.command")
    def test_download_requests_only_the_selected_manifest_and_signature(self, command, verify):
        def download(*args):
            target = Path(args[-1])
            (target / "updater--builds--10.json").write_bytes(b"manifest")
            (target / "updater--builds--10.json.sig").write_bytes(b"signature")

        command.side_effect = download
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            document, signature = _download_manifest("repo", {"tag_name": "build-100"}, 10, root, ["key"])
            self.assertEqual(command.call_args.args[6:-2], (
                "--pattern", document.name, "--pattern", signature.name,
            ))
            verify.assert_called_once_with(b"manifest", b"signature", ["key"], root / "build-10")

    @patch("tools.updater_v2.publish_release.command")
    @patch("tools.updater_v2.publish_release._download_history")
    @patch("tools.updater_v2.publish_release._release_list")
    @patch("tools.updater_v2.publish_release._previous_tags")
    def test_sync_replaces_oldest_unpinned_manifest_with_late_build(self, tags, releases, download, command):
        def assets(number):
            name = f"updater--builds--{number}.json"
            return [{"name": name + suffix, "id": number * 2 + index}
                    for index, suffix in enumerate(("", ".sig"))]

        late = {"tag_name": "build-99"}
        releases.return_value = [late, {"tag_name": "build-2"}]
        tags.return_value = {"latest": 100, "testing": 100, "stable": 1}
        newest = {"tag_name": "build-100", "assets": [
            asset for number in (1, *range(51, 99), 100) for asset in assets(number)
        ]}
        download.return_value = [Path(asset["name"]) for asset in assets(99)]
        with tempfile.TemporaryDirectory() as temporary:
            work = Path(temporary) / "sync"
            _sync_historical_builds("repo", newest, "key", work)
            download.assert_called_once_with("repo", {99: late}, work, ["key"])
        self.assertEqual(
            {call.args[-1] for call in command.call_args_list if call.args[1] == "api"},
            {"repos/repo/releases/assets/102", "repos/repo/releases/assets/103"},
        )
        self.assertEqual(command.call_args.args, (
            "gh", "release", "upload", "build-100",
            "updater--builds--99.json", "updater--builds--99.json.sig", "--repo", "repo",
        ))


class VariantContinuityTest(unittest.TestCase):
    def test_accepts_windows_and_macos_compatibility_routes(self):
        previous = {"build": 8, "variants": {
            "windows-x86_64-qt5": {"requires": {"os": "windows", "arch": "x86_64", "os_version": "10.0"}},
            "macos-aarch64-qt5": {"requires": {"os": "macos", "arch": "aarch64", "os_version": "12.0"}},
        }}
        candidate = {"build": 10, "variants": {
            "windows-x86-qt5.15": {
                "supersedes": ["windows-x86-qt5.6", "windows-x86_64-qt5"],
                "requires": {"os": "windows", "arch": "x86", "os_version": "6.1"},
            },
            "macos-x86_64-qt5.15": {
                "supersedes": ["macos-aarch64-qt5"],
                "requires": {"os": "macos", "arch": "x86_64", "os_version": "10.13"},
            },
        }}
        _check_variant_continuity(previous, candidate)

        mac_requirements = candidate["variants"]["macos-x86_64-qt5.15"]["requires"]
        with patch.dict(mac_requirements, {"os_version": "13.0"}):
            with self.assertRaisesRegex(ValueError, "missing compatible variants for macos-aarch64-qt5"):
                _check_variant_continuity(previous, candidate)

    def test_compatible_successor_covers_existing_installations(self):
        previous = {"build": 1, "variants": {
            "linux-x86_64-qt5": {"requires": {
                "os": "linux", "arch": "x86_64", "glibc": "2.19", "glibcxx": "3.4.21", "qt": "5.15.2",
            }},
        }}
        successor = {"supersedes": ["linux-x86_64-qt5"], "requires": {
            "os": "linux", "arch": "x86_64", "glibc": "2.19.0", "glibcxx": "3.4.21", "qt": "5.6.3",
        }}
        candidate = {"build": 2, "variants": {"linux-x86_64-qt5.6": successor}}
        _check_variant_continuity(previous, candidate)

        for requirements in ({"glibc": "2.20"}, {"qt": "4.8.7"}, {"arch": "x86"}):
            with self.subTest(requirements=requirements):
                with patch.dict(successor["requires"], requirements):
                    with self.assertRaisesRegex(ValueError, "missing compatible variants for linux-x86_64-qt5"):
                        _check_variant_continuity(previous, candidate)

        successor["supersedes"] = []
        with self.assertRaisesRegex(ValueError, "missing compatible variants for linux-x86_64-qt5"):
            _check_variant_continuity(previous, candidate)

    def test_rejects_cycles_and_ambiguous_successors(self):
        previous = {"build": 1, "variants": {}}
        for links, error in (
            ({"one": ["two"], "two": ["one"]}, "cycle"),
            ({"one": ["old"], "two": ["old"]}, "multiple successors"),
        ):
            with self.subTest(links=links):
                candidate = {"build": 2, "variants": {
                    name: {"supersedes": predecessors} for name, predecessors in links.items()
                }}
                with self.assertRaisesRegex(ValueError, error):
                    _check_variant_continuity(previous, candidate)


class DraftReleaseRetryTest(unittest.TestCase):
    @patch("tools.updater_v2.publish_release._finish_release")
    @patch("tools.updater_v2.publish_release._download_manifest")
    def test_resumes_history_only_for_the_same_signed_build(self, download, finish):
        release = {"tag_name": "build-42"}
        candidate = {"build": 42, "commit": "a" * 40, "variants": {}, "built": "new timestamp"}
        with tempfile.TemporaryDirectory() as temporary:
            work = Path(temporary)
            document = work / "build.json"
            document.write_text(json.dumps({**candidate, "built": "old timestamp"}))
            download.return_value = (document, work / "build.json.sig")
            finish.return_value = 41

            self.assertEqual(
                _resume_published_release("repo", release, json.dumps(candidate).encode(), "key", work),
                41,
            )
            download.assert_called_once_with("repo", release, 42, work, ["key"])
            finish.assert_called_once_with("repo", "build-42", "key", work)
            finish.reset_mock()
            with self.assertRaisesRegex(ValueError, "does not match"):
                different = {**candidate, "commit": "b" * 40}
                _resume_published_release("repo", release, json.dumps(different).encode(), "key", work)
            finish.assert_not_called()

    @patch("tools.updater_v2.publish_release.command")
    def test_make_latest_is_sent_as_a_string(self, command):
        _publish_draft("repo", {"id": 123}, False)
        self.assertEqual(command.call_args.args[-2:], ("-f", "make_latest=false"))

    @patch("tools.updater_v2.publish_release.command")
    def test_reuses_draft_after_removing_partial_assets(self, command):
        release = {"draft": True, "assets": [{"id": 123}]}
        self.assertTrue(_reuse_draft_release("Project-Tick/MeshMC-Updates", "build-1", release))
        command.assert_called_once_with(
            "gh", "api", "--method", "DELETE",
            "repos/Project-Tick/MeshMC-Updates/releases/assets/123",
        )

    def test_refuses_to_replace_published_release(self):
        with self.assertRaisesRegex(ValueError, "already exists and is published"):
            _reuse_draft_release("Project-Tick/MeshMC-Updates", "build-1", {"draft": False})

    @patch("tools.updater_v2.publish_release._gh_json")
    def test_uses_created_draft_response(self, gh_json):
        draft = {"id": 123, "tag_name": "build-1", "draft": True}
        gh_json.return_value = draft

        self.assertIs(
            _create_draft_release(
                "Project-Tick/MeshMC-Updates",
                "build-1",
                "MeshMC updater build 1",
                "Source commit: abc\nBranch: develop",
            ),
            draft,
        )
        gh_json.assert_called_once_with(
            "--method", "POST",
            "repos/Project-Tick/MeshMC-Updates/releases",
            "-f", "tag_name=build-1",
            "-F", "draft=true",
            "-f", "name=MeshMC updater build 1",
            "-f", "body=Source commit: abc\nBranch: develop",
            "-f", "target_commitish=main",
        )


class EmptyRepositoryInitTest(unittest.TestCase):
    @patch("tools.updater_v2.publish_release.command")
    def test_existing_main_branch_is_not_reinitialized(self, command):
        command.return_value = subprocess.CompletedProcess(
            ("git", "ls-remote"), 0, "abc123\trefs/heads/main\n", "",
        )

        _initialise_empty_repository("Project-Tick/MeshMC-Updates")

        self.assertFalse(any(call.args[:2] == ("git", "push") for call in command.call_args_list))
        self.assertFalse(any(call.args[:2] == ("git", "commit") for call in command.call_args_list))

    @patch("tools.updater_v2.publish_release.command")
    def test_concurrent_main_creation_does_not_fail_initialization(self, command):
        remote_checks = iter(("", "abc123\trefs/heads/main\n"))

        def fake_command(*args, capture=False, cwd=None):
            if args[:2] == ("git", "ls-remote"):
                return subprocess.CompletedProcess(args, 0, next(remote_checks), "")
            if args[:2] == ("git", "push"):
                raise subprocess.CalledProcessError(1, args)
            return subprocess.CompletedProcess(args, 0, "", "")

        command.side_effect = fake_command

        _initialise_empty_repository("Project-Tick/MeshMC-Updates")

        pushes = [call for call in command.call_args_list if call.args[:2] == ("git", "push")]
        self.assertEqual(len(pushes), 1)


if __name__ == "__main__":
    unittest.main()
