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

#include "FlameModModel.h"
#include "Application.h"
#include "Json.h"

#include <QUrl>

namespace Flame
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
			if (m_logoMap.contains(mod.logoName)) {
				return m_logoMap.value(mod.logoName);
			}
			QIcon icon = APPLICATION->getThemedIcon("screenshot-placeholder");
			const_cast<ModListModel*>(this)->requestLogo(mod.logoName,
														 mod.logoUrl);
			return icon;
		} else if (role == Qt::UserRole) {
			QVariant v;
			v.setValue(mod.addonId);
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
		int classId =
			ModPlatform::contentTypeToCurseForgeClassId(m_contentType);

		static const char* sortOrders[] = {"desc", "desc", "desc",
										   "asc",  "asc",  "desc"};
		const char* sortOrder = (m_currentSort >= 0 && m_currentSort < 6)
									? sortOrders[m_currentSort]
									: "desc";

		NetJob* netJob = new NetJob("Flame::ModSearch", APPLICATION->network());
		auto searchUrl = QString("https://api.curseforge.com/v1/mods/search?"
								 "gameId=432&"
								 "classId=%1&"
								 "index=%2&"
								 "pageSize=25&"
								 "searchFilter=%3&"
								 "sortField=%4&"
								 "sortOrder=%5")
							 .arg(classId)
							 .arg(m_nextSearchOffset)
							 .arg(m_currentSearchTerm)
							 .arg(m_currentSort + 1)
							 .arg(sortOrder);

		if (!m_mcVersion.isEmpty()) {
			searchUrl += "&gameVersion=" + m_mcVersion;
		}

		// Only filter by loader for mods - resource packs and shaders are
		// loader-agnostic
		if (!m_loader.isEmpty() &&
			m_contentType == ModPlatform::ContentType::Mod) {
			int loaderType =
				ModPlatform::loaderToCurseForgeModLoaderType(m_loader);
			if (loaderType > 0) {
				searchUrl += "&modLoaderType=" + QString::number(loaderType);
			}
		}

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
			qWarning() << "Error parsing CurseForge mod search response: "
					   << parse_error.errorString();
			return;
		}

		QList<IndexedMod> newList;
		QJsonArray arr;
		if (doc.isObject() && doc.object().contains("data")) {
			arr = doc.object().value("data").toArray();
		} else {
			arr = doc.array();
		}

		for (auto modRaw : arr) {
			auto obj = modRaw.toObject();
			IndexedMod mod;
			try {
				mod.addonId = Json::requireInteger(obj, "id");
				mod.name = Json::requireString(obj, "name");
				mod.description = Json::ensureString(obj, "summary", "");

				if (obj.contains("links") && obj.value("links").isObject()) {
					mod.websiteUrl = Json::ensureString(
						obj.value("links").toObject(), "websiteUrl", "");
				} else {
					mod.websiteUrl = Json::ensureString(obj, "websiteUrl", "");
				}

				if (obj.contains("logo") && obj.value("logo").isObject()) {
					auto logoObj = obj.value("logo").toObject();
					mod.logoName =
						Json::ensureString(logoObj, "title", mod.name);
					mod.logoUrl =
						Json::ensureString(logoObj, "thumbnailUrl", "");
				} else {
					mod.logoName = mod.name;
				}

				auto authors = Json::ensureArray(obj, "authors");
				QStringList authorNames;
				for (auto authorIter : authors) {
					auto author = authorIter.toObject();
					authorNames.append(Json::ensureString(author, "name", ""));
				}
				mod.author = authorNames.join(", ");

				newList.append(mod);
			} catch (const JSONValidationError& e) {
				qWarning() << "Error loading CurseForge mod: " << e.cause();
				continue;
			}
		}

		auto pagination = doc.object().value("pagination").toObject();
		int totalCount = Json::ensureInteger(pagination, "totalCount", 0);

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

		if (newList.size() < 25 ||
			(m_nextSearchOffset + newList.size()) >= totalCount) {
			m_searchState = Finished;
		} else {
			m_nextSearchOffset += 25;
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
			"FlameModIcons", QString("logos/%1").arg(logo.section(".", 0, 0)));
		NetJob* job = new NetJob(QString("Flame Mod Icon %1").arg(logo),
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
			if (m_mods[i].logoName == logo) {
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

} // namespace Flame
