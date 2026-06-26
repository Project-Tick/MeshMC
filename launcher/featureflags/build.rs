// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: GPL-3.0-or-later WITH LicenseRef-MeshMC-MMCO-Module-Exception-1.0

//! Build script for the MeshMC feature-flag engine.
//!
//! We generate the C++ side of the cxx bridge ourselves here, instead of using
//! Corrosion's `corrosion_add_cxxbridge()` CMake helper. Two reasons:
//!   1. Fewer moving parts on the CMake/Corrosion side.
//!   2. Corrosion's helper hard-codes a `<crate>/src/` layout for the bridge
//!      file; doing it in build.rs lets the Rust sources live flat in
//!      launcher/featureflags/, right next to FeatureFlags.{h,cpp}.
//!
//! cxx-build compiles the generated glue into the Rust staticlib. We also copy
//! the generated headers (`lib.rs.h` and `cxx.h`) into a stable, predictable
//! location — `<crate>/generated/meshmc_featureflags/` — so the C++ side can
//! `#include "meshmc_featureflags/lib.rs.h"` without having to discover cargo's
//! hashed OUT_DIR. CMake adds `<crate>/generated` to MeshMC_logic's include
//! path.

use std::path::PathBuf;
use std::{env, fs};

fn main() {
    // Compile the bridge glue into the staticlib.
    cxx_build::bridge("lib.rs").compile("meshmc_featureflags_cxx");

    // cxx-build (via the cxx crate) emits the generated headers under
    // $OUT_DIR/cxxbridge/. Locate them and copy into a stable include tree so
    // the C++ consumer has a deterministic include path.
    let out_dir = PathBuf::from(env::var("OUT_DIR").expect("OUT_DIR set by cargo"));
    let manifest_dir =
        PathBuf::from(env::var("CARGO_MANIFEST_DIR").expect("CARGO_MANIFEST_DIR set by cargo"));

    // Destination: <crate>/generated/meshmc_featureflags/{lib.rs.h, ...}
    // and        <crate>/generated/rust/cxx.h
    let gen_root = manifest_dir.join("generated");
    let bridge_inc = gen_root.join("meshmc_featureflags");
    let rust_inc = gen_root.join("rust");
    fs::create_dir_all(&bridge_inc).expect("create generated/meshmc_featureflags");
    fs::create_dir_all(&rust_inc).expect("create generated/rust");

    // The bridge header for lib.rs.
    let src_lib_h = out_dir
        .join("cxxbridge")
        .join("include")
        .join("meshmc-featureflags")
        .join("lib.rs.h");
    // cxx-build names the include subdir after the crate; fall back to scanning
    // if the exact path is not present (crate name normalisation differences).
    let lib_h = if src_lib_h.exists() {
        src_lib_h
    } else {
        find_file(&out_dir.join("cxxbridge").join("include"), "lib.rs.h")
            .expect("generated lib.rs.h not found under OUT_DIR/cxxbridge/include")
    };
    fs::copy(&lib_h, bridge_inc.join("lib.rs.h")).expect("copy lib.rs.h");

    // The shared cxx.h runtime header.
    let cxx_h = find_file(&out_dir.join("cxxbridge"), "cxx.h")
        .expect("generated rust/cxx.h not found under OUT_DIR/cxxbridge");
    fs::copy(&cxx_h, rust_inc.join("cxx.h")).expect("copy cxx.h");

    println!("cargo:rerun-if-changed=lib.rs");
    println!("cargo:rerun-if-changed=engine.rs");
    println!("cargo:rerun-if-changed=model.rs");
    println!("cargo:rerun-if-changed=murmur.rs");
}

/// Recursively find the first file named `name` under `dir`.
fn find_file(dir: &std::path::Path, name: &str) -> Option<PathBuf> {
    let entries = fs::read_dir(dir).ok()?;
    for entry in entries.flatten() {
        let path = entry.path();
        if path.is_dir() {
            if let Some(found) = find_file(&path, name) {
                return Some(found);
            }
        } else if path.file_name().map(|n| n == name).unwrap_or(false) {
            return Some(path);
        }
    }
    None
}
