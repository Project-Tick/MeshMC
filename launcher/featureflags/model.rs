// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: GPL-3.0-or-later WITH LicenseRef-MeshMC-MMCO-Module-Exception-1.0

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
