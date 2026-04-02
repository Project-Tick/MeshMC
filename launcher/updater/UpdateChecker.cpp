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

#include "UpdateChecker.h"

#include <QDebug>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QXmlStreamReader>

#include "BuildConfig.h"
#include "FileSystem.h"
#include "net/Download.h"

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

bool UpdateChecker::isPortableMode()
{
	// portable.txt lives next to the application binary.
	return QFile::exists(FS::PathCombine(QCoreApplication::applicationDirPath(),
										 "portable.txt"));
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
		const int a = (i < parts1.size()) ? parts1.at(i).toInt() : 0;
		const int b = (i < parts2.size()) ? parts2.at(i).toInt() : 0;
		if (a != b)
			return a - b;
	}
	return 0;
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

	qDebug() << "UpdateChecker: starting dual-source update check.";
	m_checking = true;
	m_feedData.clear();
	m_githubData.clear();

	m_checkJob.reset(new NetJob("Update Check", m_network));
	m_checkJob->addNetAction(Net::Download::makeByteArray(
		QUrl(BuildConfig.UPDATER_FEED_URL), &m_feedData));
	m_checkJob->addNetAction(Net::Download::makeByteArray(
		QUrl(BuildConfig.UPDATER_GITHUB_API_URL), &m_githubData));

	connect(m_checkJob.get(), &NetJob::succeeded,
			[this, notifyNoUpdate]() { onDownloadsFinished(notifyNoUpdate); });
	connect(m_checkJob.get(), &NetJob::failed, this,
			&UpdateChecker::onDownloadsFailed);

	m_checkJob->start();
}

// ---------------------------------------------------------------------------
// Private slots
// ---------------------------------------------------------------------------

void UpdateChecker::onDownloadsFinished(bool notifyNoUpdate)
{
	m_checking = false;
	m_checkJob.reset();

	// ---- Parse the RSS feed -----------------------------------------------
	// We look for the first <item> whose <projt:channel> == "stable".
	// From that item we read <projt:version> and the <projt:asset> whose
	// name attribute contains BuildConfig.BUILD_ARTIFACT.

	QString feedVersion;
	QString downloadUrl;
	QString releaseNotes;

	{
		QXmlStreamReader xml(m_feedData);
		m_feedData.clear();

		bool insideItem = false;
		bool isStable = false;
		QString itemVersion;
		QString itemUrl;
		QString itemNotes;

		// We iterate forward and take the FIRST stable item we encounter
		// (the feed lists newest first).
		while (!xml.atEnd() && !xml.hasError()) {
			xml.readNext();

			if (xml.isStartElement()) {
				const QStringView name = xml.name();

				if (name == u"item") {
					insideItem = true;
					isStable = false;
					itemVersion.clear();
					itemUrl.clear();
					itemNotes.clear();
				} else if (insideItem) {
					if (xml.namespaceUri() ==
						u"https://projecttick.org/ns/projt-launcher/feed") {
						if (name == u"version") {
							itemVersion = xml.readElementText().trimmed();
						} else if (name == u"channel") {
							isStable =
								(xml.readElementText().trimmed() == "stable");
						} else if (name == u"asset") {
							const QString assetName =
								xml.attributes().value("name").toString();
							const QString assetUrl =
								xml.attributes().value("url").toString();
							if (!BuildConfig.BUILD_ARTIFACT.isEmpty() &&
								assetName.contains(BuildConfig.BUILD_ARTIFACT,
												   Qt::CaseInsensitive)) {
								itemUrl = assetUrl;
							}
						}
					} else if (name == u"description" &&
							   xml.namespaceUri().isEmpty()) {
						itemNotes =
							xml.readElementText(
								   QXmlStreamReader::IncludeChildElements)
								.trimmed();
					}
				}
			} else if (xml.isEndElement() && xml.name() == u"item" &&
					   insideItem) {
				insideItem = false;
				if (isStable && !itemVersion.isEmpty()) {
					// First stable item wins.
					feedVersion = normalizeVersion(itemVersion);
					downloadUrl = itemUrl;
					releaseNotes = itemNotes;
					break;
				}
			}
		}

		if (xml.hasError()) {
			emit checkFailed(
				tr("Failed to parse update feed: %1").arg(xml.errorString()));
			return;
		}
	}

	if (feedVersion.isEmpty()) {
		emit checkFailed(
			tr("No stable release entry found in the update feed."));
		return;
	}

	if (downloadUrl.isEmpty()) {
		qWarning() << "UpdateChecker: feed has version" << feedVersion
				   << "but no asset matching BUILD_ARTIFACT '"
				   << BuildConfig.BUILD_ARTIFACT << "'";
		// We can still report an update even without a direct URL —
		// the UpdateDialog will inform the user.
	}

	// ---- Parse the GitHub releases JSON -----------------------------------
	// Expect the GitHub REST API format: { "tag_name": "vX.Y.Z", ... }

	QString githubVersion;
	{
		QJsonParseError jsonError;
		const QJsonDocument doc =
			QJsonDocument::fromJson(m_githubData, &jsonError);
		m_githubData.clear();

		if (jsonError.error != QJsonParseError::NoError || !doc.isObject()) {
			emit checkFailed(tr("Failed to parse GitHub releases response: %1")
								 .arg(jsonError.errorString()));
			return;
		}

		const QString tag = doc.object().value("tag_name").toString().trimmed();
		if (tag.isEmpty()) {
			emit checkFailed(
				tr("GitHub releases response contained no tag_name field."));
			return;
		}
		githubVersion = normalizeVersion(tag);
	}

	qDebug() << "UpdateChecker: feed version =" << feedVersion
			 << "| github version =" << githubVersion
			 << "| current =" << currentVersion();

	// ---- Cross-check both sources -----------------------------------------
	if (feedVersion != githubVersion) {
		qDebug() << "UpdateChecker: feed and GitHub disagree on version — no "
					"update reported.";
		if (notifyNoUpdate)
			emit noUpdateFound();
		return;
	}

	// ---- Compare against the running version ------------------------------
	if (compareVersions(feedVersion, currentVersion()) <= 0) {
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
	emit updateAvailable(status);
}

void UpdateChecker::onDownloadsFailed(QString reason)
{
	m_checking = false;
	m_checkJob.reset();
	m_feedData.clear();
	m_githubData.clear();
	qCritical() << "UpdateChecker: download failed:" << reason;
	emit checkFailed(reason);
}
