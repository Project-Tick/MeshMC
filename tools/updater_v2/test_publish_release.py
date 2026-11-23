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
    _initialise_empty_repository,
    _next_tags,
    _reuse_draft_release,
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
                    "--variant", "linux-x86_64-qt5",
                    "--name", "Linux x86_64 Qt 5",
                    "--requirements",
                    '{"os":"linux","arch":"x86_64","glibc":"2.19","glibcxx":"3.4.21","qt":"5.15.2"}',
                ],
                check=True,
            )

            output = root / "output"
            output.mkdir()
            data, assets = make_build(payload_root, 42, "a" * 40, "2026-11-22T00:00:00Z", output)
            manifest = json.loads(data)
            variant = manifest["variants"]["linux-x86_64-qt5"]
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
    def test_master_advances_all_tiers_while_develop_advances_testing(self):
        previous = {"latest": 10, "testing": 9, "stable": 7}
        self.assertEqual(
            _next_tags(11, "develop", previous),
            {"latest": 11, "testing": 11, "stable": 7},
        )
        self.assertEqual(
            _next_tags(12, "master", {"latest": 11, "testing": 11, "stable": 7}),
            {"latest": 12, "testing": 12, "stable": 12},
        )


class DraftReleaseRetryTest(unittest.TestCase):
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
