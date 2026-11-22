import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tarfile
import tempfile
import unittest

from tools.updater_v2.publish_release import _add_metadata_assets, _next_tags
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


if __name__ == "__main__":
    unittest.main()
