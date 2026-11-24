/* Copyright 2026 Project Tick
 *
 * This source is subject to the Microsoft Permissive License (MS-PL).
 * Please see the COPYING.md file for more information.
 */

#include "JavaDownloadTask.h"
#include <QtConcurrentRun>
#include <QRegularExpression>
#ifndef Q_OS_WIN
#include <unistd.h>
#endif

#include <QDir>
#include <QProcess>
#include <QDebug>
#include <QDirIterator>
#include <QFileInfo>

#include "Application.h"
#include "FileSystem.h"
#include "Json.h"
#include "net/Download.h"
#include "net/ChecksumValidator.h"
#include "MMCZip.h"

JavaDownloadTask::JavaDownloadTask(const JavaDownload::RuntimeEntry& runtime,
								   const QString& targetDir, QObject* parent)
	: Task(parent), m_runtime(runtime), m_destination(targetDir)
{
}

JavaDownloadTask::~JavaDownloadTask()
{
    m_extraction.waitForFinished();
}

void JavaDownloadTask::executeTask()
{
    const QUrl url(m_runtime.url);
    if (url.scheme() != "https" || url.host().isEmpty()) {
        emitFailed(tr("The Java runtime download address is invalid."));
        return;
    }
    const auto parent = QFileInfo(m_destination).absolutePath();
    if (!FS::ensureFolderPathExists(parent)) {
        emitFailed(tr("Could not create the Java installation directory."));
        return;
    }
    m_staging.reset(new QTemporaryDir(QDir(parent).filePath(".installing-XXXXXX")));
    if (!m_staging->isValid()) {
        emitFailed(tr("Could not stage the Java installation."));
        return;
    }
    m_targetDir = FS::PathCombine(m_staging->path(), "runtime");
    if (!QDir().mkpath(m_targetDir)) {
        emitFailed(tr("Could not create the staging directory."));
        return;
    }
	if (m_runtime.downloadType == "manifest") {
		downloadManifest();
	} else {
		downloadArchive();
	}
}

void JavaDownloadTask::downloadArchive()
{
	setStatus(tr("Downloading %1...").arg(m_runtime.name));

	// Determine archive extension and path
	QUrl url(m_runtime.url);
	QString filename = url.fileName();
	if (filename.endsWith(".zip", Qt::CaseInsensitive))
        m_archivePath = FS::PathCombine(m_staging->path(), "runtime.zip");
    else if (filename.endsWith(".tar.gz", Qt::CaseInsensitive) || filename.endsWith(".tgz", Qt::CaseInsensitive))
        m_archivePath = FS::PathCombine(m_staging->path(), "runtime.tar.gz");
    else {
        emitFailed(tr("Unsupported Java archive format."));
        return;
    }
    const auto checksum = m_runtime.checksumHash;
    const auto type = m_runtime.checksumType;
    const auto expectedLength = type == "sha1" ? 40 : (type == "sha256" ? 64 : 0);
    if (!expectedLength || checksum.size() != expectedLength ||
        checksum.contains(QRegularExpression("[^0-9a-fA-F]"))) {
        emitFailed(tr("The Java archive has no valid checksum."));
        return;
    }

	// Create target directory
	if (!FS::ensureFolderPathExists(m_targetDir)) {
		emitFailed(
			tr("Failed to create target directory: %1").arg(m_targetDir));
		return;
	}

	m_downloadJob = new NetJob(tr("Java download"), APPLICATION->network());

	auto dl = Net::Download::makeFile(m_runtime.url, m_archivePath);

	// Add checksum validation
	if (!m_runtime.checksumHash.isEmpty()) {
		QCryptographicHash::Algorithm algo = QCryptographicHash::Sha256;
		if (m_runtime.checksumType == "sha1")
			algo = QCryptographicHash::Sha1;
		auto validator = new Net::ChecksumValidator(
			algo, QByteArray::fromHex(m_runtime.checksumHash.toLatin1()));
		dl->addValidator(validator);
	}

	m_downloadJob->addNetAction(dl);

	connect(m_downloadJob.get(), &NetJob::succeeded, this,
			&JavaDownloadTask::downloadFinished);
	connect(m_downloadJob.get(), &NetJob::failed, this,
			&JavaDownloadTask::downloadFailed);
	connect(m_downloadJob.get(), &NetJob::progress, this, &Task::setProgress);
	// Show the archive being fetched as its own line in the dialog.
	propagateStepsFrom(m_downloadJob.get());

	m_downloadJob->start();
}

void JavaDownloadTask::downloadFinished()
{
	m_downloadJob.reset();
	extractArchive();
}

void JavaDownloadTask::downloadFailed(QString reason)
{
    const bool aborted = m_downloadJob && m_downloadJob->wasAborted();
    m_downloadJob.reset();
    if (aborted)
        emitAborted();
    else
        emitFailed(tr("Failed to download Java: %1").arg(reason));
}

void JavaDownloadTask::extractArchive()
{
    setStatus(tr("Extracting %1...").arg(m_runtime.name));
    setProgress(0, 0);
    connect(&m_extraction, &QFutureWatcher<QString>::finished, this, [this] {
        const auto error = m_extraction.result();
        if (!error.isEmpty())
            emitFailed(error);
        else
            installStagedRuntime();
    });
    const auto archive = m_archivePath;
    const auto target = m_targetDir;
    m_extraction.setFuture(QtConcurrent::run([archive, target]() -> QString {
        if (archive.endsWith(".zip")) {
            return MMCZip::extractDir(archive, target) ? QString() : tr("Could not extract the Java ZIP archive.");
        }
        QProcess tar;
        tar.setWorkingDirectory(target);
        tar.start("tar", {"-xzf", archive});
        if (!tar.waitForStarted() || !tar.waitForFinished(300000) ||
            tar.exitStatus() != QProcess::NormalExit || tar.exitCode() != 0)
            return tr("Could not extract the Java archive: %1").arg(QString::fromLocal8Bit(tar.readAllStandardError()));
        return {};
    }));
}

void JavaDownloadTask::installStagedRuntime()
{
    const auto binary = findJavaBinary(m_targetDir);
    if (binary.isEmpty()) {
        emitFailed(tr("No Java executable was found in the downloaded runtime."));
        return;
    }
    const auto binaryPath = QFileInfo(binary).canonicalFilePath();
    if (!binaryPath.startsWith(QDir(m_targetDir).canonicalPath() + '/')) {
        emitFailed(tr("The downloaded Java executable points outside its installation directory."));
        return;
    }
#ifndef Q_OS_WIN
    if (!QFile::setPermissions(binary, QFile::permissions(binary) | QFileDevice::ExeUser)) {
        emitFailed(tr("Could not make Java executable."));
        return;
    }
#endif
    const auto relative = QDir(m_targetDir).relativeFilePath(binary);
    const auto backup = FS::PathCombine(m_staging->path(), "previous");
    QDir fs;
    const QFileInfo existing(m_destination);
    const bool replacing = existing.exists();
    if (existing.isSymLink() || (replacing && (!existing.isDir() || !fs.rename(m_destination, backup)))) {
        emitFailed(tr("Could not move the existing Java runtime aside."));
        return;
    }
    if (!fs.rename(m_targetDir, m_destination)) {
        if (replacing && !fs.rename(backup, m_destination)) {
            m_staging->setAutoRemove(false);
            emitFailed(tr("Could not restore the old Java runtime. It is available at %1.").arg(backup));
        } else {
            emitFailed(tr("Could not activate the downloaded Java runtime."));
        }
        return;
    }
    m_installedJavaPath = QDir(m_destination).filePath(relative);
    emitSucceeded();
}

void JavaDownloadTask::downloadManifest()
{
	setStatus(tr("Downloading manifest for %1...").arg(m_runtime.name));

	if (!FS::ensureFolderPathExists(m_targetDir)) {
		emitFailed(
			tr("Failed to create target directory: %1").arg(m_targetDir));
		return;
	}

	m_downloadJob =
		new NetJob(tr("Java manifest download"), APPLICATION->network());
	auto dl =
		Net::Download::makeByteArray(QUrl(m_runtime.url), &m_manifestData);

	if (m_runtime.checksumType == "sha1" && !m_runtime.checksumHash.isEmpty()) {
		dl->addValidator(new Net::ChecksumValidator(
			QCryptographicHash::Sha1,
			QByteArray::fromHex(m_runtime.checksumHash.toLatin1())));
	}

	m_downloadJob->addNetAction(dl);

	connect(m_downloadJob.get(), &NetJob::succeeded, this,
			&JavaDownloadTask::manifestDownloaded);
	connect(m_downloadJob.get(), &NetJob::failed, this,
			&JavaDownloadTask::downloadFailed);

	m_downloadJob->start();
}

void JavaDownloadTask::manifestDownloaded()
{
	m_downloadJob.reset();

	QJsonDocument doc;
	try {
		doc = Json::requireDocument(m_manifestData);
	} catch (const Exception& e) {
		m_manifestData.clear();
		emitFailed(tr("Failed to parse Java manifest: %1").arg(e.cause()));
		return;
	}
	m_manifestData.clear();

	if (!doc.isObject()) {
		emitFailed(tr("Failed to parse Java manifest."));
		return;
	}

	const auto files = doc.object()["files"].toObject();
    if (files.isEmpty()) {
        emitFailed(tr("The Java runtime manifest has no files."));
        return;
    }
    for (auto it = files.begin(); it != files.end(); ++it) {
        const auto key = it.key();
        const auto entry = it.value().toObject();
        const auto type = entry["type"].toString();
        const auto path = FS::PathInside(m_targetDir, key);
        if (key.isEmpty() || QDir::isAbsolutePath(key) || key.contains('\\') || key.contains(':') ||
            key.split('/').contains("..") || !path || *path == m_targetDir) {
            emitFailed(tr("Invalid file path in Java manifest: %1").arg(key));
            return;
        }
        if (type == "file") {
            const auto raw = entry["downloads"].toObject()["raw"].toObject();
            const auto sha1 = raw["sha1"].toString();
            const QUrl url(raw["url"].toString());
            if (url.scheme() != "https" || url.host().isEmpty() || sha1.size() != 40 ||
                sha1.contains(QRegularExpression("[^0-9a-fA-F]"))) {
                emitFailed(tr("Invalid download or checksum in Java manifest: %1").arg(key));
                return;
            }
        } else if (type == "link") {
            const auto target = entry["target"].toString();
            const auto relativeTarget = QFileInfo(key).path() + '/' + target;
            if (target.isEmpty() || QDir::isAbsolutePath(target) || target.contains('\\') ||
                target.contains(':') || !FS::PathInside(m_targetDir, relativeTarget)) {
                emitFailed(tr("Invalid link in Java manifest: %1").arg(key));
                return;
            }
        } else if (type != "directory") {
            emitFailed(tr("Unknown entry type in Java manifest: %1").arg(key));
            return;
        }
    }
	m_executableFiles.clear();
	m_linkEntries.clear();

	// Create directories first
	for (auto it = files.begin(); it != files.end(); ++it) {
		auto entry = it.value().toObject();
		if (entry["type"].toString() == "directory") {
			QDir().mkpath(FS::PathCombine(m_targetDir, it.key()));
		}
	}

	// Queue file downloads
	setStatus(tr("Downloading %1 files...").arg(m_runtime.name));
	m_downloadJob =
		new NetJob(tr("Java runtime files"), APPLICATION->network());

	for (auto it = files.begin(); it != files.end(); ++it) {
		auto entry = it.value().toObject();

		if (entry["type"].toString() == "file") {
			auto downloads = entry["downloads"].toObject();
			auto raw = downloads["raw"].toObject();

			QString url = raw["url"].toString();
			QString sha1 = raw["sha1"].toString();
			QString filePath = FS::PathCombine(m_targetDir, it.key());

			// Ensure parent directory exists
			QFileInfo fi(filePath);
			QDir().mkpath(fi.absolutePath());

			auto dl = Net::Download::makeFile(QUrl(url), filePath);
			if (!sha1.isEmpty()) {
				dl->addValidator(new Net::ChecksumValidator(
					QCryptographicHash::Sha1,
					QByteArray::fromHex(sha1.toLatin1())));
			}
			m_downloadJob->addNetAction(dl);

			if (entry["executable"].toBool()) {
				m_executableFiles.append(filePath);
			}
		} else if (entry["type"].toString() == "link") {
			m_linkEntries.append({it.key(), entry["target"].toString()});
		}
	}

	connect(m_downloadJob.get(), &NetJob::succeeded, this,
			&JavaDownloadTask::manifestFilesDownloaded);
	connect(m_downloadJob.get(), &NetJob::failed, this,
			&JavaDownloadTask::downloadFailed);
	connect(m_downloadJob.get(), &NetJob::progress, this, &Task::setProgress);
	// One line per file being downloaded.
	propagateStepsFrom(m_downloadJob.get());

	m_downloadJob->start();
}

void JavaDownloadTask::manifestFilesDownloaded()
{
	m_downloadJob.reset();

	// Create symlinks
	for (const auto& link : m_linkEntries) {
		QString linkPath = FS::PathCombine(m_targetDir, link.first);
		QFileInfo fi(linkPath);
		QDir().mkpath(fi.absolutePath());
#ifdef Q_OS_WIN
        const bool linked = QFile::copy(fi.dir().filePath(link.second), linkPath);
#else
        const bool linked = symlink(QFile::encodeName(link.second).constData(),
                                    QFile::encodeName(linkPath).constData()) == 0;
#endif
        if (!linked) {
            emitFailed(tr("Could not create Java runtime link: %1").arg(link.first));
            return;
        }
	}

	// Set executable permissions
	for (const auto& path : m_executableFiles) {
		QFile::setPermissions(
			path, QFile::permissions(path) | QFileDevice::ExeUser |
					  QFileDevice::ExeGroup | QFileDevice::ExeOther);
	}

	installStagedRuntime();
}

QString JavaDownloadTask::findJavaBinary(const QString& dir) const
{
#if defined(Q_OS_WIN)
	QString binaryName = "javaw.exe";
#else
	QString binaryName = "java";
#endif

	// Search for java binary in bin/ subdirectories
	QDirIterator it(dir, QStringList() << binaryName, QDir::Files,
					QDirIterator::Subdirectories);
	while (it.hasNext()) {
		it.next();
		QString path = it.filePath();
		if (path.contains("/bin/")) {
			return path;
		}
	}

	// Fallback: any match
	QDirIterator it2(dir, QStringList() << binaryName, QDir::Files,
					 QDirIterator::Subdirectories);
	if (it2.hasNext()) {
		it2.next();
		return it2.filePath();
	}

	return QString();
}
