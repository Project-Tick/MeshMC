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

#include "UpdateChecker.h"

#include <QDebug>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QXmlStreamReader>

#include "BuildConfig.h"
#include "FileSystem.h"
#include "net/Download.h"

// ---------------------------------------------------------------------------
// Helpers — runtime mode detection
// ---------------------------------------------------------------------------

bool UpdateChecker::isPortableMode()
{
	// On Linux/BSD the binary lives in <prefix>/bin/, so portable.txt is one
	// level up (at the install prefix root) — matching Application.cpp's check.
	QDir dir(QCoreApplication::applicationDirPath());
	dir.cdUp();
	return QFile::exists(FS::PathCombine(dir.absolutePath(), "portable.txt"));
}

bool UpdateChecker::isAppImage()
{
	return !qEnvironmentVariable("APPIMAGE").isEmpty();
}

QString UpdateChecker::currentVersion()
{
	return QString("%1.%2.%3")
		.arg(BuildConfig.VERSION_MAJOR)
		.arg(BuildConfig.VERSION_MINOR)
		.arg(BuildConfig.VERSION_HOTFIX);
}

QString UpdateChecker::normalizeVersion(const QString& v)
{
	QString out = v.trimmed();
	if (out.startsWith('v', Qt::CaseInsensitive))
		out.remove(0, 1);
	return out;
}

int UpdateChecker::compareVersions(const QString& v1, const QString& v2)
{
	const QStringList parts1 = v1.split('.');
	const QStringList parts2 = v2.split('.');
	const int len = std::max(parts1.size(), parts2.size());
	for (int i = 0; i < len; ++i) {
		const qint64 a = (i < parts1.size()) ? parts1.at(i).toLongLong() : 0;
		const qint64 b = (i < parts2.size()) ? parts2.at(i).toLongLong() : 0;
		if (a != b)
			return (a > b) ? 1 : -1;
	}
	return 0;
}

bool UpdateChecker::isSupportedFeedNamespace(QStringView namespaceUri)
{
	return namespaceUri == u"https://projecttick.org/ns/product-feed";
}

UpdateChecker::BuildIdentity UpdateChecker::buildIdentity()
{
	BuildIdentity id;
	id.platform = BuildConfig.BUILD_PLATFORM_ID.toLower();
	id.arch = BuildConfig.BUILD_ARCH.toLower();
	id.portable = BuildConfig.BUILD_PORTABLE.toLower();
	id.kind = BuildConfig.BUILD_KIND.toLower();
	id.legacyArtifact = BuildConfig.BUILD_ARTIFACT;
	return id;
}

// ---------------------------------------------------------------------------
// Feed parsing
// ---------------------------------------------------------------------------

namespace
{
	struct AssetCandidate {
		QString name;
		QString url;
		QString platform;
		QString arch;
		QString portable;
		QString kind;
		QString sha256;
		qint64 size = 0;
	};

	/* Score a candidate asset against the build identity. Higher is better.
	 * A negative score (or zero when structured attributes are available but
	 * mandatory fields mismatch) means the candidate is not usable. */
	int scoreAsset(const AssetCandidate& a,
				   const UpdateChecker::BuildIdentity& id)
	{
		if (id.hasStructuredAttributes()) {
			// All structured attributes must agree where the build set
			// them; missing attributes on the asset side fall through to
			// the legacy substring check.
			if (a.platform.isEmpty() || a.arch.isEmpty() || a.kind.isEmpty()) {
				return 0;
			}
			if (a.platform != id.platform)
				return -1;
			if (a.arch != id.arch)
				return -1;
			if (a.kind != id.kind)
				return -1;
			int score = 100;
			if (!id.portable.isEmpty() && !a.portable.isEmpty()) {
				if (a.portable == id.portable) {
					score += 50;
				} else {
					score -= 25; // matched platform/arch/kind but wrong
								 // portable flag — still acceptable, just
								 // ranked lower than an exact match.
				}
			}
			return score;
		}

		// Legacy fallback: substring match on the artifact name.
		if (id.legacyArtifact.isEmpty()) {
			return 0;
		}
		if (a.name.contains(id.legacyArtifact, Qt::CaseInsensitive)) {
			return 10;
		}
		return -1;
	}
} // namespace

bool UpdateChecker::parseStableFeedItem(const QByteArray& feedData,
										const BuildIdentity& identity,
										QString* version, QString* downloadUrl,
										QString* releaseNotes, QString* sha256,
										qint64* fileSize, QString* parseError)
{
	Q_ASSERT(version);
	Q_ASSERT(downloadUrl);
	Q_ASSERT(releaseNotes);
	Q_ASSERT(sha256);
	Q_ASSERT(fileSize);
	Q_ASSERT(parseError);

	version->clear();
	downloadUrl->clear();
	releaseNotes->clear();
	sha256->clear();
	*fileSize = 0;
	parseError->clear();

	QXmlStreamReader xml(feedData);
	bool insideItem = false;
	bool isStable = false;
	QString itemVersion;
	QString itemNotes;
	QList<AssetCandidate> candidates;

	while (!xml.atEnd() && !xml.hasError()) {
		xml.readNext();

		if (xml.isStartElement()) {
			const QStringView name = xml.name();

			if (name == u"item") {
				insideItem = true;
				isStable = false;
				itemVersion.clear();
				itemNotes.clear();
				candidates.clear();
			} else if (insideItem) {
				if (isSupportedFeedNamespace(xml.namespaceUri())) {
					if (name == u"version") {
						itemVersion = xml.readElementText().trimmed();
					} else if (name == u"channel") {
						isStable =
							(xml.readElementText().trimmed() == u"stable");
					} else if (name == u"asset") {
						const auto attrs = xml.attributes();
						AssetCandidate c;
						c.name = attrs.value("name").toString();
						c.url = attrs.value("url").toString();
						c.platform =
							attrs.value("platform").toString().toLower();
						c.arch = attrs.value("arch").toString().toLower();
						c.portable =
							attrs.value("portable").toString().toLower();
						c.kind = attrs.value("kind").toString().toLower();
						c.sha256 = attrs.value("sha256").toString().toLower();
						c.size = attrs.value("size").toLongLong();
						if (!c.url.isEmpty()) {
							candidates.append(c);
						}
					}
				} else if (name == u"description" &&
						   xml.namespaceUri().isEmpty()) {
					itemNotes = xml.readElementText(
									   QXmlStreamReader::IncludeChildElements)
									.trimmed();
				}
			}
		} else if (xml.isEndElement() && xml.name() == u"item" && insideItem) {
			insideItem = false;
			if (!isStable || itemVersion.isEmpty()) {
				continue; // skip non-stable / malformed items, keep scanning
			}

			// Pick the highest-scoring asset for this build.
			int bestScore = 0;
			const AssetCandidate* best = nullptr;
			for (const auto& c : candidates) {
				const int s = scoreAsset(c, identity);
				if (s > bestScore) {
					bestScore = s;
					best = &c;
				}
			}

			*version = itemVersion;
			*releaseNotes = itemNotes;
			if (best) {
				*downloadUrl = best->url;
				*sha256 = best->sha256;
				*fileSize = best->size;
			}
			return true;
		}
	}

	if (xml.hasError())
		*parseError = xml.errorString();

	return false;
}

// ---------------------------------------------------------------------------
// latest.json parsing
// ---------------------------------------------------------------------------

QString UpdateChecker::parseLatestJsonVersion(const QByteArray& jsonData)
{
	QJsonParseError err{};
	const QJsonDocument doc = QJsonDocument::fromJson(jsonData, &err);
	if (err.error != QJsonParseError::NoError || !doc.isObject()) {
		return {};
	}

	const QJsonObject products =
		doc.object().value(QStringLiteral("products")).toObject();
	const QJsonObject meshmc =
		products.value(QStringLiteral("meshmc")).toObject();
	const QJsonObject stable =
		meshmc.value(QStringLiteral("stable")).toObject();

	const QString rawVersion =
		stable.value(QStringLiteral("version")).toString().trimmed();
	if (rawVersion.isEmpty()) {
		return {};
	}
	return normalizeVersion(rawVersion);
}

// ---------------------------------------------------------------------------
// Public
// ---------------------------------------------------------------------------

UpdateChecker::UpdateChecker(shared_qobject_ptr<QNetworkAccessManager> nam,
							 QObject* parent)
	: QObject(parent), m_network(nam)
{
}

bool UpdateChecker::isUpdaterSupported()
{
	if (!BuildConfig.UPDATER_ENABLED)
		return false;

#if defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD)
	// On Linux/BSD: disable unless this is a portable install and not an
	// AppImage.
	if (isAppImage())
		return false;
	if (!isPortableMode())
		return false;
#endif

	return true;
}

void UpdateChecker::checkForUpdate(bool notifyNoUpdate)
{
	if (!isUpdaterSupported()) {
		qDebug() << "UpdateChecker: updater not supported on this "
					"platform/mode. Skipping.";
		return;
	}

	if (m_checking) {
		qDebug() << "UpdateChecker: check already in progress, ignoring.";
		return;
	}

	qDebug() << "UpdateChecker: starting dual-source update check (feed="
			 << BuildConfig.UPDATER_FEED_URL
			 << "| latest.json=" << BuildConfig.UPDATER_LATEST_JSON_URL << ").";
	m_checking = true;
	m_feedData.clear();
	m_latestJsonData.clear();

	m_checkJob.reset(new NetJob("Update Check", m_network));
	m_checkJob->addNetAction(Net::Download::makeByteArray(
		QUrl(BuildConfig.UPDATER_FEED_URL), &m_feedData));
	if (!BuildConfig.UPDATER_LATEST_JSON_URL.isEmpty()) {
		m_checkJob->addNetAction(Net::Download::makeByteArray(
			QUrl(BuildConfig.UPDATER_LATEST_JSON_URL), &m_latestJsonData));
	}

	connect(m_checkJob.get(), &NetJob::succeeded,
			[this, notifyNoUpdate]() { onSourcesDownloaded(notifyNoUpdate); });
	connect(m_checkJob.get(), &NetJob::failed, this,
			&UpdateChecker::onDownloadsFailed);

	m_checkJob->start();
}

// ---------------------------------------------------------------------------
// Private slots
// ---------------------------------------------------------------------------

void UpdateChecker::onSourcesDownloaded(bool notifyNoUpdate)
{
	m_checkJob.reset();
	m_checking = false;

	const QByteArray feedData = m_feedData;
	const QByteArray latestJsonData = m_latestJsonData;
	m_feedData.clear();
	m_latestJsonData.clear();

	// ---- Parse the RSS feed (authoritative) -------------------------------
	const BuildIdentity identity = buildIdentity();

	QString feedVersion;
	QString downloadUrl;
	QString releaseNotes;
	QString sha256;
	qint64 fileSize = 0;
	QString feedParseError;
	if (!parseStableFeedItem(feedData, identity, &feedVersion, &downloadUrl,
							 &releaseNotes, &sha256, &fileSize,
							 &feedParseError)) {
		if (!feedParseError.isEmpty()) {
			qWarning() << "UpdateChecker: failed to parse update feed:"
					   << feedParseError;
			emit checkFailed(
				tr("Failed to parse update feed: %1").arg(feedParseError));
		} else {
			qWarning() << "UpdateChecker: no stable release entry found in the "
						  "update feed.";
			emit checkFailed(
				tr("No stable release entry found in the update feed."));
		}
		return;
	}

	feedVersion = normalizeVersion(feedVersion);

	if (downloadUrl.isEmpty()) {
		qWarning() << "UpdateChecker: feed has version" << feedVersion
				   << "but no asset matched the running build identity ("
				   << identity.platform << identity.arch << identity.kind
				   << "portable=" << identity.portable << ")";
		emit checkFailed(
			tr("Update feed has no asset matching this build (%1/%2/%3).")
				.arg(identity.platform, identity.arch, identity.kind));
		return;
	}

	// ---- Sanity-check against latest.json (optional) ----------------------
	if (!latestJsonData.isEmpty()) {
		const QString mirrorVersion = parseLatestJsonVersion(latestJsonData);
		if (mirrorVersion.isEmpty()) {
			qWarning()
				<< "UpdateChecker: latest.json malformed or missing meshmc "
				   "product; trusting the feed alone.";
		} else if (mirrorVersion != feedVersion) {
			qWarning() << "UpdateChecker: feed/mirror version mismatch — feed="
					   << feedVersion << "mirror=" << mirrorVersion
					   << "— trusting the feed (authoritative).";
		} else {
			qDebug() << "UpdateChecker: feed and latest.json mirror agree on"
					 << feedVersion;
		}
	} else {
		qDebug() << "UpdateChecker: latest.json mirror unavailable; trusting "
					"the feed alone.";
	}

	// ---- Compare against the running version ------------------------------
	const QString running = currentVersion();
	qDebug() << "UpdateChecker: feed version =" << feedVersion
			 << "| current =" << running;

	if (compareVersions(feedVersion, running) <= 0) {
		qDebug() << "UpdateChecker: already up to date.";
		if (notifyNoUpdate)
			emit noUpdateFound();
		return;
	}

	qDebug() << "UpdateChecker: update available:" << feedVersion;
	UpdateAvailableStatus status;
	status.version = feedVersion;
	status.downloadUrl = downloadUrl;
	status.releaseNotes = releaseNotes;
	status.sha256 = sha256;
	status.fileSize = fileSize;
	emit updateAvailable(status);
}

void UpdateChecker::onDownloadsFailed(QString reason)
{
	m_checking = false;
	m_checkJob.reset();
	m_feedData.clear();
	m_latestJsonData.clear();
	qCritical() << "UpdateChecker: download failed:" << reason;
	emit checkFailed(reason);
}
