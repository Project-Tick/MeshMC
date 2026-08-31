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
	void writeSidecars();

	QList<ModPlatform::DownloadItem> m_items;
	QString m_targetDir;
	NetJob::Ptr m_netJob;
	std::shared_ptr<ModMetadataIndex> m_metadata;

	/* Latched by abort(). Aborting the job makes it report failed() as
	 * it unwinds, and that must not turn into a second verdict on top of
	 * the aborted one. */
	bool m_aborted = false;
};
