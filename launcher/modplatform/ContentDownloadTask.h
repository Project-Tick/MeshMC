/* Copyright 2026 Project Tick
 *
 * This source is subject to the Microsoft Permissive License (MS-PL).
 * Please see the COPYING.md file for more information.
 */

#pragma once

#include <QObject>
#include <QList>
#include <QString>
#include <memory>

#include "modplatform/ModDownloadTypes.h"
#include "net/NetJob.h"
#include "tasks/Task.h"

class ModMetadataIndex;

class ContentDownloadTask : public Task
{
	Q_OBJECT

  public:
	explicit ContentDownloadTask(const QList<ModPlatform::DownloadItem>& items,
								 const QString& targetDir,
								 QObject* parent = nullptr);

	/* When provided, the downloader writes a provenance sidecar for every
	 * file it places on disk and removes the sidecar for any file it
	 * supersedes (`DownloadItem::replacesFileName`). The pointer is held
	 * by shared ownership so callers can hand off the model-owned index
	 * without lifetime concerns. */
	void setMetadataIndex(std::shared_ptr<ModMetadataIndex> index);

	/* The progress dialog carries a Skip button, so this has to be able
	 * to give up part way through. What is already on disk stays there:
	 * the folder model rescans afterwards either way. */
	bool canAbort() const override
	{
		return true;
	}

	/* Whether this stopped because the user said so, as opposed to a
	 * transfer going wrong. Task itself does not tell the two apart -
	 * emitAborted() sets the fail reason to "Aborted." - and the caller
	 * has to, since there is nothing to complain about when the answer
	 * is yes. */
	bool wasAborted() const
	{
		return m_aborted;
	}

  public slots:
	bool abort() override;

  protected:
	void executeTask() override;

  private slots:
	void onDownloadSucceeded();
	void onDownloadFailed(QString reason);
	void onDownloadProgress(qint64 current, qint64 total);

  private:
	void finishItem(const ModPlatform::DownloadItem& item);

	QList<ModPlatform::DownloadItem> m_items;
	QString m_targetDir;
	NetJob::Ptr m_netJob;
	std::shared_ptr<ModMetadataIndex> m_metadata;

	/* Latched by abort(). Aborting the job makes it report failed() as
	 * it unwinds, and that must not turn into a second verdict on top of
	 * the aborted one. */
	bool m_aborted = false;
};
