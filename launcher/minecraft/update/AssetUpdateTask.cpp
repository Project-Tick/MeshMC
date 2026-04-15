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
 */

#include "AssetUpdateTask.h"

#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"
#include "net/ChecksumValidator.h"
#include "minecraft/AssetsUtils.h"

#include "Application.h"

AssetUpdateTask::AssetUpdateTask(MinecraftInstance* inst)
{
	m_inst = inst;
}

AssetUpdateTask::~AssetUpdateTask() {}

void AssetUpdateTask::executeTask()
{
	setStatus(tr("Updating assets index..."));
	auto components = m_inst->getPackProfile();
	auto profile = components->getProfile();
	auto assets = profile->getMinecraftAssets();
	QUrl indexUrl = assets->url;
	QString localPath = assets->id + ".json";
	auto job = new NetJob(tr("Asset index for %1").arg(m_inst->name()),
						  APPLICATION->network());

	auto metacache = APPLICATION->metacache();
	auto entry = metacache->resolveEntry("asset_indexes", localPath);
	entry->setStale(true);
	auto hexSha1 = assets->sha1.toLatin1();
	qDebug() << "Asset index SHA1:" << hexSha1;
	auto dl = Net::Download::makeCached(indexUrl, entry);
	auto rawSha1 = QByteArray::fromHex(assets->sha1.toLatin1());
	dl->addValidator(
		new Net::ChecksumValidator(QCryptographicHash::Sha1, rawSha1));
	job->addNetAction(dl);

	downloadJob.reset(job);

	connect(downloadJob.get(), &NetJob::succeeded, this,
			&AssetUpdateTask::assetIndexFinished);
	connect(downloadJob.get(), &NetJob::failed, this,
			&AssetUpdateTask::assetIndexFailed);
	connect(downloadJob.get(), &NetJob::progress, this,
			&AssetUpdateTask::progress);

	qDebug() << m_inst->name() << ": Starting asset index download";
	downloadJob->start();
}

bool AssetUpdateTask::canAbort() const
{
	return true;
}

void AssetUpdateTask::assetIndexFinished()
{
	AssetsIndex index;
	qDebug() << m_inst->name() << ": Finished asset index download";

	auto components = m_inst->getPackProfile();
	auto profile = components->getProfile();
	auto assets = profile->getMinecraftAssets();

	QString asset_fname = "assets/indexes/" + assets->id + ".json";
	// FIXME: this looks like a job for a generic validator based on json
	// schema?
	if (!AssetsUtils::loadAssetsIndexJson(assets->id, asset_fname, index)) {
		auto metacache = APPLICATION->metacache();
		auto entry =
			metacache->resolveEntry("asset_indexes", assets->id + ".json");
		metacache->evictEntry(entry);
		emitFailed(tr("Failed to read the assets index!"));
	}

	auto job = index.getDownloadJob();
	if (job) {
		setStatus(tr("Getting the assets files from Mojang..."));
		downloadJob = job;
		connect(downloadJob.get(), &NetJob::succeeded, this,
				&AssetUpdateTask::emitSucceeded);
		connect(downloadJob.get(), &NetJob::failed, this,
				&AssetUpdateTask::assetsFailed);
		connect(downloadJob.get(), &NetJob::progress, this,
				&AssetUpdateTask::progress);
		downloadJob->start();
		return;
	}
	emitSucceeded();
}

void AssetUpdateTask::assetIndexFailed(QString reason)
{
	qDebug() << m_inst->name() << ": Failed asset index download";
	emitFailed(tr("Failed to download the assets index:\n%1").arg(reason));
}

void AssetUpdateTask::assetsFailed(QString reason)
{
	emitFailed(tr("Failed to download assets:\n%1").arg(reason));
}

bool AssetUpdateTask::abort()
{
	if (downloadJob) {
		return downloadJob->abort();
	} else {
		qWarning() << "Prematurely aborted AssetUpdateTask";
	}
	return true;
}
