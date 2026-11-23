/* Copyright 2026 Project Tick
 *
 * This source is subject to the Microsoft Permissive License (MS-PL).
 * Please see the COPYING.md file for more information.
 */

#include "archive/ExportToZipTask.h"

#include <QDebug>
#include <QSaveFile>
#include <QBuffer>
#include <QTimer>
#include <quazipfile.h>
#include <JlCompress.h>
#include <QFileInfo>
#include <QThreadPool>
#include <QtConcurrent>

#include "FileSystem.h"
#include "MMCZip.h"

namespace
{
    bool addZipEntry(QuaZip& zip, const QString& name, QIODevice& source)
    {
        QuaZipFile entry(&zip);
        if (!entry.open(QIODevice::WriteOnly, QuaZipNewInfo(name)))
            return false;
        const bool copied = JlCompress::copyData(source, entry);
        entry.close();
        return copied && entry.getZipError() == ZIP_OK;
    }
}

namespace MMCZip
{
	ExportToZipTask::ExportToZipTask(QString outputPath, QDir dir,
									QFileInfoList files,
									QString destinationPrefix,
									bool followSymlinks, QObject* parent)
		: Task(parent), m_outputPath(std::move(outputPath)),
		  m_dir(std::move(dir)), m_files(std::move(files)),
		  m_destinationPrefix(std::move(destinationPrefix)),
		  m_followSymlinks(followSymlinks)
	{
	}

	ExportToZipTask::ExportToZipTask(QString outputPath, const QString& dir,
									QFileInfoList files,
									QString destinationPrefix,
									bool followSymlinks, QObject* parent)
		: ExportToZipTask(std::move(outputPath), QDir(dir), std::move(files),
						  std::move(destinationPrefix), followSymlinks, parent)
	{
	}

    ExportToZipTask::~ExportToZipTask()
    {
        m_cancelled.store(true);
        m_future.waitForFinished();
    }

	void ExportToZipTask::executeTask()
	{
		setStatus(tr("Adding files..."));
		setProgress(0, m_files.length() + m_extraFiles.size());

		m_future = QtConcurrent::run(QThreadPool::globalInstance(),
									 [this]() { return exportZip(); });
		connect(&m_watcher, &QFutureWatcher<ZipResult>::finished, this,
				&ExportToZipTask::finish);
		m_watcher.setFuture(m_future);
	}

	bool ExportToZipTask::abort()
	{
		if (m_future.isRunning()) {
			m_cancelled.store(true);
			/* No emitAborted() here: the worker is between files and has
			 * to be allowed to finish the one it is on and close the
			 * archive. finish() reports the abort when it really is
			 * one. */
			return true;
		}
		return false;
	}

	void ExportToZipTask::reportStatus(const QString& status)
	{
		QTimer::singleShot(0, this, [this, status] { setStatus(status); });
	}

	void ExportToZipTask::reportProgress(qint64 current, qint64 total)
	{
		QTimer::singleShot(0, this, [this, current, total] { setProgress(current, total); });
	}

	auto ExportToZipTask::exportZip() -> ZipResult
	{
		if (!m_dir.exists()) {
			return ZipResult(tr("The folder to export does not exist."));
		}

        QSaveFile output(m_outputPath);
        if (!output.open(QIODevice::WriteOnly))
            return ZipResult(tr("Could not create '%1': %2").arg(m_outputPath, output.errorString()));
        QuaZip zip(&output);
        zip.setAutoClose(false);
        if (!zip.open(QuaZip::mdCreate))
            return ZipResult(tr("Could not open the export archive."));

		qint64 done = 0;
		const qint64 total = m_files.length() + m_extraFiles.size();

		/* The generated entries first, so that a reader handed a
		 * truncated archive still finds the manifest that says what it
		 * was meant to be. */
		for (auto it = m_extraFiles.constBegin(); it != m_extraFiles.constEnd();
			 ++it) {
			if (m_cancelled.load())
				return ZipResult();
            QBuffer source;
            source.setData(it.value());
            source.open(QIODevice::ReadOnly);
            if (!addZipEntry(zip, it.key(), source))
                return ZipResult(tr("Could not add %1 to the archive.").arg(it.key()));
			reportProgress(++done, total);
		}

		for (const QFileInfo& file : m_files) {
			if (m_cancelled.load())
				return ZipResult();

			QString absolute = file.absoluteFilePath();
			const QString relative = m_dir.relativeFilePath(absolute);

			if (m_excludeFiles.contains(relative)) {
				reportProgress(++done, total);
				continue;
			}

			reportStatus(tr("Compressing: %1").arg(relative));

			if (m_followSymlinks) {
				/* An instance's mods folder is frequently a tree of
				 * links into a shared download folder; storing the link
				 * would export an archive full of dangling paths. */
				if (file.isSymLink()) {
					absolute = file.symLinkTarget();
				} else {
					const QString canonical = file.canonicalFilePath();
					if (!canonical.isEmpty()) {
						absolute = canonical;
					}
				}
			}

            QFile source(absolute);
            if (!source.open(QIODevice::ReadOnly) ||
                !addZipEntry(zip, m_destinationPrefix + relative, source))
                return ZipResult(tr("Could not read and compress %1.").arg(relative));
			reportProgress(++done, total);
		}

        zip.close();
        if (m_cancelled.load())
            return ZipResult();
        if (zip.getZipError() != ZIP_OK || !output.commit())
            return ZipResult(tr("Could not finish '%1'.").arg(m_outputPath));
		return ZipResult();
	}

	void ExportToZipTask::finish()
	{
		/* Nothing half-written is left behind for the user to find and
		 * mistake for a finished export - in either failure case. */
		if (m_cancelled.load()) {
			emitAborted();
			return;
		}

		const ZipResult result = m_future.result();
		if (result.has_value()) {
			emitFailed(result.value());
			return;
		}
		emitSucceeded();
	}
} // namespace MMCZip
