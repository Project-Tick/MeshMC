#!/usr/bin/env python3

import argparse
import json
import os
from pathlib import Path
import tarfile


def main() -> None:
    parser = argparse.ArgumentParser(description="Archive one installed updater variant.")
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--variant", required=True)
    parser.add_argument("--name", required=True)
    parser.add_argument("--requirements", required=True)
    parser.add_argument("--abi", type=Path)
    parser.add_argument("--supersedes", action="append", default=[], metavar="VARIANT")
    args = parser.parse_args()

    source = args.source.resolve(strict=True)
    if not source.is_dir():
        parser.error(f"source is not a directory: {source}")
    try:
        requirements = json.loads(args.requirements)
    except json.JSONDecodeError as error:
        parser.error(f"--requirements is not valid JSON: {error}")
    if not isinstance(requirements, dict):
        parser.error("--requirements must be a JSON object")

    if args.abi:
        abi = json.loads(args.abi.read_text(encoding="utf-8"))
        if requirements.get("os") != "linux" or abi.get("arch") != requirements.get("arch"):
            parser.error("the measured ELF ABI does not match the Linux variant")
        if not any(lib["soname"] == "libQt6Core.so.6" for lib in abi["libs"]):
            parser.error("the Linux Qt 6 package must depend on system Qt 6")
        requirements["abi"] = abi
        for field, prefix in (("glibc", "GLIBC_"), ("glibcxx", "GLIBCXX_")):
            versions = [version[len(prefix):] for lib in abi["libs"] for version in lib["versions"]
                        if version.startswith(prefix) and all(part.isdigit() for part in version[len(prefix):].split("."))]
            if not versions:
                parser.error(f"cannot determine {field} from the ELF ABI")
            requirements[field] = max(versions, key=lambda version: tuple(map(int, version.split("."))))

    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    metadata = {
        "id": args.variant,
        "name": args.name,
        "supersedes": args.supersedes,
        "requires": requirements,
    }
    (output / "variant.json").write_text(
        json.dumps(metadata, separators=(",", ":")) + "\n",
        encoding="utf-8",
    )

    archive_path = output / "payload.tar.gz"
    with tarfile.open(archive_path, "w:gz", dereference=False) as archive:
        for root, dirs, files in os.walk(source, followlinks=False):
            dirs.sort()
            files.sort()
            root_path = Path(root)
            for name in list(dirs):
                path = root_path / name
                if path.is_symlink():
                    dirs.remove(name)
                    archive.add(path, arcname=path.relative_to(source).as_posix(), recursive=False)
            for name in files:
                path = root_path / name
                archive.add(path, arcname=path.relative_to(source).as_posix(), recursive=False)


if __name__ == "__main__":
    main()
