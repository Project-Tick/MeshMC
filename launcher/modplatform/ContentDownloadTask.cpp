/* Copyright 2026 Project Tick
 *
 * This source is subject to the Microsoft Permissive License (MS-PL).
 * Please see the COPYING.md file for more information.
 */

#include "ContentDownloadTask.h"
#include "Application.h"
#include "minecraft/mod/ModMetadataIndex.h"
#include "net/Download.h"
#include "net/ChecksumValidator.h"
#include "net/SizeValidator.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QCryptographicHash>
#include <QSet>

ContentDownloadTask::ContentDownloadTask(
	const QList<ModPlatform::DownloadItem>& items, const QString& targetDir,
	QObject* parent)
	: Task(parent), m_items(items), m_targetDir(targetDir)
{
}

void ContentDownloadTask::setMetadataIndex(
	std::shared_ptr<ModMetadataIndex> index)
{
	m_metadata = std::move(index);
}

bool ContentDownloadTask::abort()
{
	if (m_aborted) {
		return true;
	}
	m_aborted = true;

	if (m_netJob && m_netJob->isRunning()) {
		m_netJob->abort();
	}

	/* Only a running task may report a verdict - emitAborted() complains
	 * loudly otherwise - but the latch above still holds, so a task
	 * called off before it started will simply do nothing. */
	if (isRunning()) {
		emitAborted();
	}
	return true;
}

void ContentDownloadTask::executeTask()
{
	if (m_items.isEmpty()) {
		emitSucceeded();
		return;
	}

	QDir dir(m_targetDir);
	if (!dir.exists()) {
		dir.mkpath(".");
	}

    const auto isFileName = [](const QString& name) {
        return !name.isEmpty() && name != "." && name != ".." &&
               !name.contains('/') && !name.contains('\\') && !name.contains(':');
    };
    for (const auto& item : m_items) {
        if (!isFileName(item.fileName) ||
            (!item.replacesFileName.isEmpty() && !isFileName(item.replacesFileName))) {
            emitFailed(tr("Invalid download file name: %1").arg(item.fileName));
            return;
        }
    }

	setStatus(tr("Downloading %1 file(s)...").arg(m_items.size()));

	m_netJob = new NetJob("ContentDownload", APPLICATION->network());

	int skipped = 0;
	// Last-resort safety net: even if an upstream stage (dependency
	// resolver, conflict analyzer) failed to catch it, never let the same
	// target file name or the same platform+project be queued twice in one
	// batch. Without this, two versions of the same mod (e.g. one pulled in
	// as a dependency, one explicitly selected) could download side by side.
	QSet<QString> queuedFileNames;
	QSet<QString> queuedProjectKeys;
	for (const auto& item : m_items) {
        QString targetPath = dir.filePath(item.fileName);
        if (!item.replacesFileName.isEmpty()) {
            const auto previous = dir.filePath(item.replacesFileName);
            if (!QFileInfo::exists(previous) && QFileInfo::exists(previous + ".disabled"))
                targetPath += ".disabled";
        }

		if (!item.fileName.isEmpty() &&
			queuedFileNames.contains(item.fileName)) {
			qWarning() << "ContentDownload: skipping duplicate file name in "
						  "batch:"
					   << item.fileName;
			skipped++;
			continue;
		}
		QString projectKey;
		if (!item.platform.isEmpty() && !item.projectId.isEmpty()) {
			projectKey = item.platform + ":" + item.projectId;
			if (queuedProjectKeys.contains(projectKey)) {
				qWarning() << "ContentDownload: skipping duplicate "
							  "project in batch:"
						   << item.name << "(" << projectKey << ")";
				skipped++;
				continue;
			}
		}
		if (!item.fileName.isEmpty()) {
			queuedFileNames.insert(item.fileName);
		}
		if (!projectKey.isEmpty()) {
			queuedProjectKeys.insert(projectKey);
		}

		// Skip if file already exists with matching SHA1 checksum and the
		// caller did not request a forced replace.
		if (!item.replaceExisting && QFileInfo::exists(targetPath) &&
			!item.sha1.isEmpty()) {
			QFile existingFile(targetPath);
			if (existingFile.open(QIODevice::ReadOnly)) {
				QCryptographicHash hash(QCryptographicHash::Sha1);
				hash.addData(&existingFile);
				existingFile.close();
				if (hash.result().toHex() == item.sha1.toLatin1().toLower()) {
					qDebug() << "Skipping" << item.fileName
							 << "- already exists with matching SHA1";
					finishItem(item);
					skipped++;
					continue;
				}
			}
		}

		auto dl = Net::Download::makeFile(QUrl(item.downloadUrl), targetPath);
		if (!item.sha1.isEmpty()) {
			dl->addValidator(new Net::ChecksumValidator(
				QCryptographicHash::Sha1,
				QByteArray::fromHex(item.sha1.toLatin1())));
		}
        if (item.fileSize > 0)
            dl->addValidator(new Net::SizeValidator(item.fileSize));
        connect(dl.get(), &NetAction::succeeded, this, [this, item](int) {
            finishItem(item);
        });
        m_netJob->addNetAction(dl);
	}

	if (skipped > 0) {
		qDebug() << "Skipped" << skipped << "already-existing file(s)";
	}

	// If all files were skipped, nothing to download
	if (m_netJob->size() == 0) {
		m_netJob.reset();
		emitSucceeded();
		return;
	}

	connect(m_netJob.get(), &NetJob::succeeded, this,
			&ContentDownloadTask::onDownloadSucceeded);
	connect(m_netJob.get(), &NetJob::failed, this,
			&ContentDownloadTask::onDownloadFailed);
	connect(m_netJob.get(), &NetJob::progress, this,
			&ContentDownloadTask::onDownloadProgress);

    propagateStepsFrom(m_netJob.get());
	m_netJob->start();
}

void ContentDownloadTask::finishItem(const ModPlatform::DownloadItem& item)
{
    const QDir dir(m_targetDir);
    if (!item.replacesFileName.isEmpty() && item.replacesFileName != item.fileName) {
        const QString previous = dir.filePath(item.replacesFileName);
        for (const auto& path : {previous, previous + QStringLiteral(".disabled")}) {
            if (QFile::exists(path) && !QFile::remove(path))
                logWarning(tr("Could not remove the superseded file %1.").arg(path));
        }
        if (m_metadata)
            m_metadata->remove(item.replacesFileName);
    }
    if (!m_metadata) {
        return;
    }

	ModMetadataIndex::Entry e;
	e.fileName = item.fileName;
	e.platform = item.platform;
	e.projectId = item.projectId;
	e.versionId = item.versionId;
	e.name = item.name;
	/* Recorded rather than thrown away: the slug is how the same
	 * project is recognised across the two providers, and it is what
	 * a packwiz sidecar is named after. Clearing it here left every
	 * entry in the index unable to answer either question. */
	e.slug = item.slug;
	e.downloadUrl = item.downloadUrl;
	e.sha1 = item.sha1.toLower();
	e.fileSize = item.fileSize;
	e.isDependency = item.isDependency;
	e.installedAt = QDateTime::currentDateTimeUtc();
	m_metadata->put(e);
}

void ContentDownloadTask::onDownloadSucceeded()
{
	if (m_aborted) {
		return;
	}
	m_netJob.reset();
	emitSucceeded();
}

void ContentDownloadTask::onDownloadFailed(QString reason)
{
	if (m_aborted) {
		/* This is the aborted job unwinding, not a transfer that went
		 * wrong. The verdict has already been given. */
		return;
	}
	m_netJob.reset();
	emitFailed(reason);
}

void ContentDownloadTask::onDownloadProgress(qint64 current, qint64 total)
{
	if (m_aborted) {
		return;
	}
	setProgress(current, total);
}
