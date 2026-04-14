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
 *
 */

#include "ContentDownloadTask.h"
#include "Application.h"
#include "net/Download.h"
#include "net/ChecksumValidator.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QCryptographicHash>

ContentDownloadTask::ContentDownloadTask(
	const QList<ModPlatform::DownloadItem>& items, const QString& targetDir,
	QObject* parent)
	: Task(parent), m_items(items), m_targetDir(targetDir)
{
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

	setStatus(tr("Downloading %1 file(s)...").arg(m_items.size()));

	m_netJob = new NetJob("ContentDownload", APPLICATION->network());

	int skipped = 0;
	for (const auto& item : m_items) {
		QString targetPath = dir.filePath(item.fileName);

		// Skip if file already exists with matching SHA1 checksum
		if (QFileInfo::exists(targetPath) && !item.sha1.isEmpty()) {
			QFile existingFile(targetPath);
			if (existingFile.open(QIODevice::ReadOnly)) {
				QCryptographicHash hash(QCryptographicHash::Sha1);
				hash.addData(&existingFile);
				existingFile.close();
				if (hash.result().toHex() == item.sha1.toLatin1().toLower()) {
					qDebug() << "Skipping" << item.fileName
							 << "- already exists with matching SHA1";
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

	m_netJob->start();
}

void ContentDownloadTask::onDownloadSucceeded()
{
	m_netJob.reset();
	emitSucceeded();
}

void ContentDownloadTask::onDownloadFailed(QString reason)
{
	m_netJob.reset();
	emitFailed(reason);
}

void ContentDownloadTask::onDownloadProgress(qint64 current, qint64 total)
{
	setProgress(current, total);
}
