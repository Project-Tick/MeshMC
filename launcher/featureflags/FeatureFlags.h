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

#pragma once

#include <QByteArray>
#include <QDateTime>
#include <QList>
#include <QObject>
#include <QString>
#include <memory>

class QNetworkAccessManager;

/// Local, user-set override for a flag. Persisted across runs. Lets a user
/// force a flag on/off regardless of what the backend says (useful for testing
/// and for opting in/out of a rollout locally).
enum class FlagOverride {
	Auto = 0,  ///< no override; use the value evaluated from the backend
	ForceOn,   ///< always report this flag as enabled
	ForceOff   ///< always report this flag as disabled
};

/// One flag's state, as shown in the Feature Flags dialog.
struct FeatureFlagState {
	QString name;
	bool toggleEnabled = false;  ///< raw `enabled` from the document
	bool backendEffective = false;  ///< decision from the backend (no override)
	bool effective = false;      ///< final decision after applying any override
	QString strategies;          ///< comma-separated strategy names
	FlagOverride override = FlagOverride::Auto;  ///< current local override
};

/**
 * \brief Runtime feature-flag client for MeshMC.
 *
 * Thin C++ shell around the Rust decision engine
 * (crates/meshmc-featureflags). Responsibilities are split deliberately:
 *
 *   - C++ (this class) does the network transport with Qt: it issues the GET
 *     to the GitLab Unleash-compatible endpoint with the UNLEASH-INSTANCEID and
 *     UNLEASH-APPNAME headers, and persists the last good response to disk for
 *     offline launches.
 *   - Rust does the decision: parsing the `/client/features` document and
 *     evaluating activation strategies. It never touches the network.
 *
 * Resilience: on construction we synchronously seed the engine from the
 * on-disk cache (if any), so isEnabled() is usable immediately and offline.
 * refresh() then updates it in the background; when a fresh body arrives we
 * re-seed the engine and rewrite the cache.
 */
class FeatureFlags : public QObject
{
	Q_OBJECT
  public:
	explicit FeatureFlags(QObject* parent = nullptr);
	~FeatureFlags() override;

	/// Process-wide instance. Created by Application at startup.
	static FeatureFlags* instance();
	static void setInstance(FeatureFlags* ff);

	/**
	 * \brief Set the sticky id used for rollout strategies (percent /
	 * gradual). Typically a stable per-installation id. May be empty.
	 */
	void setUserId(const QString& userId);

	/**
	 * \brief Evaluate a flag.
	 * \param flag the Unleash toggle name (e.g. "meshmc_rust").
	 * \param defaultValue returned when flags are disabled at build time, no
	 *        document has been loaded yet, or the flag is unknown.
	 *
	 * Always safe to call; never blocks on the network.
	 */
	bool isEnabled(const QString& flag, bool defaultValue = false) const;

	/// Whether a feature document is currently loaded into the engine.
	bool hasData() const;

	/// List every known flag with its evaluated state for this installation.
	/// Empty when no document has been loaded.
	QList<FeatureFlagState> allFlags() const;

	/// When the currently loaded document was last fetched/seeded. Invalid if
	/// no document has been loaded.
	QDateTime lastUpdated() const;

	/// The configured endpoint, instance id presence and app name, for display.
	QString endpointUrl() const;
	QString appName() const;
	bool isConfigured() const;

	/// Local override accessors. Overrides are persisted and take precedence
	/// over the backend value in isEnabled().
	FlagOverride overrideFor(const QString& flag) const;
	void setOverride(const QString& flag, FlagOverride value);

  public slots:
	/**
	 * \brief Kick off an asynchronous fetch of the feature document. No-op when
	 * feature flags are disabled at build time (no instance id configured).
	 */
	void refresh();

  signals:
	/// Emitted after a successful refresh re-seeds the engine.
	void updated();

  private slots:
	void onReplyFinished();

  private:
	void seedFromCache();
	bool loadIntoEngine(const QByteArray& body);
	void writeCache(const QByteArray& body) const;
	QString cacheFilePath() const;

	struct Private;
	std::unique_ptr<Private> d;
};
