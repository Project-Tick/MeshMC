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
 *
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
