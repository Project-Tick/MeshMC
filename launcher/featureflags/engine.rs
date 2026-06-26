// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: GPL-3.0-or-later WITH LicenseRef-MeshMC-MMCO-Module-Exception-1.0

//! Pure, network-free Unleash strategy evaluation.
//!
//! Given a parsed feature document and a context, decide whether a named flag
//! is enabled. The HTTP fetch is done by the C++ (Qt) side; this engine only
//! ever sees bytes and a context, which makes it fully deterministic and easy
//! to unit-test.

use crate::model::{Context, Feature, FeatureDocument, Strategy};
use crate::murmur::normalized_value;

/// Supported Unleash activation strategies. Unknown strategies evaluate to
/// `false` (fail-closed for that strategy), matching Unleash SDK behaviour.
fn eval_strategy(strategy: &Strategy, ctx: &Context) -> bool {
    match strategy.name.as_str() {
        // Always on when the toggle itself is enabled.
        "default" => true,

        // Enable for an explicit list of user ids.
        "userWithId" => {
            let Some(list) = strategy.parameters.get("userIds") else {
                return false;
            };
            if ctx.user_id.is_empty() {
                return false;
            }
            list.split(',')
                .map(|s| s.trim())
                .any(|id| id == ctx.user_id)
        }

        // Legacy percentage rollout, sticky on user id.
        "gradualRolloutUserId" => {
            let percentage = param_u32(strategy, "percentage");
            let group_id = strategy
                .parameters
                .get("groupId")
                .map(String::as_str)
                .unwrap_or("");
            rollout(percentage, &ctx.user_id, group_id)
        }

        // Modern flexible rollout. Stickiness can be userId / default / random.
        "flexibleRollout" => {
            let percentage = param_u32(strategy, "rollout");
            let group_id = strategy
                .parameters
                .get("groupId")
                .map(String::as_str)
                .unwrap_or("");
            let stickiness = strategy
                .parameters
                .get("stickiness")
                .map(String::as_str)
                .unwrap_or("default");
            match stickiness {
                // "random" is not sticky; without a RNG in this deterministic
                // engine we treat an empty id as "no bucket" and fall back to
                // the user id when present.
                "userId" | "default" | "random" => {
                    if ctx.user_id.is_empty() {
                        // No stable id: only a 100% rollout is unambiguously on.
                        percentage >= 100
                    } else {
                        rollout(percentage, &ctx.user_id, group_id)
                    }
                }
                _ => {
                    if ctx.user_id.is_empty() {
                        percentage >= 100
                    } else {
                        rollout(percentage, &ctx.user_id, group_id)
                    }
                }
            }
        }

        _ => false,
    }
}

fn param_u32(strategy: &Strategy, key: &str) -> u32 {
    strategy
        .parameters
        .get(key)
        .and_then(|s| s.trim().parse::<u32>().ok())
        .unwrap_or(0)
        .min(100)
}

fn rollout(percentage: u32, user_id: &str, group_id: &str) -> bool {
    if percentage == 0 {
        return false;
    }
    if percentage >= 100 {
        return true;
    }
    if user_id.is_empty() {
        return false;
    }
    normalized_value(user_id, group_id) <= percentage
}

/// Evaluate a single feature: the toggle must be enabled AND at least one
/// strategy must pass. A toggle with no strategies but `enabled: true` is on.
fn eval_feature(feature: &Feature, ctx: &Context) -> bool {
    if !feature.enabled {
        return false;
    }
    if feature.strategies.is_empty() {
        return true;
    }
    feature.strategies.iter().any(|s| eval_strategy(s, ctx))
}

/// Evaluate `flag` against `doc`. Returns `default_value` when the flag is not
/// present in the document.
pub fn is_enabled(doc: &FeatureDocument, flag: &str, ctx: &Context, default_value: bool) -> bool {
    match doc.find(flag) {
        Some(feature) => eval_feature(feature, ctx),
        None => default_value,
    }
}

/// A single flag's evaluated state, for listing in the UI.
pub struct EvaluatedFlag {
    pub name: String,
    /// The toggle's raw `enabled` field from the document.
    pub toggle_enabled: bool,
    /// The final decision after evaluating strategies against `ctx`.
    pub effective: bool,
    /// Comma-separated strategy names, for display.
    pub strategies: String,
}

/// Evaluate every flag in the document against `ctx`. Order follows the
/// document. Used by the Feature Flags dialog.
pub fn evaluate_all(doc: &FeatureDocument, ctx: &Context) -> Vec<EvaluatedFlag> {
    doc.features
        .iter()
        .map(|f| {
            let names: Vec<&str> = f.strategies.iter().map(|s| s.name.as_str()).collect();
            EvaluatedFlag {
                name: f.name.clone(),
                toggle_enabled: f.enabled,
                effective: eval_feature(f, ctx),
                strategies: if names.is_empty() {
                    String::from("(none)")
                } else {
                    names.join(", ")
                },
            }
        })
        .collect()
}

#[cfg(test)]
mod tests {
    use super::*;

    fn ctx(user: &str) -> Context {
        Context {
            user_id: user.to_string(),
            app_name: "production".to_string(),
        }
    }

    fn doc(json: &str) -> FeatureDocument {
        FeatureDocument::parse(json.as_bytes()).expect("valid json")
    }

    #[test]
    fn missing_flag_uses_default() {
        let d = doc(r#"{"version":1,"features":[]}"#);
        assert!(is_enabled(&d, "meshmc_rust", &ctx("u1"), true));
        assert!(!is_enabled(&d, "meshmc_rust", &ctx("u1"), false));
    }

    #[test]
    fn disabled_toggle_is_off() {
        let d = doc(
            r#"{"version":1,"features":[
                {"name":"meshmc_rust","enabled":false,
                 "strategies":[{"name":"default","parameters":{}}]}]}"#,
        );
        assert!(!is_enabled(&d, "meshmc_rust", &ctx("u1"), true));
    }

    #[test]
    fn default_strategy_on() {
        let d = doc(
            r#"{"version":1,"features":[
                {"name":"meshmc_rust","enabled":true,
                 "strategies":[{"name":"default","parameters":{}}]}]}"#,
        );
        assert!(is_enabled(&d, "meshmc_rust", &ctx("u1"), false));
    }

    #[test]
    fn enabled_with_no_strategies_is_on() {
        let d = doc(
            r#"{"version":1,"features":[
                {"name":"meshmc_rust","enabled":true,"strategies":[]}]}"#,
        );
        assert!(is_enabled(&d, "meshmc_rust", &ctx(""), false));
    }

    #[test]
    fn user_with_id_matches() {
        let d = doc(
            r#"{"version":1,"features":[
                {"name":"meshmc_rust","enabled":true,
                 "strategies":[{"name":"userWithId",
                   "parameters":{"userIds":"alice, bob ,carol"}}]}]}"#,
        );
        assert!(is_enabled(&d, "meshmc_rust", &ctx("bob"), false));
        assert!(!is_enabled(&d, "meshmc_rust", &ctx("dave"), false));
        assert!(!is_enabled(&d, "meshmc_rust", &ctx(""), false));
    }

    #[test]
    fn flexible_rollout_bounds() {
        let zero = doc(
            r#"{"version":1,"features":[
                {"name":"f","enabled":true,
                 "strategies":[{"name":"flexibleRollout",
                   "parameters":{"rollout":"0","stickiness":"default","groupId":"f"}}]}]}"#,
        );
        let full = doc(
            r#"{"version":1,"features":[
                {"name":"f","enabled":true,
                 "strategies":[{"name":"flexibleRollout",
                   "parameters":{"rollout":"100","stickiness":"default","groupId":"f"}}]}]}"#,
        );
        assert!(!is_enabled(&zero, "f", &ctx("anyone"), true));
        assert!(is_enabled(&full, "f", &ctx("anyone"), false));
        // 100% must be on even without a user id.
        assert!(is_enabled(&full, "f", &ctx(""), false));
    }

    #[test]
    fn flexible_rollout_is_sticky() {
        let d = doc(
            r#"{"version":1,"features":[
                {"name":"f","enabled":true,
                 "strategies":[{"name":"flexibleRollout",
                   "parameters":{"rollout":"50","stickiness":"default","groupId":"f"}}]}]}"#,
        );
        let first = is_enabled(&d, "f", &ctx("steady-user"), false);
        for _ in 0..50 {
            assert_eq!(first, is_enabled(&d, "f", &ctx("steady-user"), false));
        }
    }

    #[test]
    fn unknown_strategy_fails_closed() {
        let d = doc(
            r#"{"version":1,"features":[
                {"name":"f","enabled":true,
                 "strategies":[{"name":"someFutureStrategy","parameters":{}}]}]}"#,
        );
        assert!(!is_enabled(&d, "f", &ctx("u1"), true));
    }

    #[test]
    fn multiple_strategies_or() {
        let d = doc(
            r#"{"version":1,"features":[
                {"name":"f","enabled":true,
                 "strategies":[
                   {"name":"userWithId","parameters":{"userIds":"alice"}},
                   {"name":"default","parameters":{}}]}]}"#,
        );
        // default makes it on for everyone regardless of the userWithId miss.
        assert!(is_enabled(&d, "f", &ctx("zzz"), false));
    }
}
