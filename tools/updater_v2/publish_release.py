#!/usr/bin/env python3

import argparse
from concurrent.futures import ThreadPoolExecutor
import datetime as dt
import hashlib
from itertools import zip_longest
import json
import os
import posixpath
from pathlib import Path
import re
import shutil
import stat
import subprocess
import tarfile
import tempfile
import time


ASSET_BASE = "https://github.com/Project-Tick/MeshMC-Updates/releases/download/"
LATEST_BASE = "https://github.com/Project-Tick/MeshMC-Updates/releases/latest/download/"
MAX_RELEASE_ASSETS = 1000
MAX_BUILD_MANIFESTS = 50
HISTORY_WORKERS = 4
KEYS_FILE = "launcher/updater/v2/keys/1.json"
KEYS_SIGNATURE_FILE = KEYS_FILE + ".sig"
BUILD_ASSET = re.compile(r"updater--builds--([1-9][0-9]*)\.json(?:\.sig)?\Z")
VARIANT_ID = re.compile(r"([a-z0-9_]+)-([a-z0-9_]+)-[a-z0-9.]+(?:-[a-z0-9.]+)*\Z")
VERSION = re.compile(r"(?:0|[1-9][0-9]*)(?:\.(?:0|[1-9][0-9]*))*\Z")
WORD = re.compile(r"[a-z0-9_]+\Z")


def command(*args, capture=False, cwd=None):
    return subprocess.run(
        args,
        check=True,
        cwd=cwd,
        text=capture,
        stdout=subprocess.PIPE if capture else None,
    )


def timestamp() -> str:
    return dt.datetime.now(dt.timezone.utc).replace(microsecond=0).isoformat().replace("+00:00", "Z")


def public_key(private_key: Path) -> bytes:
    result = subprocess.run(
        ["openssl", "pkey", "-in", str(private_key), "-pubout", "-outform", "DER"],
        check=True,
        stdout=subprocess.PIPE,
    )
    der = result.stdout
    prefix = bytes.fromhex("302a300506032b6570032100")
    if len(der) != 44 or der[:12] != prefix:
        raise ValueError(f"{private_key} is not an Ed25519 key")
    return der[-32:]


def public_pem(key: bytes, path: Path) -> None:
    der = bytes.fromhex("302a300506032b6570032100") + key
    import base64

    encoded = base64.b64encode(der).decode("ascii")
    lines = "\n".join(encoded[i:i + 64] for i in range(0, len(encoded), 64))
    path.write_text(f"-----BEGIN PUBLIC KEY-----\n{lines}\n-----END PUBLIC KEY-----\n", encoding="ascii")


def sign(private_key: Path, data: bytes, output: Path) -> bytes:
    with tempfile.NamedTemporaryFile() as source:
        source.write(data)
        source.flush()
        command(
            "openssl", "pkeyutl", "-sign", "-rawin", "-inkey", str(private_key),
            "-in", source.name, "-out", str(output),
        )
    return output.read_bytes()


def verify_signature(data: bytes, signature_file: bytes, trusted: list[str], work: Path) -> None:
    if len(signature_file) > 8 * 194 or not signature_file.endswith(b"\n"):
        raise ValueError("signature file is too large or has no final newline")
    trusted_lines = set()
    lines = signature_file[:-1].split(b"\n")
    if not 1 <= len(lines) <= 8:
        raise ValueError("signature file has an invalid number of lines")
    seen = set()
    for line in lines:
        match = re.fullmatch(rb"([0-9a-f]{64}) ([0-9a-f]{128})", line)
        if not match:
            raise ValueError("malformed signature file")
        key_hex = match.group(1).decode("ascii")
        if key_hex in seen:
            raise ValueError("a key appears more than once in the signature file")
        seen.add(key_hex)
        if key_hex not in trusted:
            continue
        if key_hex in trusted_lines:
            raise ValueError("trusted key appears more than once in signature file")
        trusted_lines.add(key_hex)
        key_path = work / f"{key_hex}.pub"
        signature_path = work / f"{key_hex}.sig"
        input_path = work / "document"
        public_pem(bytes.fromhex(key_hex), key_path)
        signature_path.write_bytes(bytes.fromhex(match.group(2).decode("ascii")))
        input_path.write_bytes(data)
        result = subprocess.run(
            [
                "openssl", "pkeyutl", "-verify", "-rawin", "-pubin",
                "-inkey", str(key_path), "-in", str(input_path),
                "-sigfile", str(signature_path),
            ],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
        if result.returncode:
            raise ValueError(f"signature by trusted key {key_hex} does not verify")
    if not trusted_lines:
        raise ValueError("signature file has no signature by a trusted key")


def metadata_asset(name: str, data: bytes, output: Path) -> Path:
    path = output / ("updater--" + name.replace("/", "--"))
    path.write_bytes(data)
    return path


def _validate_relpath(value: str) -> None:
    if (
        not value
        or len(value) > 512
        or value.startswith("/")
        or "\\" in value
        or any(char in ':*?"<>|' for char in value)
        or value.startswith(" ")
        or value.endswith(" ")
        or any(ord(char) < 0x20 or ord(char) > 0x7e for char in value)
        or any(part in ("", ".", "..") for part in value.split("/"))
    ):
        raise ValueError(f"unsafe or unsupported updater path: {value!r}")


def _validate_link_target(value: str, link_path: str) -> None:
    if (
        not value
        or value.startswith("/")
        or "\\" in value
        or any(char in ':*?"<>|' for char in value)
        or value.startswith(" ")
        or value.endswith(" ")
        or any(ord(char) < 0x20 or ord(char) > 0x7e for char in value)
        or any(part in ("", ".") for part in value.split("/"))
    ):
        raise ValueError(f"unsafe symlink target for {link_path}: {value!r}")


def _extract_archive(archive_path: Path, destination: Path) -> None:
    with tarfile.open(archive_path, "r:gz") as archive:
        members = archive.getmembers()
        names = set()
        links = []
        for member in members:
            _validate_relpath(member.name)
            if member.name in names:
                raise ValueError(f"duplicate archive entry: {member.name}")
            names.add(member.name)
            if not (member.isfile() or member.isdir() or member.issym()):
                raise ValueError(f"unsupported archive entry type: {member.name}")
            output = destination.joinpath(*member.name.split("/"))
            if not output.resolve().is_relative_to(destination.resolve()):
                raise ValueError(f"archive entry escapes its destination: {member.name}")
            if member.issym():
                _validate_link_target(member.linkname, member.name)
                resolved_target = posixpath.normpath(
                    posixpath.join(posixpath.dirname(member.name), member.linkname)
                )
                if resolved_target == ".." or resolved_target.startswith("../"):
                    raise ValueError(f"symlink escapes the installation: {member.name}")
                links.append((output, member.linkname, member.mode))
            elif member.isdir():
                output.mkdir(parents=True, exist_ok=True)
            else:
                output.parent.mkdir(parents=True, exist_ok=True)
                source = archive.extractfile(member)
                if source is None:
                    raise ValueError(f"archive file has no data: {member.name}")
                with source, output.open("wb") as target:
                    shutil.copyfileobj(source, target)
                output.chmod(member.mode & 0o777)
        for output, target, _mode in links:
            output.parent.mkdir(parents=True, exist_ok=True)
            output.symlink_to(target)


def _validate_variant_metadata(metadata: object) -> dict:
    if not isinstance(metadata, dict):
        raise ValueError("variant metadata is not an object")
    variant_id = metadata.get("id")
    name = metadata.get("name")
    supersedes = metadata.get("supersedes")
    requirements = metadata.get("requires")
    match = VARIANT_ID.fullmatch(variant_id) if isinstance(variant_id, str) else None
    if not match or len(variant_id) > 64:
        raise ValueError(f"invalid variant ID: {variant_id!r}")
    if (
        not isinstance(name, str)
        or not 1 <= len(name) <= 64
        or name[0] == " "
        or name[-1] == " "
        or any(ord(char) < 0x20 or ord(char) > 0x7e for char in name)
    ):
        raise ValueError(f"invalid variant name: {name!r}")
    if not isinstance(supersedes, list):
        raise ValueError(f"invalid supersedes list for {variant_id}")
    for other in supersedes:
        if not isinstance(other, str) or not VARIANT_ID.fullmatch(other) or len(other) > 64 or other == variant_id:
            raise ValueError(f"invalid superseded variant in {variant_id}: {other!r}")
    if len(set(supersedes)) != len(supersedes):
        raise ValueError(f"duplicate superseded variant in {variant_id}")
    if not isinstance(requirements, dict):
        raise ValueError(f"invalid requirements for {variant_id}")
    os_name, architecture = match.groups()
    if requirements.get("os") != os_name or requirements.get("arch") != architecture:
        raise ValueError(f"variant ID and requirements disagree for {variant_id}")
    if not WORD.fullmatch(os_name) or not WORD.fullmatch(architecture):
        raise ValueError(f"invalid OS or architecture for {variant_id}")
    if os_name == "linux":
        expected = {"os", "arch", "glibc", "glibcxx", "qt"}
        if not all(isinstance(requirements.get(key), str) and VERSION.fullmatch(requirements[key])
                   for key in ("glibc", "glibcxx", "qt")):
            raise ValueError(f"invalid Linux version requirements for {variant_id}")
        if "abi" in requirements:
            expected.add("abi")
            abi = requirements["abi"]
            if (not isinstance(abi, dict) or abi.get("format") != 1
                    or abi.get("arch") != architecture or not isinstance(abi.get("interpreter"), str)
                    or not abi["interpreter"].startswith("/") or not isinstance(abi.get("libs"), list)
                    or not abi["libs"]
                    or not all(isinstance(lib, dict) and isinstance(lib.get("soname"), str)
                               and isinstance(lib.get("versions"), list)
                               and all(isinstance(version, str) for version in lib["versions"])
                               for lib in abi["libs"])):
                raise ValueError(f"invalid ELF ABI requirements for {variant_id}")
        if requirements["qt"].split(".", 1)[0] == "6" and (
            "abi" not in requirements
            or not any(lib["soname"] == "libQt6Core.so.6" for lib in requirements["abi"]["libs"])
        ):
            raise ValueError(f"Linux Qt 6 variant {variant_id} needs a measured Qt 6 ABI")
    elif os_name in ("windows", "macos"):
        expected = {"os", "arch", "os_version"}
        if not isinstance(requirements.get("os_version"), str) or not VERSION.fullmatch(requirements["os_version"]):
            raise ValueError(f"invalid OS version requirement for {variant_id}")
    else:
        raise ValueError(f"unsupported OS in variant {variant_id}")
    if set(requirements) != expected:
        raise ValueError(f"unexpected requirement fields for {variant_id}")
    return metadata


def _validate_variant_entries(files: list[dict], links: list[dict], variant: dict) -> None:
    paths = [entry["path"] for entry in files]
    link_paths = [entry["path"] for entry in links]
    if paths != sorted(paths) or link_paths != sorted(link_paths):
        raise ValueError(f"variant {variant['id']} paths are not sorted")
    entries = paths + link_paths
    if len(set(entries)) != len(entries):
        raise ValueError(f"variant {variant['id']} has duplicate paths")

    spelling = {}
    directories = set()
    for entry in entries:
        parts = entry.split("/")
        for index in range(1, len(parts) + 1):
            prefix = "/".join(parts[:index])
            if index < len(parts):
                directories.add(prefix)
            lower = prefix.lower()
            if lower in spelling and spelling[lower] != prefix:
                raise ValueError(f"paths differ only by case: {spelling[lower]} and {prefix}")
            spelling[lower] = prefix
    if directories.intersection(entries):
        raise ValueError(f"variant {variant['id']} lists a path that is also a directory")
    if links and variant["requires"]["os"] == "windows":
        raise ValueError(f"Windows variant {variant['id']} contains symbolic links")

    file_paths = set(paths)
    link_map = {entry["path"]: entry["target"] for entry in links}
    for link_path, target in link_map.items():
        current = link_path.split("/")[:-1]
        pending = target.split("/")
        depth = 0
        while pending:
            part = pending.pop(0)
            if part == "..":
                if not current:
                    raise ValueError(f"symlink escapes the installation: {link_path}")
                current.pop()
                continue
            current.append(part)
            resolved = "/".join(current)
            if resolved in link_map:
                depth += 1
                if depth > 40:
                    raise ValueError(f"symlink loop: {link_path}")
                current.pop()
                pending = link_map[resolved].split("/") + pending
        resolved = "/".join(current)
        if resolved not in file_paths and resolved not in directories:
            raise ValueError(f"symlink target is not a listed file or directory: {link_path}")


def _variant_files(root: Path, variant: dict, build_number: int, assets: Path) -> dict:
    files = []
    links = []
    for path in sorted(root.rglob("*"), key=lambda item: item.relative_to(root).as_posix()):
        relative = path.relative_to(root).as_posix()
        _validate_relpath(relative)
        if path.is_symlink():
            target = os.readlink(path).replace(os.sep, "/")
            _validate_link_target(target, relative)
            links.append({"path": relative, "target": target})
            continue
        if not path.is_file():
            continue

        size = path.stat().st_size
        if size == 0:
            if (
                path.name == "CodeSignature"
                and path.parent.name == "_CodeSignature"
                and path.parent.parent.name.endswith(".framework")
            ):
                continue
            raise ValueError(f"zero-byte updater file cannot be uploaded to a GitHub Release: {relative}")

        digest_builder = hashlib.sha256()
        with path.open("rb") as source:
            for block in iter(lambda: source.read(1024 * 1024), b""):
                digest_builder.update(block)
        digest = digest_builder.hexdigest()
        asset_name = f"file-{digest}"
        asset_path = assets / asset_name
        if asset_path.exists():
            existing_digest = hashlib.sha256()
            with asset_path.open("rb") as source:
                for block in iter(lambda: source.read(1024 * 1024), b""):
                    existing_digest.update(block)
            if asset_path.stat().st_size != size or existing_digest.hexdigest() != digest:
                raise ValueError(f"content-addressed asset collision: {asset_name}")
        else:
            shutil.copyfile(path, asset_path)
            copied_digest = hashlib.sha256()
            with asset_path.open("rb") as source:
                for block in iter(lambda: source.read(1024 * 1024), b""):
                    copied_digest.update(block)
            if copied_digest.hexdigest() != digest:
                raise ValueError(f"file changed while creating asset: {relative}")
        files.append(
            {
                "path": relative,
                "sha256": digest,
                "size": size,
                "executable": bool(path.stat().st_mode & (stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH)),
                "asset_url": f"{ASSET_BASE}build-{build_number}/{asset_name}",
            }
        )
    if not files:
        raise ValueError(f"variant {variant['id']} contains no regular files")
    _validate_variant_entries(files, links, variant)
    published = {
        "name": variant["name"],
        "supersedes": variant["supersedes"],
        "requires": dict(variant["requires"]),
        "files": files,
        "links": links,
    }
    # Older Qt 5 launchers reject unknown requirements. Readers that can
    # probe the other Qt major use this signed ABI for a switch back to Qt 5.
    requirements = published["requires"]
    if requirements.get("qt", "").split(".", 1)[0] == "5" and "abi" in requirements:
        published["linux_abi"] = requirements.pop("abi")
    return published


def make_build(payload_root: Path, build_number: int, commit: str, built: str, output: Path,
               version: str | None = None) -> tuple[bytes, Path]:
    assets = output / "content"
    assets.mkdir()
    variants = {}
    with tempfile.TemporaryDirectory() as temporary:
        extracted_root = Path(temporary)
        payload_dirs = sorted(path for path in payload_root.iterdir() if path.is_dir())
        if not payload_dirs:
            raise ValueError("no updater payload artifacts were downloaded")
        for payload_dir in payload_dirs:
            metadata = _validate_variant_metadata(
                json.loads((payload_dir / "variant.json").read_text(encoding="utf-8"))
            )
            variant_id = metadata["id"]
            if variant_id in variants:
                raise ValueError(f"duplicate variant artifact: {variant_id}")
            archive = payload_dir / "payload.tar.gz"
            target = extracted_root / variant_id
            target.mkdir()
            _extract_archive(archive, target)
            variants[variant_id] = _variant_files(target, metadata, build_number, assets)

    _variant_successors(variants)
    document = {
        "type": "build",
        "format": 1,
        "build": build_number,
        "commit": commit,
        "built": built,
        "variants": variants,
    }
    if version is not None:
        document["version"] = version
    data = (json.dumps(document, separators=(",", ":")) + "\n").encode("ascii")
    if len(data) > 8 * 1024 * 1024:
        raise ValueError("build manifest exceeds Updater V2's 8 MiB limit")
    return data, assets


def _gh_json(*args) -> object:
    return json.loads(command("gh", "api", *args, capture=True).stdout)


def _release_list(repository: str) -> list[dict]:
    releases = _gh_json(f"repos/{repository}/releases?per_page=100")
    if not isinstance(releases, list):
        raise ValueError("GitHub returned an unexpected releases response")
    return releases


def _create_draft_release(repository: str, tag: str, title: str, notes: str) -> dict:
    # The POST can succeed remotely even when gh receives an empty or truncated response.
    for delay in (1, 2, 4):
        try:
            release = _gh_json(
                "--method", "POST", f"repos/{repository}/releases",
                "-f", f"tag_name={tag}",
                "-F", "draft=true",
                "-f", f"name={title}",
                "-f", f"body={notes}",
                "-f", "target_commitish=main",
            )
            break
        except (subprocess.CalledProcessError, json.JSONDecodeError) as error:
            print(f"GitHub did not confirm draft creation for {tag}; checking after {delay}s.", flush=True)
            time.sleep(delay)
            release = next(
                (item for item in _release_list(repository) if item.get("tag_name") == tag),
                None,
            )
            if release is not None:
                if release.get("body") != notes:
                    raise ValueError(f"release {tag} already belongs to a different build") from error
                break
            if delay == 4:
                raise RuntimeError(f"GitHub did not create a draft release for {tag} after 3 attempts") from error
    if (
        not isinstance(release, dict)
        or release.get("tag_name") != tag
        or release.get("draft") is not True
        or type(release.get("id")) is not int
    ):
        raise ValueError(f"GitHub returned an invalid draft release for {tag}")
    return release


def _reuse_draft_release(repository: str, release_tag: str, release: dict | None) -> bool:
    if not release:
        return False
    if not release.get("draft"):
        raise ValueError(f"release {release_tag} already exists and is published")
    for asset in release.get("assets", []):
        command(
            "gh", "api", "--method", "DELETE",
            f"repos/{repository}/releases/assets/{asset['id']}",
        )
    return True


def _target_release(repository: str) -> dict | None:
    releases = _release_list(repository)
    published = [release for release in releases if not release.get("draft") and not release.get("prerelease")]
    builds = [
        (int(match.group(1)), release)
        for release in published
        if (match := re.fullmatch(r"build-([1-9][0-9]*)", release["tag_name"]))
    ]
    if builds:
        return max(builds, key=lambda item: item[0])[1]
    return published[0] if published else None


def _manifest_assets(release: dict) -> dict[int, list[dict]]:
    builds = {}
    for asset in release.get("assets", []):
        if match := BUILD_ASSET.fullmatch(asset["name"]):
            builds.setdefault(int(match.group(1)), []).append(asset)
    return builds


def _retained_builds(available: set[int], required: set[int]) -> set[int]:
    remaining = MAX_BUILD_MANIFESTS - len(required)
    return required | set(sorted(available - required, reverse=True)[:remaining])


def _download_manifest(repository: str, release: dict, build_number: int, output: Path,
                       trusted: list[str]) -> tuple[Path, Path]:
    name = f"updater--builds--{build_number}.json"
    work = output / f"build-{build_number}"
    work.mkdir()
    command(
        "gh", "release", "download", release["tag_name"],
        "--repo", repository,
        "--pattern", name,
        "--pattern", name + ".sig",
        "--dir", str(work),
    )
    document = work / name
    signature = work / (name + ".sig")
    if not document.is_file() or not signature.is_file():
        raise ValueError(f"{release['tag_name']} is missing the signed manifest for build {build_number}")
    verify_signature(document.read_bytes(), signature.read_bytes(), trusted, work)
    return document, signature


def _download_history(repository: str, sources: dict[int, dict], output: Path,
                      trusted: list[str]) -> list[Path]:
    with ThreadPoolExecutor(max_workers=HISTORY_WORKERS) as pool:
        jobs = [
            pool.submit(_download_manifest, repository, release, number, output, trusted)
            for number, release in sorted(sources.items())
        ]
        return [path for job in jobs for path in job.result()]


def _previous_signed_document(repository: str, release: dict | None, name: str,
                              output: Path, trusted: list[str]) -> tuple[Path, Path] | None:
    asset_name = "updater--" + name
    if not release or not any(asset["name"] == asset_name for asset in release.get("assets", [])):
        return None
    command(
        "gh", "release", "download", release["tag_name"],
        "--repo", repository,
        "--pattern", asset_name + "*",
        "--dir", str(output),
    )
    document = output / asset_name
    signature = Path(str(document) + ".sig")
    if not signature.is_file():
        raise ValueError(f"the latest Release has no signature for {name}")
    data = document.read_bytes()
    verify_signature(data, signature.read_bytes(), trusted, output)
    return document, signature


def _previous_tags(repository: str, release: dict | None, output: Path, trusted: list[str]) -> dict | None:
    signed = _previous_signed_document(repository, release, "tags.json", output, trusted)
    if not signed:
        return None
    data = signed[0].read_bytes()
    tags = json.loads(data)
    if not isinstance(tags, dict):
        raise ValueError("the latest Release contains an invalid tags document")
    values = tags.get("tags", {})
    if not isinstance(values, dict) or tags.get("type") != "tags" or not all(
        type(values.get(name)) is int and values[name] > 0
        for name in ("latest", "testing", "stable")
    ):
        raise ValueError("the latest Release contains an invalid tags document")
    if not values["stable"] <= values["testing"] <= values["latest"]:
        raise ValueError("the latest Release contains out-of-order tier tags")
    return values


def _next_tags(build_number: int, branch: str, previous: dict | None,
               advance: bool = True) -> dict[str, int]:
    if branch not in ("master", "testing", "develop"):
        raise ValueError(f"unsupported branch: {branch}")
    if previous and not advance:
        return {key: previous[key] for key in ("latest", "testing", "stable")}
    stable = previous["stable"] if previous else build_number
    testing = previous["testing"] if previous else build_number
    if branch == "master":
        stable = testing = build_number
    elif branch == "testing":
        testing = build_number
    if not stable <= testing <= build_number:
        raise ValueError("the new build would put update tiers out of order")
    return {"latest": build_number, "testing": testing, "stable": stable}


def _variant_successors(variants: dict) -> dict[str, str]:
    successors = {}
    for variant_id, variant in variants.items():
        for predecessor in variant["supersedes"]:
            if predecessor in successors:
                raise ValueError(f"multiple successors for updater variant {predecessor}")
            successors[predecessor] = variant_id

    for predecessor in successors:
        visited = set()
        while predecessor in successors:
            if predecessor in visited:
                raise ValueError(f"cycle in updater variant successors at {predecessor}")
            visited.add(predecessor)
            predecessor = successors[predecessor]
    return successors


def _requirements_cover(previous: dict, candidate: dict) -> bool:
    for name, value in candidate.items():
        if name not in previous:
            return False
        if name == "os":
            if value != previous[name]:
                return False
            continue
        if name == "arch":
            # Match the compatibility routes used by the launcher. The macOS route requires Rosetta.
            route = (previous["os"], previous["arch"], value)
            if value != previous["arch"] and route not in (
                ("windows", "x86_64", "x86"),
                ("macos", "aarch64", "x86_64"),
                ("macos", "arm64", "x86_64"),
                ("macos", "arm64", "universal"),
                ("macos", "x86_64", "universal"),
            ):
                return False
            continue
        if name == "abi":
            old_abi = previous[name]
            if any(value[key] != old_abi[key] for key in ("format", "arch", "interpreter")):
                return False
            old_libs = {lib["soname"]: set(lib["versions"]) for lib in old_abi["libs"]}
            if any(lib["soname"] not in old_libs
                   or not set(lib["versions"]) <= old_libs[lib["soname"]]
                   for lib in value["libs"]):
                return False
            continue
        if name == "qt" and value.split(".", 1)[0] != previous[name].split(".", 1)[0]:
            return False
        old_parts = map(int, previous[name].split("."))
        new_parts = map(int, value.split("."))
        for old, new in zip_longest(old_parts, new_parts, fillvalue=0):
            if old != new:
                if new > old:
                    return False
                break
    return True


def _check_variant_continuity(previous: dict, candidate: dict) -> None:
    variants = candidate["variants"]
    successors = _variant_successors(variants)
    stranded = []
    for variant_id, variant in previous["variants"].items():
        chain = [variant_id]
        while chain[-1] in successors:
            chain.append(successors[chain[-1]])
        if not any(
            name in variants and _requirements_cover(variant["requires"], variants[name]["requires"])
            for name in chain
        ):
            stranded.append(variant_id)
    if stranded:
        raise ValueError(
            f"updater continuity from build {previous['build']} to {candidate['build']}: "
            f"missing compatible variants for {', '.join(sorted(stranded))}"
        )


def _initialise_empty_repository(repository: str) -> None:
    with tempfile.TemporaryDirectory() as temporary:
        root = Path(temporary)
        command("git", "init", "--initial-branch=main", cwd=root)
        command("gh", "auth", "setup-git")
        command("git", "remote", "add", "origin", f"https://github.com/{repository}.git", cwd=root)
        if command(
            "git", "ls-remote", "--heads", "origin", "refs/heads/main",
            capture=True, cwd=root,
        ).stdout.strip():
            return
        command("git", "config", "user.name", "github-actions[bot]", cwd=root)
        command("git", "config", "user.email", "41898282+github-actions[bot]@users.noreply.github.com", cwd=root)
        (root / "README.md").write_text(
            "MeshMC updater release assets are published here by the MeshMC build workflow.\n",
            encoding="utf-8",
        )
        command("git", "add", "README.md", cwd=root)
        command("git", "commit", "-m", "Initialize updater releases repository", cwd=root)
        try:
            command("git", "push", "-u", "origin", "main", cwd=root)
        except subprocess.CalledProcessError:
            if not command(
                "git", "ls-remote", "--heads", "origin", "refs/heads/main",
                capture=True, cwd=root,
            ).stdout.strip():
                raise


def _check_asset_limit(tag: str, count: int) -> None:
    if count > MAX_RELEASE_ASSETS:
        raise ValueError(f"{tag} needs {count} assets, exceeding GitHub's limit of {MAX_RELEASE_ASSETS}")


def _sync_historical_builds(repository: str, latest: dict, online_key: str, work: Path) -> int:
    latest_tag = latest["tag_name"]
    latest_number = int(latest_tag.removeprefix("build-"))
    work.mkdir()
    trusted = [online_key]
    tags = _previous_tags(repository, latest, work, trusted)
    if not tags:
        raise ValueError(f"{latest_tag} is missing signed tags metadata")

    current = _manifest_assets(latest)
    sources = {}
    for release in _release_list(repository):
        if release.get("draft") or release.get("prerelease"):
            continue
        match = re.fullmatch(r"build-([1-9][0-9]*)", release["tag_name"])
        if match and int(match.group(1)) < latest_number:
            sources[int(match.group(1))] = release

    retained = _retained_builds(current.keys() | sources.keys(), set(tags.values()) | {latest_number})
    missing = {
        number: sources[number]
        for number in retained
        if len(current.get(number, [])) != 2 and number in sources
    }
    if any(len(current.get(number, [])) != 2 and number not in missing for number in retained):
        raise ValueError(f"{latest_tag} is missing a retained signed build manifest")
    obsolete = [asset for number in current.keys() - retained for asset in current[number]]
    downloaded = _download_history(repository, missing, work, trusted)
    current_names = {asset["name"] for asset in latest.get("assets", [])}
    upload = [path for path in downloaded if path.name not in current_names]
    _check_asset_limit(latest_tag, len(current_names) - len(obsolete) + len(upload))

    for asset in obsolete:
        command("gh", "api", "--method", "DELETE", f"repos/{repository}/releases/assets/{asset['id']}")
    try:
        _upload_assets(repository, latest_tag, upload)
    except subprocess.CalledProcessError:
        refreshed = _gh_json(f"repos/{repository}/releases/tags/{latest_tag}")
        refreshed_names = {asset["name"] for asset in refreshed.get("assets", [])}
        if any(path.name not in refreshed_names for path in upload):
            raise
    return tags["stable"]


def _upload_assets(repository: str, tag: str, assets: list[Path]) -> None:
    # The GitHub CLI uploads assets within each batch concurrently.
    for offset in range(0, len(assets), 50):
        command(
            "gh", "release", "upload", tag,
            *[str(path) for path in assets[offset:offset + 50]],
            "--repo", repository,
        )


def _publish_draft(repository: str, release: dict, advance: bool) -> None:
    command(
        "gh", "api", "--method", "PATCH",
        f"repos/{repository}/releases/{release['id']}",
        "-F", "draft=false",
        "-F", "prerelease=false",
        "-f", f"make_latest={'true' if advance else 'false'}",
    )


def _finish_release(repository: str, release_tag: str, online_key: str, work: Path) -> int:
    latest = _target_release(repository)
    if not latest:
        raise ValueError(f"could not locate a published release after publishing {release_tag}")
    stable_build = _sync_historical_builds(repository, latest, online_key, work / "sync")
    if latest["tag_name"] != release_tag:
        command(
            "gh", "api", "--method", "PATCH",
            f"repos/{repository}/releases/{latest['id']}",
            "-f", "make_latest=true",
        )
    return stable_build


def _resume_published_release(repository: str, release: dict, build_data: bytes,
                              online_key: str, work: Path) -> int:
    candidate = json.loads(build_data)
    document, _signature = _download_manifest(repository, release, candidate["build"], work, [online_key])
    published = json.loads(document.read_bytes())
    # Build timestamps can change when the same payload is retried.
    candidate.pop("built", None)
    published.pop("built", None)
    if published != candidate or release.get("prerelease"):
        raise ValueError(f"published release {release['tag_name']} does not match this build")
    return _finish_release(repository, release["tag_name"], online_key, work)


def _sign_file(private_key: Path, key_id: str, data: bytes, output: Path) -> Path:
    raw_signature = output.parent / (output.name + ".raw")
    signature = sign(private_key, data, raw_signature)
    raw_signature.unlink()
    output.write_bytes(f"{key_id} {signature.hex()}\n".encode("ascii"))
    return output


def _add_metadata_assets(directory: Path, floor_data: bytes, floor_signature: bytes,
                         build_data: bytes, build_number: int, branch: str, online_key: str,
                         private_key: Path, history_dir: Path, previous: dict | None,
                         advance: bool) -> list[Path]:
    assets = []
    repository = "Project-Tick/MeshMC-Updates"
    trusted = [online_key]
    old_tags = _previous_tags(repository, previous, history_dir, trusted)
    if previous and re.fullmatch(r"build-[1-9][0-9]*", previous["tag_name"]) and not old_tags:
        raise ValueError("the previous updater Release is missing signed tags metadata")
    tags_for_release = _next_tags(build_number, branch, old_tags, advance)
    if previous:
        available = _manifest_assets(previous)
        retained = _retained_builds(set(available), set(tags_for_release.values()) | {build_number})
        replaced = {
            number for tier, number in (old_tags or {}).items()
            if number != tags_for_release[tier] and tags_for_release[tier] == build_number
        }
        historical = _download_history(
            repository, {number: previous for number in (retained | replaced) - {build_number}}, history_dir, trusted
        )
        if replaced:
            candidate = json.loads(build_data)
            for number in sorted(replaced):
                document = history_dir / f"build-{number}" / f"updater--builds--{number}.json"
                old_build = json.loads(document.read_bytes())
                if old_build.get("build") != number:
                    raise ValueError(f"previous updater manifest has the wrong build number: {document.name}")
                _check_variant_continuity(old_build, candidate)
        for source in historical:
            if int(BUILD_ASSET.fullmatch(source.name).group(1)) not in retained:
                continue
            target = directory / source.name
            shutil.copyfile(source, target)
            assets.append(target)

    assets.append(metadata_asset("keys/1.json", floor_data, directory))
    assets.append(metadata_asset("keys/1.json.sig", floor_signature, directory))

    now = timestamp()
    if previous and not advance:
        mirrors = _previous_signed_document(
            "Project-Tick/MeshMC-Updates", previous, "mirrors.json", history_dir, [online_key]
        )
        if not old_tags or not mirrors:
            raise ValueError("the latest Release is missing signed tags or mirrors metadata")
        tag_document = history_dir / "updater--tags.json"
        tag_signature = Path(str(tag_document) + ".sig")
        for document, signature in ((tag_document, tag_signature), mirrors):
            copied_document = directory / document.name
            copied_signature = directory / signature.name
            shutil.copyfile(document, copied_document)
            shutil.copyfile(signature, copied_signature)
            assets.extend((copied_document, copied_signature))
    else:
        mirrors = {
            "type": "mirrors",
            "format": 1,
            "issued": now,
            "mirrors": [[LATEST_BASE]],
        }
        tags = {
            "type": "tags",
            "format": 1,
            "serial": build_number,
            "issued": now,
            "tags": tags_for_release,
            "withdrawn": [],
        }
        for name, document in (("mirrors.json", mirrors), ("tags.json", tags)):
            data = (json.dumps(document, separators=(",", ":")) + "\n").encode("ascii")
            doc_path = metadata_asset(name, data, directory)
            sig_path = _sign_file(private_key, online_key, data, Path(str(doc_path) + ".sig"))
            assets.extend((doc_path, sig_path))

    build_path = metadata_asset(f"builds/{build_number}.json", build_data, directory)
    build_sig = _sign_file(private_key, online_key, build_data, Path(str(build_path) + ".sig"))
    assets.extend((build_path, build_sig))
    return assets


def publish(args) -> int:
    if not re.fullmatch(r"[0-9a-f]{40}", args.commit):
        raise ValueError("commit must be a full lowercase Git SHA")
    if not args.build_number.isdecimal() or int(args.build_number) < 1:
        raise ValueError("build number must be a positive integer")
    build_number = int(args.build_number)
    if not re.fullmatch(r"[0-9]+\.[0-9]+\.[0-9]+", args.version):
        raise ValueError("version must be major.minor.series")
    if not re.fullmatch(r"[1-9][0-9]*", args.workflow_run):
        raise ValueError("workflow run must be a positive integer")
    release_notes = (
        f"Source commit: {args.commit}\nBranch: {args.branch}\n"
        f"Version: {args.version}\nWorkflow run: {args.workflow_run}"
    )
    private_key = Path(args.signing_key).resolve(strict=True)
    floor_path = Path(args.floor_keys).resolve(strict=True)
    floor_signature_path = Path(args.floor_signature).resolve(strict=True)
    floor_data = floor_path.read_bytes()
    floor_signature = floor_signature_path.read_bytes()
    floor = json.loads(floor_data)
    if floor.get("type") != "keys" or floor.get("format") != 1 or floor.get("version") != 1:
        raise ValueError("keys/1.json has an unsupported format")
    root_keys = floor.get("root_keys")
    online_keys = floor.get("online_keys")
    if not isinstance(root_keys, list) or not isinstance(online_keys, list):
        raise ValueError("keys/1.json must contain root_keys and online_keys")
    if (
        not 1 <= len(root_keys) <= 4
        or not 1 <= len(online_keys) <= 4
        or any(not isinstance(key, str) or not re.fullmatch(r"[0-9a-f]{64}", key) for key in root_keys + online_keys)
        or len(set(root_keys)) != len(root_keys)
        or len(set(online_keys)) != len(online_keys)
        or set(root_keys).intersection(online_keys)
    ):
        raise ValueError("keys/1.json contains an invalid public key")
    root_keys_path = floor_path.parent / "root-keys.txt"
    if not root_keys_path.is_file():
        raise ValueError(f"missing CMake root-key list: {root_keys_path}")
    compiled_root_keys = [line.strip() for line in root_keys_path.read_text(encoding="ascii").splitlines() if line.strip()]
    if compiled_root_keys != root_keys:
        raise ValueError("root-keys.txt does not match the ordered root_keys in keys/1.json")
    expires = floor.get("expires")
    if not isinstance(expires, str):
        raise ValueError("keys/1.json is missing expires")
    try:
        expiry = dt.datetime.fromisoformat(expires.replace("Z", "+00:00"))
    except ValueError as error:
        raise ValueError("keys/1.json has an invalid expiry") from error
    if expiry.tzinfo is None or expiry <= dt.datetime.now(dt.timezone.utc):
        raise ValueError("keys/1.json has expired or has no UTC offset")
    online_key = public_key(private_key).hex()
    if online_key not in online_keys:
        raise ValueError("UPDATER_ONLINE_SIGNING_KEY is not listed in keys/1.json")

    repository = "Project-Tick/MeshMC-Updates"
    with tempfile.TemporaryDirectory() as temporary:
        work = Path(temporary)
        verify_signature(floor_data, floor_signature, root_keys, work)
        build_data, content_assets = make_build(
            Path(args.payload_root).resolve(strict=True),
            build_number,
            args.commit,
            args.built,
            work,
            args.version,
        )
        metadata_dir = work / "metadata"
        metadata_dir.mkdir()
        history_dir = work / "history"
        history_dir.mkdir()
        existing = _release_list(repository)
        release_tag = f"build-{build_number}"
        existing_build = next(
            (release for release in existing if release.get("tag_name") == release_tag),
            None,
        )
        if not existing_build:
            # A rerun can refer to a published build outside the first release page.
            older_release = _gh_json(
                "--paginate", "--slurp", f"repos/{repository}/releases?per_page=100"
            ) if len(existing) == 100 else []
            existing_build = next(
                (release for page in older_release for release in page
                 if release.get("tag_name") == release_tag),
                None,
            )
        if existing_build and not existing_build.get("draft"):
            if existing_build.get("body") != release_notes:
                raise ValueError(f"release {release_tag} already belongs to a different workflow run")
            stable_build = _resume_published_release(repository, existing_build, build_data, online_key, work)
            print(f"Completed history synchronization for {release_tag}.")
            return stable_build

        previous = _target_release(repository)
        previous_builds = []
        if previous:
            previous_builds = [
                int(match.group(1))
                for asset in previous.get("assets", [])
                if (match := BUILD_ASSET.fullmatch(asset["name"]))
            ]
        advance = not previous_builds or build_number > max(previous_builds)
        metadata_assets = _add_metadata_assets(
            metadata_dir,
            floor_data,
            floor_signature,
            build_data,
            build_number,
            args.branch,
            online_key,
            private_key,
            history_dir,
            previous,
            advance,
        )
        all_assets = sorted(content_assets.iterdir()) + sorted(metadata_assets)
        _check_asset_limit(release_tag, len(all_assets))
        reuse_draft = _reuse_draft_release(repository, release_tag, existing_build)
        _initialise_empty_repository(repository)
        release = existing_build if reuse_draft else _create_draft_release(
            repository,
            release_tag,
            f"MeshMC updater build {build_number}",
            release_notes,
        )
        if release is None:
            raise ValueError(f"GitHub did not return the draft release for {release_tag}")

        if reuse_draft and release.get("body") != release_notes:
            # The previous attempt never published this build ID.
            _gh_json(
                "--method", "PATCH", f"repos/{repository}/releases/{release['id']}",
                "-f", f"body={release_notes}",
            )
        _upload_assets(repository, release_tag, all_assets)
        _publish_draft(repository, release, advance)
        stable_build = _finish_release(repository, release_tag, online_key, work)
        print(f"Published {release_tag} with {len(all_assets)} assets.")
        return stable_build


def main() -> None:
    parser = argparse.ArgumentParser(description="Publish a signed Updater V2 build release.")
    parser.add_argument("--payload-root", required=True)
    parser.add_argument("--build-number", required=True)
    parser.add_argument("--version", required=True)
    parser.add_argument("--workflow-run", required=True)
    parser.add_argument("--commit", required=True)
    parser.add_argument("--branch", required=True)
    parser.add_argument("--built", default=timestamp())
    parser.add_argument("--floor-keys", default=KEYS_FILE)
    parser.add_argument("--floor-signature", default=KEYS_SIGNATURE_FILE)
    parser.add_argument("--signing-key", required=True)
    parser.add_argument("--github-output", type=Path)
    args = parser.parse_args()
    stable_build = publish(args)
    if args.github_output:
        with args.github_output.open("a", encoding="utf-8") as output:
            output.write(f"stable-build={stable_build}\n")


if __name__ == "__main__":
    main()
