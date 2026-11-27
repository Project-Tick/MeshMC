"""Allocate the updater build ID and the user-visible version in CI."""

import argparse
import json
from pathlib import Path
import re
import sys


def allocate(cmake, refs, releases, run_id):
    components = []
    for part in ("MAJOR", "MINOR"):
        match = re.search(rf"^set\(Launcher_VERSION_{part}\s+([0-9]+)\)", cmake, re.M)
        if not match:
            raise ValueError(f"CMakeLists.txt does not define Launcher_VERSION_{part}")
        components.append(match[1])
    series = ".".join(components)
    max_build = max(
        (int(match[1]) for item in refs
         if (match := re.fullmatch(r"refs/tags/build-([1-9][0-9]*)", item["ref"]))),
        default=0,
    )
    last_number = -1
    for release in releases:
        if release["draft"] or release["prerelease"]:
            continue
        tag = re.fullmatch(r"build-([1-9][0-9]*)", release["tag_name"])
        if not tag:
            continue
        max_build = max(max_build, int(tag[1]))
        body = release.get("body") or ""
        version = re.search(r"^Version: ([0-9]+\.[0-9]+)\.([0-9]+)\r?$", body, re.M)
        run = re.search(r"^Workflow run: ([0-9]+)\r?$", body, re.M)
        if run and run[1] == run_id:
            if not version or version[1] != series:
                raise ValueError(f"Version metadata mismatch for workflow run {run_id}")
            return int(tag[1]), f"{series}.{version[2]}"
        if version and version[1] == series:
            last_number = max(last_number, int(version[2]))

    build = max_build + 1
    # Existing 12.0 installs already use the global build ID as their third component.
    number = build if series == "12.0" else last_number + 1
    if number > 65535:
        raise ValueError(f"Version component {number} exceeds the Windows limit of 65535")
    if build > 2147483647:
        raise ValueError(f"Build ID {build} exceeds the launcher's integer limit")
    return build, f"{series}.{number}"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("refs", type=Path)
    parser.add_argument("releases", type=Path)
    parser.add_argument("run_id")
    args = parser.parse_args()
    build, version = allocate(
        Path("CMakeLists.txt").read_text(),
        json.loads(args.refs.read_text()),
        json.loads(args.releases.read_text()),
        args.run_id,
    )
    sys.stdout.write(f"build-number={build}\nversion={version}\nseries-number={version.rsplit('.', 1)[1]}\n")


if __name__ == "__main__":
    main()
