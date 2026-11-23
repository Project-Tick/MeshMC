import os
import tempfile
import tarfile
import unittest
import zipfile
from unittest.mock import patch
from pathlib import Path

from tools.package_release import create_archive


class PackageReleaseTest(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.root = Path(self.temporary.name)
        self.source = self.root / "install"
        self.source.mkdir()
        (self.source / "bin").mkdir()
        (self.source / "bin" / "MeshMC").write_text("launcher", encoding="utf-8")
        (self.source / "bin" / "MeshMC").chmod(0o755)
        (self.source / ".hidden").write_text("included", encoding="utf-8")
        (self.root / "MeshMC.dmg").write_text("not part of install", encoding="utf-8")

    def tearDown(self):
        self.temporary.cleanup()

    def test_tarball_contains_only_install_tree_under_named_root(self):
        output = self.root / "MeshMC-Linux.tar.gz"
        create_archive(self.source, output, "tar.gz", "MeshMC")

        with tarfile.open(output) as archive:
            self.assertEqual(
                set(archive.getnames()),
                {"MeshMC", "MeshMC/.hidden", "MeshMC/bin", "MeshMC/bin/MeshMC"},
            )
            self.assertEqual(archive.extractfile("MeshMC/bin/MeshMC").read(), b"launcher")
            self.assertTrue(archive.getmember("MeshMC/bin/MeshMC").mode & 0o111)

    def test_tarball_preserves_symlinks(self):
        (self.source / "MeshMC-link").symlink_to("bin/MeshMC")
        output = self.root / "MeshMC-Linux.tar.gz"
        create_archive(self.source, output, "tar.gz", "MeshMC")

        with tarfile.open(output) as archive:
            link = archive.getmember("MeshMC/MeshMC-link")
            self.assertTrue(link.issym())
            self.assertEqual(link.linkname, "bin/MeshMC")

    def test_zip_contains_hidden_files_under_named_root(self):
        output = self.root / "MeshMC-Windows.zip"
        create_archive(self.source, output, "zip", "MeshMC")

        with zipfile.ZipFile(output) as archive:
            self.assertEqual(
                set(archive.namelist()),
                {"MeshMC/", "MeshMC/.hidden", "MeshMC/bin/", "MeshMC/bin/MeshMC"},
            )
            self.assertEqual(archive.read("MeshMC/.hidden"), b"included")

    def test_archive_root_is_portable(self):
        for name in ("../escape", r"..\escape", "C:MeshMC"):
            with self.subTest(name=name), self.assertRaises(ValueError):
                create_archive(self.source, self.root / "out.zip", "zip", name)

    def test_failed_write_preserves_existing_package(self):
        output = self.root / "out.zip"
        output.write_bytes(b"existing package")
        with patch("tools.package_release.zipfile.ZipFile.write", side_effect=OSError("write failed")):
            with self.assertRaisesRegex(OSError, "write failed"):
                create_archive(self.source, output, "zip", "MeshMC")
        self.assertEqual(output.read_bytes(), b"existing package")
        self.assertEqual(list(self.root.glob(".package-*")), [])

    @unittest.skipUnless(hasattr(os, "mkfifo"), "requires POSIX FIFOs")
    def test_special_files_are_rejected(self):
        os.mkfifo(self.source / "pipe")
        for archive_format in ("zip", "tar.gz"):
            with self.subTest(archive_format=archive_format), self.assertRaisesRegex(ValueError, "unsupported file type"):
                create_archive(self.source, self.root / "out", archive_format, "MeshMC")


if __name__ == "__main__":
    unittest.main()
