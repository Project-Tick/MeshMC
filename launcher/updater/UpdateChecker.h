/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-FileContributor: Project Tick
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 *   MeshMC - A Custom Launcher for Minecraft
 *   Copyright (C) 2026 Project Tick
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

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
	/// Normalized version string, e.g. "7.1.0"
	QString version;
	/// Direct download URL for this platform's artifact (from the feed).
	QString downloadUrl;
	/// HTML release notes extracted from the feed's <description> element.
	QString releaseNotes;
};
Q_DECLARE_METATYPE(UpdateAvailableStatus)

/*!
 * UpdateChecker performs the two-source update check used by MeshMC.
 *
 * Algorithm:
 *   1. Download the RSS feed at BuildConfig.UPDATER_FEED_URL in parallel with
 *      the GitHub releases JSON at BuildConfig.UPDATER_GITHUB_API_URL.
 *   2. Parse the latest stable <item> from the feed -> extract <projt:version>
 *      and the <projt:asset> URL whose name contains
 *      BuildConfig.BUILD_ARTIFACT. Feed metadata accepts both the current
 *      Project Tick product-feed namespace and the legacy launcher-feed
 *      namespace.
 *   3. From the GitHub JSON, locate the components.json asset and download it.
 *      Extract the canonical MeshMC version from components.meshmc.version.
 *      Fallback: strip leading "v" from tag_name if components.json is absent.
 *   4. Both versions must match and be greater than the running version;
 *      otherwise the check is considered a no-update or failure.
 *
 * Platform / mode gating (runtime):
 *   - Linux + APPIMAGE env variable set  -> updater disabled.
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
	 * Starts an asynchronous two-source update check.
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

  signals:
	//! Emitted when both sources agree that a newer version is available.
	void updateAvailable(UpdateAvailableStatus status);

	//! Emitted when the check completes but the running version is current.
	void noUpdateFound();

	//! Emitted on any network or parse failure.
	void checkFailed(QString reason);

  private slots:
	void onPhase1Finished(bool notifyNoUpdate);
	void onComponentsDownloaded(bool notifyNoUpdate);
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
	/// Accepts current and legacy Project Tick feed namespaces.
	static bool isSupportedFeedNamespace(QStringView namespaceUri);
	/// Extracts the first stable item from the feed and reports parse errors.
	static bool parseStableFeedItem(const QByteArray& feedData,
									   const QString& buildArtifact,
									   QString* version,
									   QString* downloadUrl,
									   QString* releaseNotes,
									   QString* parseError);

	/// Parse GitHub releases JSON, return components.json browser_download_url.
	/// Stores tag_name fallback in m_githubTagVersion.
	QString findComponentsUrl();

	/// Finalize the version comparison after all data is available.
	void finalizeCheck(bool notifyNoUpdate, const QString& githubVersion);

	shared_qobject_ptr<QNetworkAccessManager> m_network;
	NetJob::Ptr m_checkJob;
	QByteArray m_feedData;
	QByteArray m_githubData;
	QByteArray m_componentsData;
	bool m_checking = false;

	// Intermediate state between Phase 1 and Phase 2
	QString m_feedVersion;
	QString m_downloadUrl;
	QString m_releaseNotes;
	QString m_githubTagVersion; // fallback if no components.json
};
