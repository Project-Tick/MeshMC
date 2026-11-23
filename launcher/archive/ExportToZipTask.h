/* Copyright 2026 Project Tick
 *
 * This source is subject to the Microsoft Permissive License (MS-PL).
 * Please see the COPYING.md file for more information.
 */

#pragma once

#include <QByteArray>
#include <QDir>
#include <QFileInfoList>
#include <QFuture>
#include <QFutureWatcher>
#include <QHash>
#include <QString>
#include <QStringList>

#include <atomic>
#include <nonstd/optional>

#include "tasks/Task.h"

namespace MMCZip
{
	/*
	 * Writing a list of files into a zip, off the UI thread.
	 *
	 * The export dialogs used to call MMCZip::compressDir() straight
	 * from the dialog's accept handler: the window froze for as long as
	 * the instance took to compress - minutes for a modpack with a
	 * world in it - with no progress and no way out. This is the same
	 * job as a task, so the progress dialog can show which file is being
	 * written and the user can change their mind.
	 *
	 * The file list is decided by the caller rather than walked here,
	 * because the callers already know it: they built the tree the user
	 * unchecked entries in, and a pack export additionally needs the
	 * same list to write its manifest. See
	 * MMCZip::collectFileListRecursively().
	 *
	 * `destinationPrefix` is prepended to every entry, which is how a
	 * pack export puts an instance's files under `overrides/`.
	 * `excludeFiles` names entries - relative to `dir` - to skip, which
	 * is how it then leaves out the mods the manifest says to download
	 * rather than shipping them twice. `extraFiles` are generated
	 * entries (the manifest itself) and are written before anything from
	 * disk, so a truncated archive still starts with something a reader
	 * can identify.
	 */
	class ExportToZipTask : public Task
	{
		Q_OBJECT
	  public:
		ExportToZipTask(QString outputPath, QDir dir, QFileInfoList files,
						QString destinationPrefix = QString(),
						bool followSymlinks = false, QObject* parent = nullptr);
		ExportToZipTask(QString outputPath, const QString& dir,
						QFileInfoList files,
						QString destinationPrefix = QString(),
						bool followSymlinks = false, QObject* parent = nullptr);
		~ExportToZipTask() override;

		void setExcludeFiles(QStringList excludeFiles)
		{
			m_excludeFiles = std::move(excludeFiles);
		}
		void addExtraFile(const QString& fileName, const QByteArray& data)
		{
			m_extraFiles.insert(fileName, data);
		}

		bool canAbort() const override
		{
			return true;
		}
		bool abort() override;

	  protected:
		void executeTask() override;

	  private:
		/* Empty on success, the reason on failure. */
		using ZipResult = nonstd::optional<QString>;

		ZipResult exportZip();
		void finish();

		/*
		 * setStatus() / setProgress(), as called from the worker thread.
		 *
		 * Those two write plain members - m_status is a QString - and
		 * ProgressDialog reads them straight off the task when it
		 * attaches to one that is already running, as does
		 * ConcurrentTask when it folds a subtask into its step list.
		 * Writing them from the thread doing the compressing while the
		 * UI thread reads them is a race over a refcounted string, not
		 * merely a stale number.
		 *
		 * So the report hops back to the thread this task lives on
		 * first, and only touches task state there. Same reasoning, and
		 * the same shape, as BackupTask::reportProgress().
		 */
		void reportStatus(const QString& status);
		void reportProgress(qint64 current, qint64 total);

		QString m_outputPath;
		QDir m_dir;
		QFileInfoList m_files;
		QString m_destinationPrefix;
		bool m_followSymlinks;
		QStringList m_excludeFiles;
		QHash<QString, QByteArray> m_extraFiles;

		/* Cancellation is tracked here rather than through
		 * QFuture::cancel(): the future QtConcurrent::run() hands back
		 * does not support being cancelled, so isCanceled() on it stays
		 * false forever and an abort would be silently ignored. */
		std::atomic<bool> m_cancelled{false};

		QFuture<ZipResult> m_future;
		QFutureWatcher<ZipResult> m_watcher;
	};
} // namespace MMCZip
