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

#include <QList>
#include <QObject>
#include <QString>
#include <QStringView>
#include "net/NetJob.h"

class UpdateCheckerTest;

/*!
 * Carries all information about an available update that is needed by the
 * UpdateController and the UpdateDialog.
 */
struct UpdateAvailableStatus {
	/// Normalized version string, e.g. "7.19.0"
	QString version;
	/// Release channel the entry came from, e.g. "stable" or "beta".
	QString channel;
	/// Web page for this release (from `<projt:release_page>`), if any.
	QString releasePage;
	/// Direct download URL for this platform's artifact.
	/// TODO(updater): filled in once the GitHub release resolution lands; the
	/// feed's `<projt:asset>` list is deliberately not used for this.
	QString downloadUrl;
	/// HTML release notes extracted from the feed's <description> element.
	QString releaseNotes;
	/// Expected SHA-256 of the artifact (lower-case hex). May be empty for
	/// legacy entries that did not embed a checksum.
	QString sha256;
	/// Expected file size in bytes, if the feed declared one. 0 = unknown.
	qint64 fileSize = 0;
};
Q_DECLARE_METATYPE(UpdateAvailableStatus)

/*!
 * UpdateChecker performs the dual-source update check used by MeshMC.
 *
 * Sources (Project Tick polyrepo, product-based release chain):
 *
 *   1. RSS feed at `BuildConfig.UPDATER_FEED_URL`
 *      (https://projecttick.org/product/meshmc/feed.xml).
 *      Authoritative: every `<item>` carries `<projt:version>` and
 *      `<projt:channel>` in the `https://projecttick.org/ns/product-feed`
 *      namespace.
 *
 *   2. `latest.json` mirror at `BuildConfig.UPDATER_LATEST_JSON_URL`
 *      (https://ftp.projecttick.org/Project-Tick/latest.json).
 *      Cross-check: `products.meshmc.stable.version` must match the feed's
 *      `<projt:version>`. It is a sanity gate only and never the primary
 *      source. It only applies when the picked entry is a stable one.
 *
 * The feed's `<projt:asset>` elements are intentionally ignored. Whether an
 * update exists is a pure version + channel question; the artifact itself is
 * fetched from the GitHub release.
 *
 * Algorithm:
 *
 *   1. Download both sources in parallel.
 *   2. Parse every `<item>` in the feed into a FeedItem (version, channel,
 *      notes, release page).
 *   3. Discard entries whose channel this build does not subscribe to (see
 *      isChannelAccepted) and keep the highest remaining version. The feed's
 *      ordering is not trusted.
 *   4. Parse `latest.json` and read `products.meshmc.stable.version`.
 *   5. If the picked version exceeds the running version, emit
 *      `updateAvailable()`. A feed/mirror disagreement is logged as a warning
 *      and the feed still wins (it is the authoritative source). If the feed
 *      itself fails to parse, the check fails outright.
 *
 * Platform / mode gating (runtime):
 *   - Linux + APPIMAGE env variable set  -> updater disabled (the AppImage
 *     is updated externally).
 *   - Linux + no portable.txt in app dir -> updater disabled.
 *   - Windows / macOS / Linux-portable   -> updater active.
 */
class UpdateChecker : public QObject
{
	Q_OBJECT

  public:
	explicit UpdateChecker(shared_qobject_ptr<QNetworkAccessManager> nam,
						   QObject* parent = nullptr);

	/*!
	 * Starts an asynchronous dual-source update check.
	 * If \a notifyNoUpdate is true, noUpdateFound() is emitted when the running
	 * version is already the latest; otherwise the signal is suppressed.
	 * Repeated calls while a check is in progress are silently ignored.
	 */
	void checkForUpdate(bool notifyNoUpdate);

	/*!
	 * Returns true if the updater may run on this platform / installation mode.
	 * Evaluated at runtime: AppImage detection, portable.txt presence, OS.
	 * Also checks that BuildConfig.UPDATER_ENABLED is true.
	 */
	static bool isUpdaterSupported();

	/*!
	 * A single `<item>` of the product feed, reduced to what the update
	 * decision needs. Asset elements are not represented on purpose.
	 */
	struct FeedItem {
		/// Normalized version, e.g. "7.19.0".
		QString version;
		/// Lower-cased `<projt:channel>`, e.g. "stable" or "beta".
		/// An entry without a channel is treated as "stable".
		QString channel;
		/// HTML release notes from `<description>`.
		QString releaseNotes;
		/// `<projt:release_page>`, if present.
		QString releasePage;
	};

  signals:
	//! Emitted when the feed reports a newer version and (when available) the
	//! latest.json mirror agrees with it.
	void updateAvailable(UpdateAvailableStatus status);

	//! Emitted when the check completes but the running version is current.
	void noUpdateFound();

	//! Emitted on any network or parse failure on the authoritative feed.
	void checkFailed(QString reason);

  private slots:
	void onSourcesDownloaded(bool notifyNoUpdate);
	void onDownloadsFailed(QString reason);

  private:
	friend class UpdateCheckerTest;

	static bool isPortableMode();
	static bool isAppImage();
	/// Returns current version as "MAJOR.MINOR.HOTFIX".
	static QString currentVersion();
	/// Strips a leading 'v' and returns a clean X.Y.Z string.
	static QString normalizeVersion(const QString& v);
	/// Compares two "X.Y.Z" strings numerically. Returns >0 if v1 > v2.
	static int compareVersions(const QString& v1, const QString& v2);
	/// Accepts the current Project Tick feed namespace.
	static bool isSupportedFeedNamespace(QStringView namespaceUri);

	/// The channel this build subscribes to, normalized. Defaults to
	/// "stable" when BuildConfig leaves it empty.
	static QString buildChannel();

	/// Risk ordering of a channel name: stable = 0, beta = 1.
	/// Returns -1 for anything we do not know about.
	static int channelRank(const QString& channel);

	/// True when a build on \a buildChannel may be offered an entry
	/// published on \a itemChannel. Unknown channels are never accepted, and
	/// an unknown build channel falls back to stable-only.
	static bool isChannelAccepted(const QString& itemChannel,
								  const QString& buildChannel);

	/// Parses every `<item>` of the feed. Returns false only when the XML
	/// itself is broken; entries without a version are skipped silently.
	static bool parseFeedItems(const QByteArray& feedData,
							   QList<FeedItem>* items, QString* parseError);

	/// Index of the highest-versioned entry this build may install, or -1
	/// when the feed holds nothing for our channel.
	static int pickBestItemIndex(const QList<FeedItem>& items,
								 const QString& buildChannel);

	/// Git tag a release is published under: "v" + version, e.g. "v7.19.0".
	static QString releaseTag(const QString& version);

	/*!
	 * Name of the release asset this build should install. Mirrors the
	 * naming produced by .github/workflows/release.yml:
	 *
	 *   Linux-Qt6             -> MeshMC-Linux-Qt6-Portable-v7.19.0.tar.gz
	 *   Linux-aarch64-Qt6     -> MeshMC-Linux-aarch64-Qt6-Portable-v7.19.0.tar.gz
	 *   Windows-MSVC-Qt6      -> MeshMC-Windows-MSVC[-Portable]-v7.19.0.zip
	 *   Windows-MinGW-w64-Qt6 -> MeshMC-Windows-MinGW-w64[-Portable]-v7.19.0.zip
	 *   macOS-Qt6             -> MeshMC-macOS-v7.19.0.zip
	 *
	 * \a artifact is BuildConfig.BUILD_ARTIFACT (the CI artifact name).
	 * Returns an empty string for a build that maps to no published asset.
	 */
	static QString releaseAssetName(const QString& artifact, const QString& tag,
									bool portable);

	/// Composes the GitHub release download URL out of the repository URL
	/// (BuildConfig.MESHMC_GIT), the artifact name and the version. Empty
	/// when this build has no asset to download.
	static QString makeGithubDownloadUrl(const QString& repoUrl,
										 const QString& artifact,
										 const QString& version, bool portable);

	/// Parse `latest.json` and return `products.meshmc.stable.version`
	/// (normalized). Returns an empty string if the mirror is malformed or
	/// the meshmc product is absent.
	static QString parseLatestJsonVersion(const QByteArray& jsonData);

	shared_qobject_ptr<QNetworkAccessManager> m_network;
	NetJob::Ptr m_checkJob;
	QByteArray m_feedData;
	QByteArray m_latestJsonData;
	bool m_checking = false;
};
