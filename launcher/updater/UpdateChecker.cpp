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
	// Must agree with Application.cpp's portable detection, which is not the
	// same on every platform: on Windows the binary sits at the install root
	// and the marker lies next to it, while on Linux/BSD the binary lives in
	// <prefix>/bin/ and the marker is one level up.
#if defined(Q_OS_WIN32)
	return QFile::exists(FS::PathCombine(QCoreApplication::applicationDirPath(),
										 "portable.txt"));
#else
	QDir dir(QCoreApplication::applicationDirPath());
	dir.cdUp();
	return QFile::exists(FS::PathCombine(dir.absolutePath(), "portable.txt"));
#endif
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

// ---------------------------------------------------------------------------
// Channel policy
// ---------------------------------------------------------------------------

QString UpdateChecker::buildChannel()
{
	const QString channel = BuildConfig.UPDATE_CHANNEL.trimmed().toLower();
	return channel.isEmpty() ? QStringLiteral("stable") : channel;
}

int UpdateChecker::channelRank(const QString& channel)
{
	const QString c = channel.trimmed().toLower();
	if (c == QLatin1String("stable"))
		return 0;
	if (c == QLatin1String("beta"))
		return 1;
	return -1;
}

bool UpdateChecker::isChannelAccepted(const QString& itemChannel,
									  const QString& buildChannel)
{
	const int itemRank = channelRank(itemChannel);
	if (itemRank < 0) {
		// A channel we have never heard of is never installed: a future
		// "alpha" line must not land on today's users.
		return false;
	}

	const int buildRank = channelRank(buildChannel);
	if (buildRank < 0) {
		return itemRank == 0;
	}

	// A beta build also takes stable releases, so it can never end up
	// stranded behind the stable line.
	return itemRank <= buildRank;
}

// ---------------------------------------------------------------------------
// Feed parsing
// ---------------------------------------------------------------------------

bool UpdateChecker::parseFeedItems(const QByteArray& feedData,
								   QList<FeedItem>* items, QString* parseError)
{
	Q_ASSERT(items);
	Q_ASSERT(parseError);

	items->clear();
	parseError->clear();

	QXmlStreamReader xml(feedData);
	bool insideItem = false;
	FeedItem current;

	while (!xml.atEnd() && !xml.hasError()) {
		xml.readNext();

		if (xml.isStartElement()) {
			const QStringView name = xml.name();

			if (name == u"item") {
				insideItem = true;
				current = FeedItem();
			} else if (insideItem) {
				if (isSupportedFeedNamespace(xml.namespaceUri())) {
					// <projt:asset> is skipped on purpose: the artifact comes
					// from the GitHub release, not from the feed.
					if (name == u"version") {
						current.version =
							normalizeVersion(xml.readElementText());
					} else if (name == u"channel") {
						current.channel =
							xml.readElementText().trimmed().toLower();
					} else if (name == u"release_page") {
						current.releasePage = xml.readElementText().trimmed();
					}
				} else if (name == u"description" &&
						   xml.namespaceUri().isEmpty()) {
					current.releaseNotes =
						xml.readElementText(
							   QXmlStreamReader::IncludeChildElements)
							.trimmed();
				}
			}
		} else if (xml.isEndElement() && xml.name() == u"item" && insideItem) {
			insideItem = false;

			if (current.version.isEmpty()) {
				// Malformed entry — keep scanning, one bad item must not
				// blind the updater to the rest of the feed.
				continue;
			}
			if (current.channel.isEmpty()) {
				// Feeds that predate the channel element only ever carried
				// stable releases.
				current.channel = QStringLiteral("stable");
			}
			items->append(current);
		}
	}

	if (xml.hasError()) {
		*parseError = xml.errorString();
		return false;
	}

	return true;
}

int UpdateChecker::pickBestItemIndex(const QList<FeedItem>& items,
									 const QString& buildChannel)
{
	int best = -1;
	for (int i = 0; i < items.size(); ++i) {
		if (!isChannelAccepted(items.at(i).channel, buildChannel)) {
			continue;
		}
		// The feed is expected to be newest-first, but nothing enforces it,
		// so compare instead of trusting the order.
		if (best < 0 ||
			compareVersions(items.at(i).version, items.at(best).version) > 0) {
			best = i;
		}
	}
	return best;
}

// ---------------------------------------------------------------------------
// GitHub release resolution
// ---------------------------------------------------------------------------

QString UpdateChecker::releaseTag(const QString& version)
{
	const QString v = normalizeVersion(version);
	return v.isEmpty() ? QString() : QLatin1String("v") + v;
}

QString UpdateChecker::releaseAssetName(const QString& artifact,
										const QString& tag, bool portable)
{
	if (artifact.isEmpty() || tag.isEmpty()) {
		return {};
	}

	// macOS ships a single universal archive; the .dmg is for manual installs
	// only, the updater unpacks the .zip. Both macOS matrix entries (plain and
	// Xcode) publish under the same name.
	if (artifact.startsWith(QLatin1String("macOS"), Qt::CaseInsensitive)) {
		return QStringLiteral("MeshMC-macOS-%1.zip").arg(tag);
	}

	// Linux updates only ever run in portable mode (see isUpdaterSupported),
	// and the Qt suffix is part of the published name there.
	if (artifact.startsWith(QLatin1String("Linux"), Qt::CaseInsensitive)) {
		return QStringLiteral("MeshMC-%1-Portable-%2.tar.gz").arg(artifact, tag);
	}

	// Windows drops the "-Qt6" suffix and distinguishes the portable archive
	// from the plain one. The Setup .exe is deliberately not used: the updater
	// replaces files in place, it does not re-run an installer.
	if (artifact.startsWith(QLatin1String("Windows"), Qt::CaseInsensitive)) {
		QString base = artifact;
		if (base.endsWith(QLatin1String("-Qt6"), Qt::CaseInsensitive)) {
			base.chop(4);
		}
		return QStringLiteral("MeshMC-%1%2-%3.zip")
			.arg(base, portable ? QStringLiteral("-Portable") : QString(), tag);
	}

	return {};
}

QString UpdateChecker::makeGithubDownloadUrl(const QString& repoUrl,
											 const QString& artifact,
											 const QString& version,
											 bool portable)
{
	QString base = repoUrl.trimmed();
	if (base.isEmpty()) {
		return {};
	}
	// Order matters: a clone URL can carry both, as in ".../MeshMC.git/".
	while (base.endsWith(QLatin1Char('/'))) {
		base.chop(1);
	}
	if (base.endsWith(QLatin1String(".git"), Qt::CaseInsensitive)) {
		base.chop(4);
	}
	while (base.endsWith(QLatin1Char('/'))) {
		base.chop(1);
	}

	const QString tag = releaseTag(version);
	const QString asset = releaseAssetName(artifact, tag, portable);
	if (base.isEmpty() || tag.isEmpty() || asset.isEmpty()) {
		return {};
	}

	return QStringLiteral("%1/releases/download/%2/%3").arg(base, tag, asset);
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
	QList<FeedItem> items;
	QString feedParseError;
	if (!parseFeedItems(feedData, &items, &feedParseError)) {
		qWarning() << "UpdateChecker: failed to parse update feed:"
				   << feedParseError;
		emit checkFailed(
			tr("Failed to parse update feed: %1").arg(feedParseError));
		return;
	}

	const QString channel = buildChannel();
	const int bestIndex = pickBestItemIndex(items, channel);
	if (bestIndex < 0) {
		qWarning() << "UpdateChecker: the feed carries" << items.size()
				   << "entries, none of them on the" << channel << "channel.";
		emit checkFailed(
			tr("The update feed has no release for the %1 channel.")
				.arg(channel));
		return;
	}

	const FeedItem& best = items.at(bestIndex);
	const QString feedVersion = best.version;
	qDebug() << "UpdateChecker: picked feed entry" << feedVersion << "on the"
			 << best.channel << "channel (build channel:" << channel << ","
			 << items.size() << "entries in the feed).";

	// ---- Sanity-check against latest.json (optional) ----------------------
	// The mirror only publishes the stable line, so it can say nothing about
	// a beta entry.
	if (best.channel != QLatin1String("stable")) {
		qDebug() << "UpdateChecker: skipping the latest.json cross-check for a"
				 << best.channel << "entry.";
	} else if (!latestJsonData.isEmpty()) {
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
	status.channel = best.channel;
	status.releasePage = best.releasePage;
	status.releaseNotes = best.releaseNotes;

	// The artifact comes from the GitHub release, not from the feed.
	status.downloadUrl =
		makeGithubDownloadUrl(BuildConfig.MESHMC_GIT, BuildConfig.BUILD_ARTIFACT,
							  feedVersion, isPortableMode());
	if (status.downloadUrl.isEmpty()) {
		// Not fatal: MainWindow tells the user to grab the build by hand.
		qWarning() << "UpdateChecker: no release asset could be derived for "
					  "build artifact"
				   << BuildConfig.BUILD_ARTIFACT << "- offering" << feedVersion
				   << "without a download URL.";
	} else {
		qDebug() << "UpdateChecker: download URL =" << status.downloadUrl;
	}

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
