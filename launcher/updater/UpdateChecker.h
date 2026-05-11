/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-FileContributor: Project Tick
 * SPDX-License-Identifier: GPL-3.0-or-later WITH LicenseRef-MeshMC-MMCO-Module-Exception-1.0
 *
 *   MeshMC - A Custom Launcher for Minecraft
 *   Copyright (C) 2026 Project Tick
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version, with the additional permission
 *   described in the MeshMC MMCO Module Exception 1.0.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 *   You should have received a copy of the MeshMC MMCO Module Exception 1.0
 *   along with this program.  If not, see <https://projecttick.org/licenses/>.
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
	/// Normalized version string, e.g. "7.19.0"
	QString version;
	/// Direct download URL for this platform's artifact (from the feed).
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
 *      Authoritative: provides the latest stable item, per-platform asset
 *      list with `platform`, `arch`, `portable`, `kind`, `sha256` and
 *      `size` attributes in the `https://projecttick.org/ns/product-feed`
 *      namespace.
 *
 *   2. `latest.json` mirror at `BuildConfig.UPDATER_LATEST_JSON_URL`
 *      (https://ftp.projecttick.org/Project-Tick/latest.json).
 *      Cross-check: `products.meshmc.stable.version` must match the feed's
 *      `<projt:version>`. The mirror does not carry SHA-256 checksums today,
 *      so it is treated as a sanity gate only and never used as the primary
 *      source.
 *
 * Algorithm:
 *
 *   1. Download both sources in parallel.
 *   2. Parse the first stable `<item>` from the feed. Pick the asset whose
 *      `platform`/`arch`/`portable`/`kind` attributes best match the running
 *      build's identity (see BuildIdentity below). The legacy substring
 *      match on `BUILD_ARTIFACT` is kept as a last-resort fallback.
 *   3. Parse `latest.json` and read `products.meshmc.stable.version`.
 *   4. If the two versions agree and exceed the running version, emit
 *      `updateAvailable()`. If they disagree, the check is downgraded to a
 *      warning and we still trust the feed (it is the authoritative source)
 *      but log the discrepancy. If the feed itself fails to parse, the
 *      check fails outright.
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
	 * Describes the running build's identity used to pick a matching asset
	 * out of the feed. Populated from BuildConfig at compile time.
	 */
	struct BuildIdentity {
		QString platform; /* "linux" | "windows" | "macos"          */
		QString arch;	  /* "x86_64" | "aarch64"                   */
		QString portable; /* "true" | "false"                       */
		QString kind;	  /* "archive" | "appimage" | "installer"   */
		/* Legacy substring fallback (e.g. "MeshMC-Linux-Portable"). */
		QString legacyArtifact;

		bool hasStructuredAttributes() const
		{
			return !platform.isEmpty() && !arch.isEmpty() && !kind.isEmpty();
		}
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

	/// Build the runtime build identity from BuildConfig.
	static BuildIdentity buildIdentity();

	/// Extracts the first stable <item> from the feed and reports parse
	/// errors. Picks the matching asset using the structured attribute set
	/// from \a identity, falling back to the legacy artifact substring.
	static bool parseStableFeedItem(const QByteArray& feedData,
									const BuildIdentity& identity,
									QString* version, QString* downloadUrl,
									QString* releaseNotes, QString* sha256,
									qint64* fileSize, QString* parseError);

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
