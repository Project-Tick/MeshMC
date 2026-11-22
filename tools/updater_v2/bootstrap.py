#!/usr/bin/env python3

import argparse
import datetime as dt
import json
import os
from pathlib import Path
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[2]
DEFAULT_PUBLIC_DIR = ROOT / "launcher" / "updater" / "v2" / "keys"


def run(*args, **kwargs):
    return subprocess.run(args, check=True, **kwargs)


def public_key(private_key: Path) -> bytes:
    result = run(
        "openssl", "pkey", "-in", str(private_key), "-pubout", "-outform", "DER",
        stdout=subprocess.PIPE,
    )
    der = result.stdout
    if len(der) != 44 or der[:12] != bytes.fromhex("302a300506032b6570032100"):
        raise RuntimeError(f"{private_key} is not an Ed25519 key")
    return der[-32:]


def sign(private_key: Path, document: bytes) -> bytes:
    with tempfile.NamedTemporaryFile() as source, tempfile.NamedTemporaryFile() as signature:
        source.write(document)
        source.flush()
        run(
            "openssl", "pkeyutl", "-sign", "-rawin", "-inkey", str(private_key),
            "-in", source.name, "-out", signature.name,
        )
        signature.seek(0)
        return signature.read()


def create_key(path: Path) -> bytes:
    run("openssl", "genpkey", "-algorithm", "Ed25519", "-out", str(path))
    path.chmod(0o600)
    return public_key(path)


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Create an offline Updater V2 root key and an online signing key."
    )
    parser.add_argument(
        "--private-dir",
        type=Path,
        required=True,
        help="private output directory outside the repository; keep root-private.pem offline",
    )
    parser.add_argument(
        "--public-dir",
        type=Path,
        default=DEFAULT_PUBLIC_DIR,
        help="directory for keys/1.json and its public signature",
    )
    args = parser.parse_args()

    private_dir = args.private_dir.expanduser().resolve()
    public_dir = args.public_dir.expanduser().resolve()
    try:
        private_dir.relative_to(ROOT)
    except ValueError:
        pass
    else:
        parser.error("--private-dir must be outside the repository")

    if private_dir == public_dir or public_dir in private_dir.parents or private_dir in public_dir.parents:
        parser.error("private and public output directories must not overlap")
    if private_dir.exists() and any(private_dir.iterdir()):
        parser.error(f"private output directory is not empty: {private_dir}")
    if (public_dir / "1.json").exists() or (public_dir / "1.json.sig").exists():
        parser.error(f"public key document already exists in {public_dir}")

    private_dir.mkdir(parents=True, exist_ok=True, mode=0o700)
    private_dir.chmod(0o700)
    public_dir.mkdir(parents=True, exist_ok=True)

    root_private = private_dir / "root-private.pem"
    online_private = private_dir / "online-private.pem"
    root_public = create_key(root_private)
    online_public = create_key(online_private)

    expires = (dt.datetime.now(dt.timezone.utc) + dt.timedelta(days=365 * 5))
    document = {
        "type": "keys",
        "format": 1,
        "version": 1,
        "expires": expires.replace(microsecond=0).isoformat().replace("+00:00", "Z"),
        "root_keys": [root_public.hex()],
        "online_keys": [online_public.hex()],
    }
    data = (json.dumps(document, separators=(",", ":")) + "\n").encode("ascii")
    signature = sign(root_private, data)
    signature_file = f"{root_public.hex()} {signature.hex()}\n".encode("ascii")

    (public_dir / "1.json").write_bytes(data)
    (public_dir / "1.json.sig").write_bytes(signature_file)
    (public_dir / "root-keys.txt").write_text(root_public.hex() + "\n", encoding="ascii")
    os.chmod(public_dir / "1.json", 0o644)
    os.chmod(public_dir / "1.json.sig", 0o644)
    os.chmod(public_dir / "root-keys.txt", 0o644)

    print(f"Root public key:   {root_public.hex()}")
    print(f"Online public key: {online_public.hex()}")
    print(f"Commit these public files: {public_dir / '1.json'}, {public_dir / '1.json.sig'}, and {public_dir / 'root-keys.txt'}")
    print(f"Store {online_private} as the UPDATER_ONLINE_SIGNING_KEY GitHub secret.")
    print(f"Keep {root_private} offline; do not add it to GitHub or the workflow.")


if __name__ == "__main__":
    main()
