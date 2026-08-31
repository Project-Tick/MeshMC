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

//! Deserialisation model for the GitLab Unleash-compatible client API
//! (`/client/features`), plus the evaluation context supplied by C++.

use serde::Deserialize;
use std::collections::BTreeMap;

/// Top-level document returned by the Unleash client endpoint.
#[derive(Debug, Clone, Deserialize)]
pub struct FeatureDocument {
    #[serde(default)]
    pub version: u32,
    #[serde(default)]
    pub features: Vec<Feature>,
}

#[derive(Debug, Clone, Deserialize)]
pub struct Feature {
    pub name: String,
    #[serde(default)]
    pub enabled: bool,
    #[serde(default)]
    pub strategies: Vec<Strategy>,
}

#[derive(Debug, Clone, Deserialize)]
pub struct Strategy {
    pub name: String,
    /// Free-form string parameters. Unleash always serialises these as strings,
    /// even numeric ones (e.g. "percentage": "30").
    #[serde(default)]
    pub parameters: BTreeMap<String, String>,
}

impl FeatureDocument {
    pub fn parse(bytes: &[u8]) -> Result<Self, serde_json::Error> {
        serde_json::from_slice(bytes)
    }

    pub fn find(&self, name: &str) -> Option<&Feature> {
        self.features.iter().find(|f| f.name == name)
    }
}

/// Evaluation context. Mirrors the Unleash context fields MeshMC actually uses.
#[derive(Debug, Clone, Default)]
pub struct Context {
    /// Stable per-installation / per-user id used for sticky rollouts.
    pub user_id: String,
    /// Application name / environment (matches UNLEASH-APPNAME).
    pub app_name: String,
}
