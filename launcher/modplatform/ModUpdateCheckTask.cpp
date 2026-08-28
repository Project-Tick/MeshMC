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

#include "ModUpdateCheckTask.h"

#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>

#include "Application.h"
#include "Json.h"
#include "minecraft/mod/ModMetadataIndex.h"
#include "modplatform/ContentType.h"
#include "net/Download.h"
#include "net/NetJob.h"

ModUpdateCheckTask::ModUpdateCheckTask(std::shared_ptr<ModMetadataIndex> index,
									   QString mcVersion, QString loader,
									   QObject* parent)
	: Task(parent), m_index(std::move(index)),
	  m_mcVersion(std::move(mcVersion)), m_loader(std::move(loader))
{
}

namespace
{
	/* Build the per-platform "latest matching version" endpoint URL. */
	QString buildQueryUrl(const ModMetadataIndex::Entry& e,
						  const QString& mcVersion, const QString& loader)
	{
		if (e.platform == QStringLiteral("modrinth")) {
			QString url =
				QStringLiteral("https://api.modrinth.com/v2/project/%1/"
							   "version?game_versions=[\"%2\"]")
					.arg(e.projectId, mcVersion);
			if (!loader.isEmpty()) {
				url += QStringLiteral("&loaders=[\"%1\"]").arg(loader);
			}
			return url;
		}
		if (e.platform == QStringLiteral("curseforge")) {
			QString url =
				QStringLiteral("https://api.curseforge.com/v1/mods/%1/"
							   "files?gameVersion=%2")
					.arg(e.projectId, mcVersion);
			if (!loader.isEmpty()) {
				const int t =
					ModPlatform::loaderToCurseForgeModLoaderType(loader);
				if (t > 0) {
					url += QStringLiteral("&modLoaderType=%1").arg(t);
				}
			}
			return url;
		}
		return {};
	}

	bool buildModrinthUpdate(const ModMetadataIndex::Entry& entry,
							 const QByteArray& body,
							 ModUpdateCheckTask::UpdateInfo& out)
	{
		QJsonDocument doc = QJsonDocument::fromJson(body);
		if (!doc.isArray() || doc.array().isEmpty()) {
			return false;
		}
		const auto vObj = doc.array().first().toObject();
		const QString newVer = Json::ensureString(vObj, "id", "");
		if (newVer.isEmpty() || newVer == entry.versionId) {
			return false;
		}

		ModPlatform::DownloadItem item;
		item.name = entry.name;
		item.platform = entry.platform;
		item.projectId = entry.projectId;
		item.versionId = newVer;
		item.isDependency = entry.isDependency;
		item.replaceExisting = true;
		item.replacesFileName = entry.fileName;

		const auto files = Json::ensureArray(vObj, "files");
		for (auto fileRaw : files) {
			const auto fObj = fileRaw.toObject();
			const bool primary = Json::ensureBoolean(fObj, "primary", false);
			if (primary || files.size() == 1) {
				item.downloadUrl = Json::ensureString(fObj, "url", "");
				item.fileName = Json::ensureString(fObj, "filename", "");
				item.fileSize = Json::ensureInteger(fObj, "size", 0);
				const auto hashes = Json::ensureObject(fObj, "hashes");
				item.sha1 = Json::ensureString(hashes, "sha1", "");
				break;
			}
		}
		if (item.downloadUrl.isEmpty() || item.fileName.isEmpty()) {
			return false;
		}

		out.currentFileName = entry.fileName;
		out.currentVersionId = entry.versionId;
		out.newVersionId = newVer;
		out.name = entry.name;
		out.platform = entry.platform;
		out.item = item;
		return true;
	}

	bool buildCurseForgeUpdate(const ModMetadataIndex::Entry& entry,
							   const QByteArray& body,
							   ModUpdateCheckTask::UpdateInfo& out)
	{
		QJsonDocument doc = QJsonDocument::fromJson(body);
		QJsonArray arr;
		if (doc.isObject() && doc.object().contains("data")) {
			const auto v = doc.object().value("data");
			if (v.isArray()) {
				arr = v.toArray();
			}
		}
		if (arr.isEmpty()) {
			return false;
		}
		const auto fObj = arr.first().toObject();
		const QString newVer =
			QString::number(Json::ensureInteger(fObj, "id", 0));
		if (newVer.isEmpty() || newVer == QStringLiteral("0") ||
			newVer == entry.versionId) {
			return false;
		}

		ModPlatform::DownloadItem item;
		item.name = Json::ensureString(fObj, "displayName", entry.name);
		item.platform = entry.platform;
		item.projectId = entry.projectId;
		item.versionId = newVer;
		item.fileName = Json::ensureString(fObj, "fileName", "");
		item.downloadUrl = Json::ensureString(fObj, "downloadUrl", "");
		item.fileSize = Json::ensureInteger(fObj, "fileLength", 0);
		item.sha1 = ModPlatform::curseForgeSha1FromFileObject(fObj);
		item.isDependency = entry.isDependency;
		item.replaceExisting = true;
		item.replacesFileName = entry.fileName;

		if (item.downloadUrl.isEmpty()) {
			const int fileId = Json::ensureInteger(fObj, "id", 0);
			if (fileId > 0 && !item.fileName.isEmpty()) {
				item.downloadUrl =
					QStringLiteral("https://www.curseforge.com/api/v1/mods/"
								   "%1/files/%2/download")
						.arg(entry.projectId)
						.arg(fileId);
			}
		}
		if (item.downloadUrl.isEmpty() || item.fileName.isEmpty()) {
			return false;
		}

		out.currentFileName = entry.fileName;
		out.currentVersionId = entry.versionId;
		out.newVersionId = newVer;
		out.name = entry.name;
		out.platform = entry.platform;
		out.item = item;
		return true;
	}
} // namespace

void ModUpdateCheckTask::executeTask()
{
	if (!m_index) {
		emitSucceeded();
		return;
	}

	const auto entries = m_index->all();
	for (const auto& e : entries) {
		if (!e.hasPlatformOrigin()) {
			continue;
		}
		if (e.platform != QStringLiteral("modrinth") &&
			e.platform != QStringLiteral("curseforge")) {
			continue;
		}
		m_total++;
	}

	if (m_total == 0) {
		setStatus(tr("No tracked mods to check."));
		emitSucceeded();
		return;
	}

	setStatus(tr("Checking %1 mod(s) for updates...").arg(m_total));

	for (const auto& e : entries) {
		if (!e.hasPlatformOrigin()) {
			continue;
		}
		if (e.platform != QStringLiteral("modrinth") &&
			e.platform != QStringLiteral("curseforge")) {
			continue;
		}

		const QString url = buildQueryUrl(e, m_mcVersion, m_loader);
		if (url.isEmpty()) {
			continue;
		}

		auto* response = new QByteArray();
		NetJob* job = new NetJob(
			QString("UpdateCheck(%1:%2)").arg(e.platform, e.projectId),
			APPLICATION->network());
		job->addNetAction(Net::Download::makeByteArray(QUrl(url), response));

		const ModMetadataIndex::Entry entry = e;
		m_pending++;

		connect(job, &NetJob::succeeded, this, [this, entry, response, job]() {
			job->deleteLater();
			UpdateInfo u;
			bool found = false;
			if (entry.platform == QStringLiteral("modrinth")) {
				found = buildModrinthUpdate(entry, *response, u);
			} else if (entry.platform == QStringLiteral("curseforge")) {
				found = buildCurseForgeUpdate(entry, *response, u);
			}
			if (found) {
				m_updates.append(u);
			}
			delete response;
			onOneDone();
		});
		connect(job, &NetJob::failed, this,
				[this, response, job, entry](QString reason) {
					qWarning() << "Update check failed for" << entry.name << ":"
							   << reason;
					job->deleteLater();
					delete response;
					onOneDone();
				});
		// Show the check as its own line in the progress dialog.
		propagateStepsFrom(job);
		job->start();
	}
}

void ModUpdateCheckTask::onOneDone()
{
	m_completed++;
	m_pending--;
	setProgress(m_completed, m_total);
	if (m_pending <= 0) {
		qDebug() << "ModUpdateCheckTask:" << m_updates.size()
				 << "update(s) found across" << m_total << "mod(s)";
		emitSucceeded();
	}
}
