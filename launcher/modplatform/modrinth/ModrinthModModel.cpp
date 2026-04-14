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

#include "ModrinthModModel.h"
#include "Application.h"
#include "Json.h"

#include <QUrl>

namespace Modrinth
{

	ModListModel::ModListModel(ModPlatform::ContentType contentType,
							   QObject* parent)
		: QAbstractListModel(parent), m_contentType(contentType)
	{
	}

	ModListModel::~ModListModel() {}

	int ModListModel::rowCount(const QModelIndex& parent) const
	{
		return m_mods.size();
	}

	int ModListModel::columnCount(const QModelIndex& parent) const
	{
		return 1;
	}

	QVariant ModListModel::data(const QModelIndex& index, int role) const
	{
		int pos = index.row();
		if (pos >= m_mods.size() || pos < 0 || !index.isValid()) {
			return QString("INVALID INDEX %1").arg(pos);
		}

		auto& mod = m_mods.at(pos);
		if (role == Qt::DisplayRole) {
			return mod.name;
		} else if (role == Qt::ToolTipRole) {
			if (mod.description.length() > 100) {
				QString edit = mod.description.left(97);
				edit = edit.left(edit.lastIndexOf(" ")).append("...");
				return edit;
			}
			return mod.description;
		} else if (role == Qt::DecorationRole) {
			if (m_logoMap.contains(mod.slug)) {
				return m_logoMap.value(mod.slug);
			}
			QIcon icon = APPLICATION->getThemedIcon("screenshot-placeholder");
			const_cast<ModListModel*>(this)->requestLogo(mod.slug, mod.iconUrl);
			return icon;
		} else if (role == Qt::UserRole) {
			QVariant v;
			v.setValue(mod.projectId);
			return v;
		}

		return QVariant();
	}

	Qt::ItemFlags ModListModel::flags(const QModelIndex& index) const
	{
		return QAbstractListModel::flags(index);
	}

	bool ModListModel::canFetchMore(const QModelIndex& parent) const
	{
		return m_searchState == CanPossiblyFetchMore;
	}

	void ModListModel::fetchMore(const QModelIndex& parent)
	{
		if (parent.isValid())
			return;
		if (m_nextSearchOffset == 0)
			return;
		performPaginatedSearch();
	}

	IndexedMod ModListModel::modAt(int index) const
	{
		if (index >= 0 && index < m_mods.size()) {
			return m_mods.at(index);
		}
		return {};
	}

	void ModListModel::searchWithTerm(const QString& term,
									  const QString& mcVersion,
									  const QString& loader, int sort)
	{
		if (m_currentSearchTerm == term && m_mcVersion == mcVersion &&
			m_loader == loader && m_currentSort == sort) {
			return;
		}
		m_currentSearchTerm = term;
		m_mcVersion = mcVersion;
		m_loader = loader;
		m_currentSort = sort;
		if (m_jobPtr) {
			m_jobPtr->abort();
			m_searchState = ResetRequested;
			return;
		} else {
			beginResetModel();
			m_mods.clear();
			endResetModel();
			m_searchState = None;
		}
		m_nextSearchOffset = 0;
		performPaginatedSearch();
	}

	void ModListModel::performPaginatedSearch()
	{
		static const char* sortFields[] = {"relevance", "downloads", "updated",
										   "newest", "follows"};
		int sortIndex =
			(m_currentSort >= 0 && m_currentSort < 5) ? m_currentSort : 0;

		QString projectType =
			ModPlatform::contentTypeToModrinthFacet(m_contentType);

		NetJob* netJob =
			new NetJob("Modrinth::ModSearch", APPLICATION->network());

		QString facets = QString("[[\"project_type:%1\"]").arg(projectType);
		if (!m_mcVersion.isEmpty()) {
			facets += QString(",[\"versions:%1\"]").arg(m_mcVersion);
		}
		// Only filter by loader for mods - resource packs and shaders are
		// loader-agnostic
		if (!m_loader.isEmpty() &&
			m_contentType == ModPlatform::ContentType::Mod) {
			facets += QString(",[\"categories:%1\"]").arg(m_loader);
		}
		facets += "]";

		QString searchUrl = QString("https://api.modrinth.com/v2/search?"
									"query=%1&"
									"index=%2&"
									"offset=%3&"
									"limit=20")
								.arg(m_currentSearchTerm, sortFields[sortIndex])
								.arg(m_nextSearchOffset);
		searchUrl += "&facets=" + facets;
		netJob->addNetAction(
			Net::Download::makeByteArray(QUrl(searchUrl), &m_response));
		m_jobPtr = netJob;
		m_jobPtr->start();
		QObject::connect(netJob, &NetJob::succeeded, this,
						 &ModListModel::searchRequestFinished);
		QObject::connect(netJob, &NetJob::failed, this,
						 &ModListModel::searchRequestFailed);
	}

	void ModListModel::searchRequestFinished()
	{
		m_jobPtr.reset();

		QJsonParseError parse_error;
		QJsonDocument doc = QJsonDocument::fromJson(m_response, &parse_error);
		if (parse_error.error != QJsonParseError::NoError) {
			qWarning() << "Error parsing Modrinth mod search response: "
					   << parse_error.errorString();
			return;
		}

		QList<IndexedMod> newList;
		auto obj = doc.object();
		auto hits = Json::ensureArray(obj, "hits");
		for (auto modRaw : hits) {
			auto modObj = modRaw.toObject();
			IndexedMod mod;
			try {
				mod.projectId = Json::ensureString(modObj, "project_id", "");
				if (mod.projectId.isEmpty()) {
					mod.projectId = Json::requireString(modObj, "id");
				}
				mod.slug = Json::ensureString(modObj, "slug", "");
				mod.name = Json::requireString(modObj, "title");
				mod.description = Json::ensureString(modObj, "description", "");
				mod.author = Json::ensureString(modObj, "author", "");
				mod.downloads = Json::ensureInteger(modObj, "downloads", 0);
				mod.iconUrl = Json::ensureString(modObj, "icon_url", "");

				newList.append(mod);
			} catch (const JSONValidationError& e) {
				qWarning() << "Error loading Modrinth mod: " << e.cause();
				continue;
			}
		}

		int totalHits = Json::ensureInteger(obj, "total_hits", 0);

		if (m_searchState == ResetRequested) {
			beginResetModel();
			m_mods = newList;
			endResetModel();
		} else {
			int firstRow = m_mods.size();
			int lastRow = firstRow + newList.size() - 1;
			if (!newList.isEmpty()) {
				beginInsertRows(QModelIndex(), firstRow, lastRow);
				m_mods.append(newList);
				endInsertRows();
			}
		}

		if (newList.size() < 20 ||
			(m_nextSearchOffset + newList.size()) >= totalHits) {
			m_searchState = Finished;
		} else {
			m_nextSearchOffset += 20;
			m_searchState = CanPossiblyFetchMore;
		}
	}

	void ModListModel::searchRequestFailed(QString reason)
	{
		m_jobPtr.reset();

		if (m_searchState == ResetRequested) {
			beginResetModel();
			m_mods.clear();
			endResetModel();
			m_searchState = None;
			m_nextSearchOffset = 0;
			performPaginatedSearch();
		} else {
			m_searchState = Finished;
		}
	}

	void ModListModel::requestLogo(QString logo, QString url)
	{
		if (m_loadingLogos.contains(logo) || m_failedLogos.contains(logo) ||
			url.isEmpty()) {
			return;
		}

		MetaEntryPtr entry = APPLICATION->metacache()->resolveEntry(
			"ModrinthModIcons", QString("logos/%1").arg(logo));
		NetJob* job = new NetJob(QString("Modrinth Mod Icon %1").arg(logo),
								 APPLICATION->network());
		job->addNetAction(Net::Download::makeCached(QUrl(url), entry));

		auto fullPath = entry->getFullPath();
		QObject::connect(job, &NetJob::succeeded, this,
						 [this, job, logo, fullPath] {
							 job->deleteLater();
							 emit logoLoaded(logo, QIcon(fullPath));
						 });
		QObject::connect(job, &NetJob::failed, this, [this, job, logo] {
			job->deleteLater();
			emit logoFailed(logo);
		});

		job->start();
		m_loadingLogos.append(logo);
	}

	void ModListModel::logoLoaded(QString logo, QIcon out)
	{
		m_loadingLogos.removeAll(logo);
		m_logoMap.insert(logo, out);
		for (int i = 0; i < m_mods.size(); i++) {
			if (m_mods[i].slug == logo) {
				emit dataChanged(createIndex(i, 0), createIndex(i, 0),
								 {Qt::DecorationRole});
			}
		}
	}

	void ModListModel::logoFailed(QString logo)
	{
		m_failedLogos.append(logo);
		m_loadingLogos.removeAll(logo);
	}

} // namespace Modrinth
