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

#include "PrepareStage.h"

#include "ArchiveReader.h"
#include "UpdateLock.h"
#include "UpdaterUtil.h"

#include "BuildConfig.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcess>
#include <QThread>
#include <QUrl>

namespace
{

	//! How long to keep waiting for MeshMC itself to shut down.
	constexpr int kLauncherExitTimeoutMs = 30 * 1000;

	//! How long the second stage gets to announce that it has taken over.
	constexpr int kHandOffTimeoutMs = 20 * 1000;
	constexpr int kHandOffPollMs = 100;

	QString downloadDirectory(const UpdaterOptions& options)
	{
		return QDir(options.dataDir)
			.absoluteFilePath(QLatin1String(UpdaterUtil::kDownloadDirName));
	}

	QString stagingDirectory(const UpdaterOptions& options)
	{
		return QDir(options.dataDir)
			.absoluteFilePath(QLatin1String(UpdaterUtil::kStagingDirName));
	}

} // namespace

PrepareStage::PrepareStage(const UpdaterOptions& options, UpdateLock& lock,
						   QObject* parent)
	: QObject(parent), m_options(options), m_lock(lock)
{
	m_network = new QNetworkAccessManager(this);
}

QString PrepareStage::updaterBinaryName() const
{
	QString name = BuildConfig.MESHMC_BINARY + QLatin1String("-updater");
#ifdef Q_OS_WIN
	name += QLatin1String(".exe");
#endif
	return name;
}

void PrepareStage::start()
{
	emit progress(QStringLiteral("Preparing update of %1").arg(m_options.root));

	// Anything left over from a previous update is dead weight now, and the
	// staging directory must be empty before we unpack into it.
	const int swept = UpdaterUtil::sweepDisplacedFiles(m_options.root);
	if (swept > 0)
		emit progress(QStringLiteral("Removed %1 file(s) left behind by an "
									 "earlier update")
						  .arg(swept));

	const QString staging = stagingDirectory(m_options);
	if (QFileInfo::exists(staging) &&
		!UpdaterUtil::removeDirectoryTree(staging)) {
		// Not fatal on its own -- the extraction overwrites what it needs --
		// but it usually means a previous second stage is somehow still alive.
		qWarning() << "PrepareStage: could not fully clear" << staging;
	}

	beginDownload();
}

void PrepareStage::beginDownload()
{
	const QUrl url(m_options.url);
	QString fileName = QFileInfo(url.path()).fileName();
	if (fileName.isEmpty())
		fileName = QStringLiteral("meshmc-update.bin");

	const QString directory = downloadDirectory(m_options);
	if (!UpdaterUtil::ensureDirectory(directory)) {
		fail(tr("Cannot create the download directory:\n%1").arg(directory));
		return;
	}

	m_downloadPath = QDir(directory).absoluteFilePath(fileName);
	QFile::remove(m_downloadPath);

	m_download = new QFile(m_downloadPath, this);
	if (!m_download->open(QIODevice::WriteOnly)) {
		fail(tr("Cannot write to the download directory:\n%1\n\n%2")
				 .arg(m_downloadPath, m_download->errorString()));
		return;
	}

	emit progress(QStringLiteral("Downloading %1 to %2")
					  .arg(m_options.url, m_downloadPath));

	QNetworkRequest request{url};
	request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
						 QNetworkRequest::NoLessSafeRedirectPolicy);
	request.setHeader(QNetworkRequest::UserAgentHeader,
					  BuildConfig.USER_AGENT_UNCACHED);

	m_reply = m_network->get(request);
	connect(m_reply, &QNetworkReply::readyRead, this,
			&PrepareStage::onDownloadReadyRead);
	connect(m_reply, &QNetworkReply::downloadProgress, this,
			&PrepareStage::onDownloadProgress);
	connect(m_reply, &QNetworkReply::finished, this,
			&PrepareStage::onDownloadFinished);
}

void PrepareStage::onDownloadReadyRead()
{
	// Stream to disk. A release artifact is a couple of hundred megabytes and
	// there is no reason for all of it to sit in memory at once.
	if (m_download != nullptr && m_reply != nullptr)
		m_download->write(m_reply->readAll());
}

void PrepareStage::onDownloadProgress(qint64 received, qint64 total)
{
	if (total <= 0)
		return;
	const int percent = static_cast<int>((received * 100) / total);
	if (percent / 10 == m_lastReportedPercent / 10)
		return; // one line per 10%, the log is not a progress bar
	m_lastReportedPercent = percent;
	emit progress(QStringLiteral("Downloaded %1%  (%2 of %3)")
					  .arg(percent)
					  .arg(UpdaterUtil::formatBytes(received),
						   UpdaterUtil::formatBytes(total)));
}

void PrepareStage::onDownloadFinished()
{
	if (m_reply == nullptr)
		return;

	m_reply->deleteLater();
	QNetworkReply* reply = m_reply;
	m_reply = nullptr;

	if (m_download != nullptr) {
		m_download->write(reply->readAll());
		m_download->flush();
		m_download->close();
	}

	if (reply->error() != QNetworkReply::NoError) {
		const int status =
			reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
		QFile::remove(m_downloadPath);
		fail(tr("Could not download the update.\n\n%1%2")
				 .arg(reply->errorString(),
					  status > 0 ? tr("\nHTTP status: %1").arg(status)
								 : QString()));
		return;
	}

	const qint64 size = QFileInfo(m_downloadPath).size();
	if (size <= 0) {
		QFile::remove(m_downloadPath);
		fail(tr("The downloaded update is empty:\n%1").arg(m_options.url));
		return;
	}

	emit progress(QStringLiteral("Download complete, %1")
					  .arg(UpdaterUtil::formatBytes(size)));
	unpackAndHandOff();
}

void PrepareStage::unpackAndHandOff()
{
	if (m_downloadPath.endsWith(QLatin1String(".exe"), Qt::CaseInsensitive)) {
		if (runBundledInstaller())
			emit finished(true, QString(), true);
		return;
	}

	const QString staging = stagingDirectory(m_options);
	emit progress(QStringLiteral("Unpacking into %1").arg(staging));

	const ArchiveReader::Result result =
		ArchiveReader::extract(m_downloadPath, staging);
	if (!result.ok) {
		fail(tr("Could not unpack the update.\n\n%1").arg(result.error));
		return;
	}

	emit progress(QStringLiteral("Unpacked %1 file(s), %2")
					  .arg(result.fileCount)
					  .arg(UpdaterUtil::formatBytes(result.byteCount)));

	// The archive is no longer needed and is the largest thing we wrote.
	QFile::remove(m_downloadPath);

	const QString payload = ArchiveReader::descendIntoSingleRoot(staging);
	const QString problem = describeStagingProblem(payload);
	if (!problem.isEmpty()) {
		fail(problem);
		return;
	}

	if (handOffTo(payload))
		emit finished(true, QString(), true);
}

QString PrepareStage::describeStagingProblem(const QString& stagingDir) const
{
	// Refuse to hand over to something that is not a MeshMC installation.
	// Without this check a wrong URL -- a source tarball, an HTML error page
	// saved as a zip -- would be copied straight over the install root.
	const QDir staged(stagingDir);

	const QString updater = staged.absoluteFilePath(updaterBinaryName());
	if (!QFileInfo(updater).isFile())
		return tr("The downloaded update does not look like a MeshMC "
				  "release: it contains no %1.\n\nUnpacked to:\n%2")
			.arg(updaterBinaryName(), QDir::toNativeSeparators(stagingDir));

	if (!m_options.exec.isEmpty()) {
		const QString appName = QFileInfo(m_options.exec).fileName();
		if (!QFileInfo(staged.absoluteFilePath(appName)).isFile())
			return tr("The downloaded update does not contain %1.\n\nUnpacked "
					  "to:\n%2")
				.arg(appName, QDir::toNativeSeparators(stagingDir));
	}

	return QString();
}

bool PrepareStage::runBundledInstaller()
{
#ifdef Q_OS_WIN
	emit progress(QStringLiteral("Artifact is an installer, launching %1")
					  .arg(m_downloadPath));

	QProcess installer;
	// Windows elevates anything that looks like a setup program unless it is
	// told not to; the installer writes into a per-user directory and must
	// stay in the user's own session.
	QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
	environment.insert(QStringLiteral("__COMPAT_LAYER"),
					   QStringLiteral("RUNASINVOKER"));
	installer.setProcessEnvironment(environment);
	installer.setProgram(m_downloadPath);
	installer.setWorkingDirectory(QFileInfo(m_downloadPath).absolutePath());

	if (!installer.startDetached()) {
		fail(tr("Could not start the downloaded installer:\n%1\n\n%2")
				 .arg(m_downloadPath, installer.errorString()));
		return false;
	}

	// From here the installer owns the installation, and we will never learn
	// how it went. Holding the lock would leave every future update warning
	// about an attempt that is not actually stuck.
	m_lock.release();
	emit progress(QStringLiteral(
		"Installer started; releasing the update lock and exiting."));
	return true;
#else
	fail(tr("This update is a Windows installer and cannot be applied on this "
			"platform:\n%1")
			 .arg(m_downloadPath));
	return false;
#endif
}

bool PrepareStage::handOffTo(const QString& stagingDir)
{
	// Wait here rather than in the second stage: once we exit, our own
	// libraries are released too, and the second stage can start immediately.
	if (m_options.waitPid > 0) {
		emit progress(QStringLiteral("Waiting for MeshMC (pid %1) to exit")
						  .arg(m_options.waitPid));
		if (!UpdaterUtil::waitForProcessExit(m_options.waitPid,
											 kLauncherExitTimeoutMs)) {
			fail(tr("MeshMC is still running and did not exit within %1 "
					"seconds.\n\nClose MeshMC and try again.")
					 .arg(kLauncherExitTimeoutMs / 1000));
			return false;
		}
	}

	const QString updater =
		QDir(stagingDir).absoluteFilePath(updaterBinaryName());
	const QStringList arguments = m_options.applyArguments(
		stagingDir, QCoreApplication::applicationPid());

	emit progress(QStringLiteral("Handing over to %1 %2")
					  .arg(updater, arguments.join(' ')));

	QProcess next;
#ifdef Q_OS_WIN
	QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
	environment.insert(QStringLiteral("__COMPAT_LAYER"),
					   QStringLiteral("RUNASINVOKER"));
	next.setProcessEnvironment(environment);
#endif
	next.setProgram(updater);
	next.setArguments(arguments);
	// Run it from the staging directory so it resolves its libraries there
	// and not, by accident, from the installation it is about to overwrite.
	next.setWorkingDirectory(stagingDir);

	qint64 applyPid = 0;
	if (!next.startDetached(&applyPid)) {
		fail(tr("Could not start the second stage of the update:\n%1\n\n%2")
				 .arg(updater, next.errorString()));
		return false;
	}

	// Starting it is not the same as it working. The binary we just launched
	// comes out of the downloaded release, so it is only as new as whatever is
	// being installed -- and a release older than the two-stage updater does
	// not understand these arguments at all. It exits immediately, without a
	// console to complain to, and the user is left with a launcher that closed
	// and an update that never happened.
	if (!waitForApplyStage(applyPid)) {
		fail(tr("The update could not be handed over to the version being "
				"installed.\n\nIts updater (%1) did not start. This happens "
				"when the release being installed is older than the current "
				"update mechanism.\n\nNothing has been changed -- %2 is still "
				"installed and working. Please install the new version "
				"manually.")
				 .arg(QDir::toNativeSeparators(updater),
					  BuildConfig.MESHMC_DISPLAYNAME));
		return false;
	}

	return true;
}

bool PrepareStage::waitForApplyStage(qint64 applyPid)
{
	emit progress(QStringLiteral("Waiting for the second stage (pid %1) to "
								 "take over the update")
					  .arg(applyPid));

	// The polling itself lives on UpdateLock, where it can be tested without
	// a download, a second process or a network.
	const UpdateLock::Takeover outcome =
		m_lock.waitForTakeover(QCoreApplication::applicationPid(), applyPid,
							   kHandOffTimeoutMs, kHandOffPollMs);

	const bool ok = outcome == UpdateLock::Takeover::Accepted ||
					outcome == UpdateLock::Takeover::Released;
	if (ok)
		emit progress(QStringLiteral("Hand-off confirmed: %1")
						  .arg(UpdateLock::describeTakeover(outcome)));
	else
		qCritical().noquote() << "PrepareStage: hand-off failed --"
							  << UpdateLock::describeTakeover(outcome);
	return ok;
}

void PrepareStage::fail(const QString& error)
{
	// This stage never writes into the installation, so any failure here
	// leaves it exactly as it was. Dropping the lock keeps the next attempt
	// from warning about a half-finished update that never touched anything.
	m_lock.release();
	emit finished(false, error, false);
}
