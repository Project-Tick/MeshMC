#!/usr/bin/env python3

import argparse
import os
import stat
import tempfile
from pathlib import Path
import tarfile
import zipfile


def _entries(source: Path):
    def raise_walk_error(error):
        raise error

    for root, dirs, files in os.walk(source, followlinks=False, onerror=raise_walk_error):
        dirs.sort()
        files.sort()
        root_path = Path(root)
        for name in list(dirs):
            path = root_path / name
            if path.is_symlink():
                dirs.remove(name)
            yield path
        for name in files:
            yield root_path / name


def create_archive(source: Path, output: Path, archive_format: str, archive_root: str) -> None:
    source = source.resolve(strict=True)
    if not source.is_dir():
        raise ValueError(f"source is not a directory: {source}")
    if archive_root in ("", ".", "..") or any(char in archive_root for char in "/\\:\0"):
        raise ValueError(f"archive root must be a single directory name: {archive_root}")
    root = Path(archive_root)
    output = output.resolve()
    if output.is_relative_to(source):
        raise ValueError(f"output must not be inside the source directory: {output}")
    if archive_format not in ("zip", "tar.gz"):
        raise ValueError(f"unsupported archive format: {archive_format}")

    entries = list(_entries(source))
    if not entries:
        raise ValueError(f"source directory is empty: {source}")
    for path in entries:
        mode = path.lstat().st_mode
        if stat.S_ISLNK(mode):
            if archive_format == "zip":
                raise ValueError(f"ZIP packages do not support symbolic links: {path}")
        elif not (stat.S_ISREG(mode) or stat.S_ISDIR(mode)):
            raise ValueError(f"unsupported file type in package: {path}")

    output.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix=".package-", dir=output.parent) as directory:
        temporary = Path(directory) / output.name
        _write_archive(source, temporary, archive_format, root, entries)
        temporary.replace(output)


def _write_archive(source: Path, output: Path, archive_format: str, root: Path, entries: list[Path]) -> None:
    if archive_format == "tar.gz":
        with tarfile.open(output, "w:gz", dereference=False) as archive:
            archive.add(source, arcname=root.as_posix(), recursive=False)
            for path in entries:
                archive.add(path, arcname=(root / path.relative_to(source)).as_posix(), recursive=False)
    else:
        with zipfile.ZipFile(output, "w", compression=zipfile.ZIP_DEFLATED, strict_timestamps=False) as archive:
            root_entry = zipfile.ZipInfo(root.as_posix() + "/")
            root_entry.external_attr = (0o40755 << 16) | 0x10
            archive.writestr(root_entry, b"")
            for path in entries:
                name = (root / path.relative_to(source)).as_posix()
                if path.is_dir():
                    entry = zipfile.ZipInfo(name + "/")
                    entry.external_attr = (0o40755 << 16) | 0x10
                    archive.writestr(entry, b"")
                else:
                    archive.write(path, name)


def main() -> None:
    parser = argparse.ArgumentParser(description="Package a clean platform install tree.")
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--format", choices=("tar.gz", "zip"), required=True)
    parser.add_argument("--archive-root", required=True)
    args = parser.parse_args()
    try:
        create_archive(args.source, args.output, args.format, args.archive_root)
    except (OSError, tarfile.TarError, ValueError, zipfile.BadZipFile) as error:
        parser.error(str(error))


if __name__ == "__main__":
    main()
