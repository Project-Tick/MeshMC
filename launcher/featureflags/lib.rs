/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-FileContributor: Project Tick
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (C) 2026 Project Tick
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//! MeshMC feature-flag engine.
//!
//! This is the first MeshMC subsystem written in Rust and called from C++.
//! It implements the *decision* half of an Unleash client: parsing the
//! `/client/features` document and evaluating activation strategies against a
//! context. The *transport* half (the HTTPS GET, with the UNLEASH-INSTANCEID /
//! UNLEASH-APPNAME headers) is done by the Qt/C++ side and the raw response
//! body is handed to us here. That keeps this crate network-free and fully
//! deterministic.
//!
//! Offline resilience: C++ owns one `Evaluator`. It feeds in the freshest body
//! it managed to fetch (or the last one it persisted to disk). If nothing has
//! ever been loaded, every query returns the caller-supplied compiled-in
//! default, so a launcher with no network still launches.
//!
//! The Rust sources live directly beside the C++ that consumes them
//! (FeatureFlags.{h,cpp}); there is no separate src/ directory.

mod engine;
mod model;
mod murmur;

use model::{Context, FeatureDocument};

/// Holds the last successfully parsed feature document. Cheap to keep around;
/// C++ updates it whenever a fresh body arrives and queries it on every
/// `is_enabled` call.
pub struct Evaluator {
    document: Option<FeatureDocument>,
}

impl Evaluator {
    fn new() -> Self {
        Evaluator { document: None }
    }

    fn load(&mut self, body: &[u8]) -> bool {
        match FeatureDocument::parse(body) {
            Ok(doc) => {
                self.document = Some(doc);
                true
            }
            // Keep whatever we had before; a malformed/empty response must not
            // wipe a previously good document.
            Err(_) => false,
        }
    }

    fn has_data(&self) -> bool {
        self.document.is_some()
    }

    fn is_enabled(&self, flag: &str, user_id: &str, app_name: &str, default_value: bool) -> bool {
        match &self.document {
            Some(doc) => {
                let ctx = Context {
                    user_id: user_id.to_string(),
                    app_name: app_name.to_string(),
                };
                engine::is_enabled(doc, flag, &ctx, default_value)
            }
            // No document loaded yet (first run / offline): use the default.
            None => default_value,
        }
    }

    fn list_flags(&self, user_id: &str, app_name: &str) -> Vec<ffi::FlagInfo> {
        match &self.document {
            Some(doc) => {
                let ctx = Context {
                    user_id: user_id.to_string(),
                    app_name: app_name.to_string(),
                };
                engine::evaluate_all(doc, &ctx)
                    .into_iter()
                    .map(|f| ffi::FlagInfo {
                        name: f.name,
                        toggle_enabled: f.toggle_enabled,
                        effective: f.effective,
                        strategies: f.strategies,
                    })
                    .collect()
            }
            None => Vec::new(),
        }
    }
}

#[cxx::bridge(namespace = "meshmc::ff")]
mod ffi {
    /// A flag and its evaluated state, returned to C++ for display in the
    /// Feature Flags dialog.
    struct FlagInfo {
        name: String,
        /// The toggle's raw `enabled` field.
        toggle_enabled: bool,
        /// Final decision after evaluating strategies for the current context.
        effective: bool,
        /// Comma-separated strategy names (or "(none)").
        strategies: String,
    }

    extern "Rust" {
        type Evaluator;

        /// Create an empty evaluator. Owned by C++ as a Box<Evaluator>.
        fn new_evaluator() -> Box<Evaluator>;

        /// Replace the cached feature document from a freshly fetched (or
        /// persisted) Unleash `/client/features` body. Returns true if the
        /// body parsed cleanly; on false the previous document is kept.
        fn load(self: &mut Evaluator, body: &[u8]) -> bool;

        /// Whether a valid feature document is currently loaded.
        fn has_data(self: &Evaluator) -> bool;

        /// Evaluate a flag. `user_id` is the sticky id for rollout strategies
        /// (may be empty), `app_name` is the environment. `default_value` is
        /// returned when the flag is unknown or no document is loaded.
        fn is_enabled(
            self: &Evaluator,
            flag: &str,
            user_id: &str,
            app_name: &str,
            default_value: bool,
        ) -> bool;

        /// List every flag in the loaded document with its evaluated state for
        /// the given context. Empty when no document is loaded.
        fn list_flags(self: &Evaluator, user_id: &str, app_name: &str) -> Vec<FlagInfo>;
    }
}

fn new_evaluator() -> Box<Evaluator> {
    Box::new(Evaluator::new())
}

#[cfg(test)]
mod tests {
    use super::*;

    const SAMPLE: &[u8] = br#"{"version":1,"features":[
        {"name":"meshmc_rust","enabled":true,
         "strategies":[{"name":"default","parameters":{}}]}]}"#;

    #[test]
    fn default_before_any_load() {
        let e = Evaluator::new();
        assert!(!e.has_data());
        // With no document, the compiled-in default is returned verbatim.
        assert!(e.is_enabled("meshmc_rust", "u1", "production", true));
        assert!(!e.is_enabled("meshmc_rust", "u1", "production", false));
    }

    #[test]
    fn load_then_evaluate() {
        let mut e = Evaluator::new();
        assert!(e.load(SAMPLE));
        assert!(e.has_data());
        assert!(e.is_enabled("meshmc_rust", "u1", "production", false));
    }

    #[test]
    fn malformed_body_keeps_previous() {
        let mut e = Evaluator::new();
        assert!(e.load(SAMPLE));
        assert!(!e.load(b"not json"));
        // Still answering from the good document.
        assert!(e.is_enabled("meshmc_rust", "u1", "production", false));
    }
}
