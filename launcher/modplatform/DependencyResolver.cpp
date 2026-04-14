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

#include "DependencyResolver.h"
#include "Application.h"
#include "Json.h"
#include "modplatform/ContentType.h"
#include "net/Download.h"
#include "net/NetJob.h"

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QRegularExpression>
#include <QUrl>

DependencyResolver::DependencyResolver(
	const QList<ModPlatform::SelectedMod>& selectedMods,
	const QString& mcVersion, const QString& loader, QObject* parent)
	: Task(parent), m_selectedMods(selectedMods), m_mcVersion(mcVersion),
	  m_loader(loader)
{
	// pre-populate resolved set so we don't fetch deps for things we're already
	// installing
	for (const auto& mod : m_selectedMods) {
		m_resolvedProjectIds.insert(mod.platform + ":" + mod.projectId);
		m_resolvedNames.insert(normalizeName(mod.name));
	}
}

QString DependencyResolver::normalizeName(const QString& name)
{
	// Lowercase, trim, remove common suffixes like " (Fabric)", remove
	// non-alphanumeric
	QString n = name.toLower().trimmed();
	// Remove parenthesized loader/version suffixes: "(Fabric)", "(Forge)", etc.
	n.remove(QRegularExpression("\\s*\\([^)]*\\)\\s*"));
	// Keep only alphanumeric and spaces, then collapse spaces
	n.remove(QRegularExpression("[^a-z0-9 ]"));
	n = n.simplified();
	return n;
}

void DependencyResolver::executeTask()
{
	if (m_selectedMods.isEmpty()) {
		emitSucceeded();
		return;
	}
	setStatus(tr("Resolving dependencies..."));
	m_currentModIndex = 0;
	resolveNextMod();
}

void DependencyResolver::resolveNextMod()
{
	if (m_currentModIndex >= m_selectedMods.size()) {
		checkCompletion();
		return;
	}

	const auto& mod = m_selectedMods[m_currentModIndex];
	if (mod.platform == "curseforge") {
		resolveCurseForgeDependencies(mod);
	} else if (mod.platform == "modrinth") {
		resolveModrinthDependencies(mod);
	} else {
		m_currentModIndex++;
		resolveNextMod();
	}
}

void DependencyResolver::resolveCurseForgeDependencies(
	const ModPlatform::SelectedMod& mod)
{
	if (mod.versionId.isEmpty()) {
		m_currentModIndex++;
		resolveNextMod();
		return;
	}

	auto* response = new QByteArray();
	NetJob* job = new NetJob(QString("CF::DepResolve(%1)").arg(mod.name),
							 APPLICATION->network());
	auto url = QString("https://api.curseforge.com/v1/mods/%1/files/%2")
				   .arg(mod.projectId, mod.versionId);
	job->addNetAction(Net::Download::makeByteArray(QUrl(url), response));

	m_pendingRequests++;
	auto currentMod = mod;
	connect(job, &NetJob::succeeded, this, [this, currentMod, response, job] {
		job->deleteLater();
		onCurseForgeVersionResolved(currentMod, *response);
		delete response;
		m_pendingRequests--;
		m_currentModIndex++;
		resolveNextMod();
	});
	connect(job, &NetJob::failed, this, [this, response, job](QString reason) {
		job->deleteLater();
		delete response;
		qWarning() << "Failed to resolve CurseForge dependencies:" << reason;
		m_pendingRequests--;
		m_currentModIndex++;
		resolveNextMod();
	});
	job->start();
}

void DependencyResolver::resolveModrinthDependencies(
	const ModPlatform::SelectedMod& mod)
{
	if (mod.versionId.isEmpty()) {
		m_currentModIndex++;
		resolveNextMod();
		return;
	}

	auto* response = new QByteArray();
	NetJob* job = new NetJob(QString("MR::DepResolve(%1)").arg(mod.name),
							 APPLICATION->network());
	auto url =
		QString("https://api.modrinth.com/v2/version/%1").arg(mod.versionId);
	job->addNetAction(Net::Download::makeByteArray(QUrl(url), response));

	m_pendingRequests++;
	auto currentMod = mod;
	connect(job, &NetJob::succeeded, this, [this, currentMod, response, job] {
		job->deleteLater();
		onModrinthVersionResolved(currentMod, *response);
		delete response;
		m_pendingRequests--;
		checkCompletion();
	});
	connect(job, &NetJob::failed, this, [this, response, job](QString reason) {
		job->deleteLater();
		delete response;
		qWarning() << "Failed to resolve Modrinth dependencies:" << reason;
		m_pendingRequests--;
		checkCompletion();
	});
	job->start();
	m_currentModIndex++;
	resolveNextMod();
}

void DependencyResolver::onCurseForgeVersionResolved(
	const ModPlatform::SelectedMod& mod, const QByteArray& data)
{
	QJsonParseError parseError;
	QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
	if (parseError.error != QJsonParseError::NoError)
		return;

	QJsonObject fileObj;
	if (doc.isObject() && doc.object().contains("data")) {
		fileObj = doc.object().value("data").toObject();
	} else {
		fileObj = doc.object();
	}

	processCFFileDeps(fileObj);
}

void DependencyResolver::onModrinthVersionResolved(
	const ModPlatform::SelectedMod& mod, const QByteArray& data)
{
	QJsonParseError parseError;
	QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
	if (parseError.error != QJsonParseError::NoError)
		return;

	processMRVersionDeps(doc.object());
}

void DependencyResolver::onDependencyProjectResolved(const QString& platform,
													 const QString& projectId,
													 const QByteArray& data)
{
	QJsonParseError parseError;
	QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
	if (parseError.error != QJsonParseError::NoError)
		return;

	if (platform == "curseforge") {
		// Response is from /v1/mods/{id}/files endpoint
		QJsonArray filesArr;
		if (doc.isObject() && doc.object().contains("data")) {
			auto dataVal = doc.object().value("data");
			if (dataVal.isArray()) {
				filesArr = dataVal.toArray();
			} else if (dataVal.isObject()) {
				filesArr.append(dataVal);
			}
		}

		if (filesArr.isEmpty()) {
			qWarning() << "No compatible files found for CurseForge dependency"
					   << projectId << "- attempting cross resolve to Modrinth";
			crossResolveFromCurseForge(projectId);
			return;
		}

		ModPlatform::DependencyInfo dep;
		dep.projectId = projectId;
		dep.platform = "curseforge";
		dep.isRequired = true;

		// Pick the first file (already filtered by gameVersion via API)
		auto fileObj = filesArr.first().toObject();
		dep.versionId = QString::number(Json::ensureInteger(fileObj, "id", 0));
		dep.fileName = Json::ensureString(fileObj, "fileName", "");
		dep.downloadUrl = Json::ensureString(fileObj, "downloadUrl", "");
		dep.fileSize = Json::ensureInteger(fileObj, "fileLength", 0);
		dep.name = Json::ensureString(fileObj, "displayName", dep.fileName);

		// Handle restricted downloads (downloadUrl is null)
		if (dep.downloadUrl.isEmpty()) {
			int fileId = Json::ensureInteger(fileObj, "id", 0);
			if (fileId > 0 && !dep.fileName.isEmpty()) {
				dep.downloadUrl = QString("https://www.curseforge.com/api/v1/"
										  "mods/%1/files/%2/download")
									  .arg(projectId)
									  .arg(fileId);
				qWarning() << "CurseForge dependency" << projectId
						   << "has restricted download, using fallback URL";
			}
		}

		if (!dep.downloadUrl.isEmpty()) {
			qDebug() << "Resolved CurseForge dependency:" << dep.name
					 << "file:" << dep.fileName << "url:" << dep.downloadUrl;
			m_resolvedNames.insert(normalizeName(dep.name));
			m_dependencies.append(dep);
			processCFFileDeps(fileObj); // recurse into sub-deps
		} else {
			qWarning() << "CurseForge dependency" << projectId
					   << "has no download URL, skipping";
		}
	} else if (platform == "modrinth") {
		// data is an array of versions
		QJsonArray arr;
		if (doc.isArray()) {
			arr = doc.array();
		} else {
			return;
		}

		if (arr.isEmpty()) {
			qWarning() << "No compatible files found for Modrinth dependency"
					   << projectId
					   << "- attempting cross resolve to CurseForge";
			crossResolveFromModrinth(projectId);
			return;
		}

		// Pick the first (latest compatible) version
		auto vObj = arr.first().toObject();
		ModPlatform::DependencyInfo dep;
		dep.projectId = projectId;
		dep.versionId = Json::ensureString(vObj, "id", "");
		dep.name = Json::ensureString(vObj, "name", projectId);
		dep.platform = "modrinth";
		dep.isRequired = true;

		auto files = Json::ensureArray(vObj, "files");
		for (auto fileRaw : files) {
			auto fileObj = fileRaw.toObject();
			bool primary = Json::ensureBoolean(fileObj, "primary", false);
			if (primary || files.size() == 1) {
				dep.downloadUrl = Json::ensureString(fileObj, "url", "");
				dep.fileName = Json::ensureString(fileObj, "filename", "");
				dep.fileSize = Json::ensureInteger(fileObj, "size", 0);
				auto hashes = Json::ensureObject(fileObj, "hashes");
				dep.sha1 = Json::ensureString(hashes, "sha1", "");
				break;
			}
		}

		if (!dep.downloadUrl.isEmpty()) {
			m_resolvedNames.insert(normalizeName(dep.name));
			m_dependencies.append(dep);
			processMRVersionDeps(vObj); // recurse into sub-deps
		}
	}
}

void DependencyResolver::processCFFileDeps(const QJsonObject& fileObj)
{
	auto deps = Json::ensureArray(fileObj, "dependencies");
	for (auto depRaw : deps) {
		auto depObj = depRaw.toObject();
		int relationType = Json::ensureInteger(depObj, "relationType", 0);

		int modId = Json::ensureInteger(depObj, "modId", 0);
		if (modId == 0)
			continue;

		QString depProjectId = QString::number(modId);
		QString key = "curseforge:" + depProjectId;

		// Track embedded/included deps so they won't be re-downloaded from
		// other mods
		if (relationType == 1 ||
			relationType == 6) { // 1=EmbeddedLibrary, 6=Include
			m_resolvedProjectIds.insert(key);
			continue;
		}

		if (relationType != 3)
			continue; // 3 = required

		if (m_resolvedProjectIds.contains(key))
			continue;
		m_resolvedProjectIds.insert(key);

		auto* depResponse = new QByteArray();
		NetJob* depJob =
			new NetJob(QString("CF::DepFetch(%1)").arg(depProjectId),
					   APPLICATION->network());
		QString url =
			QString(
				"https://api.curseforge.com/v1/mods/%1/files?gameVersion=%2")
				.arg(depProjectId, m_mcVersion);
		if (!m_loader.isEmpty()) {
			url += QString("&modLoaderType=%1")
					   .arg(ModPlatform::loaderToCurseForgeModLoaderType(
						   m_loader));
		}
		qDebug() << "Fetching CurseForge dependency files (recursive):" << url;
		depJob->addNetAction(
			Net::Download::makeByteArray(QUrl(url), depResponse));

		m_pendingRequests++;
		connect(depJob, &NetJob::succeeded, this,
				[this, depProjectId, depResponse, depJob] {
					depJob->deleteLater();
					onDependencyProjectResolved("curseforge", depProjectId,
												*depResponse);
					delete depResponse;
					m_pendingRequests--;
					checkCompletion();
				});
		connect(depJob, &NetJob::failed, this,
				[this, depResponse, depJob](QString) {
					depJob->deleteLater();
					delete depResponse;
					m_pendingRequests--;
					checkCompletion();
				});
		depJob->start();
	}
}

void DependencyResolver::processMRVersionDeps(const QJsonObject& versionObj)
{
	auto deps = Json::ensureArray(versionObj, "dependencies");
	for (auto depRaw : deps) {
		auto depObj = depRaw.toObject();
		QString depType = Json::ensureString(depObj, "dependency_type", "");

		QString depProjectId = Json::ensureString(depObj, "project_id", "");
		QString depVersionId = Json::ensureString(depObj, "version_id", "");
		if (depProjectId.isEmpty())
			continue;

		QString key = "modrinth:" + depProjectId;

		// Track embedded deps so they won't be re-downloaded from other mods
		if (depType == "embedded") {
			m_resolvedProjectIds.insert(key);
			continue;
		}

		if (depType != "required")
			continue;

		if (m_resolvedProjectIds.contains(key))
			continue;
		m_resolvedProjectIds.insert(key);

		if (!depVersionId.isEmpty()) {
			auto* depResponse = new QByteArray();
			NetJob* depJob =
				new NetJob(QString("MR::DepVersion(%1)").arg(depProjectId),
						   APPLICATION->network());
			const auto url = QString("https://api.modrinth.com/v2/version/%1")
								 .arg(depVersionId);
			depJob->addNetAction(
				Net::Download::makeByteArray(QUrl(url), depResponse));

			m_pendingRequests++;
			connect(
				depJob, &NetJob::succeeded, this,
				[this, depProjectId, depVersionId, depResponse, depJob] {
					depJob->deleteLater();

					QJsonDocument vDoc = QJsonDocument::fromJson(*depResponse);
					auto vObj = vDoc.object();

					// Check game version compatibility before accepting this
					// version
					auto gameVersions =
						Json::ensureArray(vObj, "game_versions");
					bool compatible = false;
					for (auto v : gameVersions) {
						if (v.toString() == m_mcVersion) {
							compatible = true;
							break;
						}
					}

					if (!compatible) {
						qWarning()
							<< "Modrinth dep version" << depVersionId
							<< "is not compatible with MC" << m_mcVersion
							<< "- falling back to project version search";
						delete depResponse;

						// Fall back to project-level search with game version
						// filter
						auto* fbResponse = new QByteArray();
						NetJob* fbJob = new NetJob(
							QString("MR::DepProject(%1)").arg(depProjectId),
							APPLICATION->network());
						auto fbUrl =
							QString("https://api.modrinth.com/v2/project/%1/"
									"version?game_versions=[\"%2\"]")
								.arg(depProjectId, m_mcVersion);
						if (!m_loader.isEmpty()) {
							fbUrl += QString("&loaders=[\"%1\"]").arg(m_loader);
						}
						fbJob->addNetAction(Net::Download::makeByteArray(
							QUrl(fbUrl), fbResponse));

						// Reuse pending count - don't decrement for current,
						// don't increment for fallback
						connect(fbJob, &NetJob::succeeded, this,
								[this, depProjectId, fbResponse, fbJob] {
									fbJob->deleteLater();
									onDependencyProjectResolved(
										"modrinth", depProjectId, *fbResponse);
									delete fbResponse;
									m_pendingRequests--;
									checkCompletion();
								});
						connect(fbJob, &NetJob::failed, this,
								[this, fbResponse, fbJob](QString) {
									fbJob->deleteLater();
									delete fbResponse;
									m_pendingRequests--;
									checkCompletion();
								});
						fbJob->start();
						return;
					}

					ModPlatform::DependencyInfo dep;
					dep.projectId = depProjectId;
					dep.versionId = Json::ensureString(vObj, "id", "");
					dep.name = Json::ensureString(vObj, "name", depProjectId);
					dep.platform = "modrinth";
					dep.isRequired = true;

					auto files = Json::ensureArray(vObj, "files");
					for (auto fileRaw : files) {
						auto fileObj = fileRaw.toObject();
						bool primary =
							Json::ensureBoolean(fileObj, "primary", false);
						if (primary || files.size() == 1) {
							dep.downloadUrl =
								Json::ensureString(fileObj, "url", "");
							dep.fileName =
								Json::ensureString(fileObj, "filename", "");
							dep.fileSize =
								Json::ensureInteger(fileObj, "size", 0);
							auto hashes = Json::ensureObject(fileObj, "hashes");
							dep.sha1 = Json::ensureString(hashes, "sha1", "");
							break;
						}
					}

					if (!dep.downloadUrl.isEmpty()) {
						m_resolvedNames.insert(normalizeName(dep.name));
						m_dependencies.append(dep);
						processMRVersionDeps(vObj); // recurse into sub-deps
					}

					delete depResponse;
					m_pendingRequests--;
					checkCompletion();
				});
			connect(depJob, &NetJob::failed, this,
					[this, depResponse, depJob](QString) {
						depJob->deleteLater();
						delete depResponse;
						m_pendingRequests--;
						checkCompletion();
					});
			depJob->start();
		} else {
			auto* depResponse = new QByteArray();
			NetJob* depJob =
				new NetJob(QString("MR::DepProject(%1)").arg(depProjectId),
						   APPLICATION->network());
			auto url = QString("https://api.modrinth.com/v2/project/%1/"
							   "version?game_versions=[\"%2\"]")
						   .arg(depProjectId, m_mcVersion);
			if (!m_loader.isEmpty()) {
				url += QString("&loaders=[\"%1\"]").arg(m_loader);
			}
			depJob->addNetAction(
				Net::Download::makeByteArray(QUrl(url), depResponse));

			m_pendingRequests++;
			connect(depJob, &NetJob::succeeded, this,
					[this, depProjectId, depResponse, depJob] {
						depJob->deleteLater();
						onDependencyProjectResolved("modrinth", depProjectId,
													*depResponse);
						delete depResponse;
						m_pendingRequests--;
						checkCompletion();
					});
			connect(depJob, &NetJob::failed, this,
					[this, depResponse, depJob](QString) {
						depJob->deleteLater();
						delete depResponse;
						m_pendingRequests--;
						checkCompletion();
					});
			depJob->start();
		}
	}
}

void DependencyResolver::crossResolveFromCurseForge(const QString& projectId)
{
	auto* response = new QByteArray();
	NetJob* job = new NetJob(QString("CF::FetchName(%1)").arg(projectId),
							 APPLICATION->network());
	job->addNetAction(Net::Download::makeByteArray(
		QUrl(QString("https://api.curseforge.com/v1/mods/%1").arg(projectId)),
		response));

	m_pendingRequests++;
	connect(job, &NetJob::succeeded, this, [this, projectId, response, job]() {
		job->deleteLater();
		QJsonDocument doc = QJsonDocument::fromJson(*response);
		delete response;

		QString name;
		QString slug;
		if (doc.isObject() && doc.object().contains("data")) {
			auto dataObj = doc.object().value("data").toObject();
			name = Json::ensureString(dataObj, "name", "");
			slug = Json::ensureString(dataObj, "slug", "");
		}
		if (!name.isEmpty()) {
			qDebug() << "CF cross resolve extracted name:" << name
					 << "slug:" << slug << "for" << projectId;
			executeCrossResolve("modrinth", name, slug);
		}
		m_pendingRequests--;
		checkCompletion();
	});
	connect(job, &NetJob::failed, this, [this, response, job](QString) {
		job->deleteLater();
		delete response;
		m_pendingRequests--;
		checkCompletion();
	});
	job->start();
}

void DependencyResolver::crossResolveFromModrinth(const QString& projectId)
{
	auto* response = new QByteArray();
	NetJob* job = new NetJob(QString("MR::FetchName(%1)").arg(projectId),
							 APPLICATION->network());
	job->addNetAction(Net::Download::makeByteArray(
		QUrl(QString("https://api.modrinth.com/v2/project/%1").arg(projectId)),
		response));

	m_pendingRequests++;
	connect(job, &NetJob::succeeded, this, [this, projectId, response, job]() {
		job->deleteLater();
		QJsonDocument doc = QJsonDocument::fromJson(*response);
		delete response;

		QString name;
		QString slug;
		if (doc.isObject()) {
			name = Json::ensureString(doc.object(), "title", "");
			slug = Json::ensureString(doc.object(), "slug", "");
		}
		if (!name.isEmpty()) {
			qDebug() << "MR cross resolve extracted title:" << name
					 << "slug:" << slug << "for" << projectId;
			executeCrossResolve("curseforge", name, slug);
		}
		m_pendingRequests--;
		checkCompletion();
	});
	connect(job, &NetJob::failed, this, [this, response, job](QString) {
		job->deleteLater();
		delete response;
		m_pendingRequests--;
		checkCompletion();
	});
	job->start();
}

void DependencyResolver::executeCrossResolve(const QString& targetPlatform,
											 const QString& projectName,
											 const QString& sourceSlug)
{
	// Check if we already resolved something with this name (cross-platform
	// dedup)
	QString normalized = normalizeName(projectName);
	if (m_resolvedNames.contains(normalized)) {
		qDebug() << "Cross resolve skipped - already resolved by name:"
				 << projectName;
		return;
	}

	auto* response = new QByteArray();
	NetJob* job = new NetJob(QString("CrossSearch(%1)").arg(projectName),
							 APPLICATION->network());

	QString query = QUrl::toPercentEncoding(projectName);
	QString url;
	if (targetPlatform == "modrinth") {
		url = QString(
				  "https://api.modrinth.com/v2/search?query=%1&limit=5&facets=")
				  .arg(query) +
			  "[[%22project_type:mod%22]]";
	} else {
		url = QString("https://api.curseforge.com/v1/mods/"
					  "search?gameId=432&classId=6&searchFilter=%1&sortField=2&"
					  "sortOrder=desc&pageSize=5")
				  .arg(query);
	}

	job->addNetAction(Net::Download::makeByteArray(QUrl(url), response));

	m_pendingRequests++;
	connect(
		job, &NetJob::succeeded, this,
		[this, targetPlatform, projectName, sourceSlug, response, job]() {
			job->deleteLater();
			QJsonDocument doc = QJsonDocument::fromJson(*response);
			delete response;

			// Extract multiple hits and find the best match using slug
			// comparison
			QString hitId;
			QString hitSlug;
			QString hitName;

			if (targetPlatform == "modrinth") {
				if (doc.isObject() && doc.object().contains("hits")) {
					auto hits = Json::ensureArray(doc.object(), "hits");
					for (auto hitRaw : hits) {
						auto hitObj = hitRaw.toObject();
						QString candidateSlug =
							Json::ensureString(hitObj, "slug", "");
						QString candidateId =
							Json::ensureString(hitObj, "project_id", "");
						QString candidateName =
							Json::ensureString(hitObj, "title", "");

						// Prefer exact slug match
						if (!sourceSlug.isEmpty() &&
							candidateSlug.compare(sourceSlug,
												  Qt::CaseInsensitive) == 0) {
							hitId = candidateId;
							hitSlug = candidateSlug;
							hitName = candidateName;
							break;
						}
						// Accept normalized name match
						if (normalizeName(candidateName) ==
							normalizeName(projectName)) {
							hitId = candidateId;
							hitSlug = candidateSlug;
							hitName = candidateName;
							break;
						}
						// Keep first result as fallback only if no slug was
						// provided
						if (hitId.isEmpty() && sourceSlug.isEmpty()) {
							hitId = candidateId;
							hitSlug = candidateSlug;
							hitName = candidateName;
						}
					}
				}
			} else {
				if (doc.isObject() && doc.object().contains("data")) {
					auto dataArr = Json::ensureArray(doc.object(), "data");
					for (auto hitRaw : dataArr) {
						auto hitObj = hitRaw.toObject();
						QString candidateSlug =
							Json::ensureString(hitObj, "slug", "");
						QString candidateId = QString::number(
							Json::ensureInteger(hitObj, "id", 0));
						QString candidateName =
							Json::ensureString(hitObj, "name", "");

						if (!sourceSlug.isEmpty() &&
							candidateSlug.compare(sourceSlug,
												  Qt::CaseInsensitive) == 0) {
							hitId = candidateId;
							hitSlug = candidateSlug;
							hitName = candidateName;
							break;
						}
						if (normalizeName(candidateName) ==
							normalizeName(projectName)) {
							hitId = candidateId;
							hitSlug = candidateSlug;
							hitName = candidateName;
							break;
						}
						if (hitId.isEmpty() && sourceSlug.isEmpty()) {
							hitId = candidateId;
							hitSlug = candidateSlug;
							hitName = candidateName;
						}
					}
				}
			}

			// Validate the match quality
			if (!hitId.isEmpty() && !sourceSlug.isEmpty() &&
				hitSlug.isEmpty()) {
				// We had a source slug but couldn't find a slug match — reject
				// ambiguous result
				qWarning() << "Cross resolve rejected ambiguous match for"
						   << projectName << "- source slug:" << sourceSlug
						   << "didn't match any result";
				hitId.clear();
			}

			if (!hitId.isEmpty()) {
				QString key = targetPlatform + ":" + hitId;
				if (!m_resolvedProjectIds.contains(key)) {
					m_resolvedProjectIds.insert(key);
					m_resolvedNames.insert(normalizeName(projectName));
					qDebug() << "Cross resolved to" << targetPlatform
							 << "project" << hitId << "slug:" << hitSlug
							 << "(source slug:" << sourceSlug << ")";

					auto* depResponse = new QByteArray();
					NetJob* depJob =
						new NetJob(QString("CrossDepResolve(%1)").arg(hitId),
								   APPLICATION->network());

					QString fileUrl;
					if (targetPlatform == "modrinth") {
						fileUrl = QString("https://api.modrinth.com/v2/project/"
										  "%1/version?game_versions=[\"%2\"]")
									  .arg(hitId, m_mcVersion);
						if (!m_loader.isEmpty()) {
							fileUrl +=
								QString("&loaders=[\"%1\"]").arg(m_loader);
						}
					} else {
						fileUrl = QString("https://api.curseforge.com/v1/mods/"
										  "%1/files?gameVersion=%2")
									  .arg(hitId, m_mcVersion);
						if (!m_loader.isEmpty()) {
							int loaderType =
								ModPlatform::loaderToCurseForgeModLoaderType(
									m_loader);
							if (loaderType > 0) {
								fileUrl += QString("&modLoaderType=%1")
											   .arg(loaderType);
							}
						}
					}

					depJob->addNetAction(Net::Download::makeByteArray(
						QUrl(fileUrl), depResponse));
					m_pendingRequests++;
					connect(depJob, &NetJob::succeeded, this,
							[this, targetPlatform, hitId, projectName,
							 depResponse, depJob]() {
								depJob->deleteLater();
								onDependencyProjectResolved(
									targetPlatform, hitId, *depResponse);
								delete depResponse;
								m_pendingRequests--;
								checkCompletion();
							});
					connect(depJob, &NetJob::failed, this,
							[this, projectName, depResponse, depJob](QString) {
								depJob->deleteLater();
								delete depResponse;
								ModPlatform::UnresolvedDep unresolved;
								unresolved.name = projectName;
								m_unresolvedDeps.append(unresolved);
								m_pendingRequests--;
								checkCompletion();
							});
					depJob->start();
				} else {
					bool wasResolved = false;
					for (const auto& dep : m_dependencies) {
						if ((dep.platform + ":" + dep.projectId) == key) {
							wasResolved = true;
							break;
						}
					}
					if (!wasResolved) {
						qWarning() << "Dependency" << projectName
								   << "could not be resolved on any platform";
						ModPlatform::UnresolvedDep unresolved;
						unresolved.name = projectName;
						m_unresolvedDeps.append(unresolved);
					}
				}
			} else {
				qWarning() << "Cross resolve search returned no valid match for"
						   << projectName;
				ModPlatform::UnresolvedDep unresolved;
				unresolved.name = projectName;
				m_unresolvedDeps.append(unresolved);
			}

			m_pendingRequests--;
			checkCompletion();
		});
	connect(job, &NetJob::failed, this, [this, response, job](QString) {
		job->deleteLater();
		delete response;
		m_pendingRequests--;
		checkCompletion();
	});
	job->start();
}

void DependencyResolver::checkCompletion()
{
	if (m_pendingRequests <= 0 && m_currentModIndex >= m_selectedMods.size()) {
		// === Pass 1: Deduplicate dependencies by same platform+projectId
		// (version conflict) === If multiple dep chains resolved different
		// versions of the same project, keep the first.
		{
			QSet<QString> seenKeys;
			QList<ModPlatform::DependencyInfo> deduped;
			for (const auto& dep : m_dependencies) {
				QString key = dep.platform + ":" + dep.projectId;
				if (seenKeys.contains(key)) {
					qDebug()
						<< "Removing duplicate dep (same project, version "
						   "conflict):"
						<< dep.name << dep.versionId << "on" << dep.platform;
					continue;
				}
				seenKeys.insert(key);
				deduped.append(dep);
			}
			m_dependencies = deduped;
		}

		// === Pass 2: Cross-platform dedup by normalized name ===
		// Same library resolved from both CF and MR should only appear once.
		{
			QMap<QString, int>
				nameToIndex; // normalized name -> index in deduped list
			QList<ModPlatform::DependencyInfo> deduped;
			for (const auto& dep : m_dependencies) {
				QString normalized = normalizeName(dep.name);
				if (normalized.isEmpty()) {
					deduped.append(dep);
					continue;
				}
				if (nameToIndex.contains(normalized)) {
					// Prefer the one with SHA1, then the one with a download
					// URL
					int existingIdx = nameToIndex[normalized];
					const auto& existing = deduped[existingIdx];
					bool replaceExisting =
						existing.sha1.isEmpty() && !dep.sha1.isEmpty();
					if (replaceExisting) {
						qDebug() << "Cross-platform dedup: replacing"
								 << existing.name << "(" << existing.platform
								 << ") with" << dep.name << "(" << dep.platform
								 << ") - better metadata";
						deduped[existingIdx] = dep;
					} else {
						qDebug() << "Cross-platform dedup: skipping duplicate"
								 << dep.name << "(" << dep.platform
								 << ") - already have" << existing.name << "("
								 << existing.platform << ")";
					}
				} else {
					nameToIndex.insert(normalized, deduped.size());
					deduped.append(dep);
				}
			}
			m_dependencies = deduped;
		}

		// === Pass 3: Deduplicate by fileName (exact same file from different
		// paths) ===
		{
			QSet<QString> seenFiles;
			QList<ModPlatform::DependencyInfo> deduped;
			for (const auto& dep : m_dependencies) {
				if (!dep.fileName.isEmpty() &&
					seenFiles.contains(dep.fileName)) {
					qDebug() << "Removing dep with duplicate filename:"
							 << dep.fileName;
					continue;
				}
				if (!dep.fileName.isEmpty()) {
					seenFiles.insert(dep.fileName);
				}
				deduped.append(dep);
			}
			m_dependencies = deduped;
		}

		// === Pass 4: Also dedup against selected mods by name ===
		{
			QList<ModPlatform::DependencyInfo> deduped;
			for (const auto& dep : m_dependencies) {
				QString normalized = normalizeName(dep.name);
				bool isDuplicate = false;
				for (const auto& selected : m_selectedMods) {
					if (normalizeName(selected.name) == normalized) {
						qDebug() << "Removing dep that duplicates selected mod:"
								 << dep.name;
						isDuplicate = true;
						break;
					}
				}
				if (!isDuplicate) {
					deduped.append(dep);
				}
			}
			m_dependencies = deduped;
		}

		// === Clean up unresolved list ===
		QList<ModPlatform::UnresolvedDep> finalUnresolved;
		QSet<QString> seenNames;
		for (const auto& unresolved : m_unresolvedDeps) {
			if (seenNames.contains(unresolved.name))
				continue;
			bool wasResolved = false;
			for (const auto& dep : m_dependencies) {
				if (dep.name == unresolved.name ||
					dep.projectId == unresolved.projectId ||
					normalizeName(dep.name) == normalizeName(unresolved.name)) {
					wasResolved = true;
					break;
				}
			}
			if (!wasResolved) {
				seenNames.insert(unresolved.name);
				finalUnresolved.append(unresolved);
			}
		}
		m_unresolvedDeps = finalUnresolved;

		if (!m_unresolvedDeps.isEmpty()) {
			qWarning() << "Unresolved dependencies:";
			for (const auto& u : m_unresolvedDeps) {
				qWarning() << "  -" << u.name << "(" << u.platform
						   << u.projectId << ")";
			}
		}

		qDebug() << "Dependency resolution complete:" << m_dependencies.size()
				 << "resolved," << m_unresolvedDeps.size() << "unresolved";

		emitSucceeded();
	}
}
