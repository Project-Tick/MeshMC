#!/usr/bin/env python3
"""Measure external ELF requirements of a Linux x86_64 launcher package."""

import argparse
from collections import defaultdict
import json
import os
from pathlib import Path
import re
import subprocess


def inspect_elf(path: Path, report: str) -> dict:
    tags = defaultdict(list)
    for tag, value in re.findall(r"\((NEEDED|SONAME|RPATH|RUNPATH)\).*?\[([^\]]*)\]", report):
        tags[tag].append(value)
    versions = {}
    dependency = None
    in_needs = False
    for line in report.splitlines():
        if line.startswith("Version ") and " section " in line:
            in_needs = line.startswith("Version needs section ")
            dependency = None
        if not in_needs:
            continue
        match = re.search(r"\bFile: (\S+)", line)
        if match:
            dependency = match[1]
        match = re.search(r"\bName: (\S+)\s+Flags: (.*?)\s+Version: (\d+)", line)
        if dependency and match:
            versions[int(match[3])] = {
                "soname": dependency, "version": match[1], "weak": "WEAK" in match[2].split()
            }

    imports = []
    for line in report.splitlines():
        columns = line.split(maxsplit=7)
        if len(columns) != 8 or not columns[0].rstrip(":").isdigit() or columns[6] != "UND":
            continue
        name, _, version_index = columns[7].partition(" (")
        symbol = {"name": name.split("@", 1)[0], "weak": columns[4] == "WEAK"}
        version = versions.get(int(version_index.rstrip(")"))) if version_index.endswith(")") else None
        if version:
            symbol.update(soname=version["soname"], version=version["version"])
        imports.append(symbol)

    interpreter = re.search(r"\[Requesting program interpreter: ([^\]]+)\]", report)
    return {
        "path": path.as_posix(),
        "interpreter": interpreter[1] if interpreter else None,
        "soname": next(iter(tags["SONAME"]), None),
        "needed": tags["NEEDED"],
        "rpath": tags["RPATH"],
        "runpath": tags["RUNPATH"],
        "version_needs": list(versions.values()),
        "imports": imports,
    }


def measure(root: Path, *, details=None) -> dict:
    bundled = set()
    needed = defaultdict(set)
    interpreters = set()
    objects = []
    warnings = []
    env = dict(os.environ, LC_ALL="C")
    for path in sorted(root.rglob("*")):
        if path.is_symlink() or not path.is_file():
            continue
        with path.open("rb") as executable:
            header = executable.read(20)
        if not header.startswith(b"\x7fELF"):
            continue
        if len(header) != 20 or header[4:6] != b"\x02\x01" or header[18:20] != b"\x3e\x00":
            raise ValueError(f"expected a little-endian x86_64 ELF file: {path}")
        command = ["readelf", "-W", "-d", "-l", "-V"]
        if details is not None:
            command.append("--dyn-syms")
        report = subprocess.run(command + [str(path)], check=True, text=True, env=env,
                                capture_output=True)
        if report.stderr:
            raise ValueError(f"cannot inspect {path}: {report.stderr.strip()}")
        elf = inspect_elf(path.relative_to(root), report.stdout)
        objects.append(elf)
        bundled.add(path.name)
        if elf["soname"]:
            bundled.add(elf["soname"])
        if elf["interpreter"]:
            interpreters.add(elf["interpreter"])
        for name in elf["needed"]:
            needed[name]
        for requirement in elf["version_needs"]:
            version = requirement["version"]
            if version.endswith("_PRIVATE_API") or version == "GLIBC_PRIVATE":
                warnings.append({
                    "path": elf["path"], "soname": requirement["soname"], "version": version,
                    "message": "Private ABI version nodes do not establish binary compatibility.",
                })
            if not requirement["weak"]:
                needed[requirement["soname"]].add(version)
    if len(interpreters) != 1:
        raise ValueError("expected one ELF interpreter in the launcher package")
    requirements = {
        "format": 1,
        "arch": "x86_64",
        "interpreter": interpreters.pop(),
        "libs": [
            {"soname": name, "versions": sorted(needed[name])}
            for name in sorted(needed.keys() - bundled)
        ],
    }
    if details is not None:
        details.update(format=1, requirements=requirements, objects=objects,
                       warnings=warnings, bundled_names=sorted(bundled))
    return requirements


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("package", type=Path)
    parser.add_argument("--details", type=Path, help="write a per-ELF diagnostic report as JSON")
    args = parser.parse_args()
    if not args.package.is_dir():
        parser.error("package must be a directory")
    try:
        details = {} if args.details else None
        requirements = measure(args.package, details=details)
        if args.details:
            args.details.write_text(json.dumps(details, indent=2) + "\n", encoding="utf-8")
        print(json.dumps(requirements, separators=(",", ":")))
    except (OSError, ValueError, subprocess.CalledProcessError) as error:
        parser.error(str(error))


if __name__ == "__main__":
    main()
