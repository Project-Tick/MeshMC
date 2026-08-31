#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Project Tick
# SPDX-License-Identifier: Apache-2.0
"""
mmco_sign.py — append a GPG signature trailer to a built .mmco module.

Layout of the trailer (matches MMCOFormat.h):

    <original module bytes>
    <ASCII-armored detached signature, N bytes>
    uint64_t signature_size  (little-endian)
    uint32_t trailer_magic   (little-endian, = 0x53434D4D — "MMCS")

The script is idempotent: if a trailer already exists, it is stripped first
and replaced with the new one. The signing key is looked up by ``--key``
(passed straight to ``gpg --local-user``). Output may either overwrite the
input file or be written to ``--output``.

Typical use from CMake:

    add_custom_command(
        TARGET MyPlugin POST_BUILD
        COMMAND ${Python3_EXECUTABLE}
                ${CMAKE_SOURCE_DIR}/scripts/mmco_sign.py
                --key 0xDEADBEEF
                $<TARGET_FILE:MyPlugin>
        VERBATIM
    )
"""

from __future__ import annotations

import argparse
import struct
import subprocess
import sys
import tempfile
from pathlib import Path


MMCO_TRAILER_MAGIC = 0x53434D4D  # ASCII "MMCS"
TRAILER_FOOTER_FMT = "<QI"  # uint64 size, uint32 magic — 12 bytes
TRAILER_FOOTER_SIZE = struct.calcsize(TRAILER_FOOTER_FMT)


def strip_existing_trailer(data: bytes) -> bytes:
    """If `data` ends with our trailer, return the bytes before it."""
    if len(data) < TRAILER_FOOTER_SIZE:
        return data
    sig_size, magic = struct.unpack(
        TRAILER_FOOTER_FMT, data[-TRAILER_FOOTER_SIZE:]
    )
    if magic != MMCO_TRAILER_MAGIC:
        return data
    cut = len(data) - TRAILER_FOOTER_SIZE - sig_size
    if cut < 0:
        # Malformed — fall through and return the original blob; the
        # launcher will flag it as malformed at load time.
        return data
    return data[:cut]


def gpg_sign(payload: bytes, key: str, gpg_bin: str, homedir: str | None) -> bytes:
    """Produce an ASCII-armored detached signature over `payload`."""
    cmd = [gpg_bin, "--batch", "--yes", "--armor", "--detach-sign"]
    if homedir:
        cmd += ["--homedir", homedir]
    if key:
        cmd += ["--local-user", key]
    with tempfile.NamedTemporaryFile() as in_f:
        in_f.write(payload)
        in_f.flush()
        proc = subprocess.run(
            cmd + ["--output", "-", in_f.name],
            check=False,
            capture_output=True,
        )
    if proc.returncode != 0:
        sys.stderr.write(proc.stderr.decode("utf-8", errors="replace"))
        raise SystemExit(
            f"gpg --detach-sign failed (exit {proc.returncode})"
        )
    return proc.stdout


def main() -> int:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("module", type=Path, help="Path to the .mmco file")
    p.add_argument("--key", required=True, help="GPG signing key (user id or fpr)")
    p.add_argument("--output", type=Path, default=None,
                   help="Where to write the signed file (default: overwrite)")
    p.add_argument("--gpg", default="gpg", help="gpg executable to use")
    p.add_argument("--homedir", default=None, help="GnuPG home directory")
    args = p.parse_args()

    if not args.module.is_file():
        sys.stderr.write(f"error: {args.module} is not a regular file\n")
        return 2

    original = args.module.read_bytes()
    payload = strip_existing_trailer(original)
    signature = gpg_sign(payload, args.key, args.gpg, args.homedir)

    footer = struct.pack(TRAILER_FOOTER_FMT, len(signature), MMCO_TRAILER_MAGIC)
    out_bytes = payload + signature + footer

    target = args.output if args.output is not None else args.module
    target.write_bytes(out_bytes)
    print(
        f"Signed {args.module.name}: payload {len(payload)} B, "
        f"signature {len(signature)} B (key {args.key})"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
