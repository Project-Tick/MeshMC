/* Copyright 2026 Project Tick
 *
 * This source is subject to the Microsoft Permissive License (MS-PL).
 * Please see the COPYING.md file for more information.
 */

#pragma once

#include "tasks/Task.h"
#include "net/NetJob.h"
#include "java/download/JavaRuntime.h"

#include <QUrl>
#include <QTemporaryDir>
#include <QFutureWatcher>

class JavaDownloadTask : public Task
{
	Q_OBJECT

  public:
	explicit JavaDownloadTask(const JavaDownload::RuntimeEntry& runtime,
							  const QString& targetDir,
							  QObject* parent = nullptr);
	~JavaDownloadTask() override;
    bool canAbort() const override { return m_downloadJob && m_downloadJob->isRunning(); }
    bool abort() override { return canAbort() && m_downloadJob->abort(); }

	QString installedJavaPath() const
	{
		return m_installedJavaPath;
	}

  protected:
	void executeTask() override;

  private slots:
	void downloadFinished();
	void downloadFailed(QString reason);
	void extractArchive();
	void manifestDownloaded();
	void manifestFilesDownloaded();

  private:
	void installStagedRuntime();
	void downloadArchive();
	void downloadManifest();
	QString findJavaBinary(const QString& dir) const;

	JavaDownload::RuntimeEntry m_runtime;
    QString m_destination;
    QString m_targetDir;
    std::unique_ptr<QTemporaryDir> m_staging;
    QFutureWatcher<QString> m_extraction;
	QString m_archivePath;
	QString m_installedJavaPath;
	NetJob::Ptr m_downloadJob;
	QByteArray m_manifestData;
	QStringList m_executableFiles;
	QList<QPair<QString, QString>> m_linkEntries;
};
