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

#pragma once

#include <QAbstractListModel>
#include <QIcon>
#include <QList>
#include <QString>
#include <QStringList>
#include <functional>

#include <RWStorage.h>
#include <net/NetJob.h>
#include <modplatform/modrinth/ModrinthPackIndex.h>
#include <modplatform/ContentType.h>

namespace Modrinth
{

	struct IndexedMod {
		QString projectId;
		QString slug;
		QString name;
		QString description;
		QString author;
		QString iconUrl;
		int downloads = 0;
		QVector<IndexedVersion> versions;
		bool versionsLoaded = false;
	};

	typedef QMap<QString, QIcon> ModLogoMap;
	typedef std::function<void(QString)> ModLogoCallback;

	class ModListModel : public QAbstractListModel
	{
		Q_OBJECT

	  public:
		explicit ModListModel(ModPlatform::ContentType contentType,
							  QObject* parent = nullptr);
		virtual ~ModListModel() override;

		int rowCount(const QModelIndex& parent) const override;
		int columnCount(const QModelIndex& parent) const override;
		QVariant data(const QModelIndex& index, int role) const override;
		Qt::ItemFlags flags(const QModelIndex& index) const override;
		bool canFetchMore(const QModelIndex& parent) const override;
		void fetchMore(const QModelIndex& parent) override;

		void searchWithTerm(const QString& term, const QString& mcVersion,
							const QString& loader, int sort);

		IndexedMod modAt(int index) const;

	  private slots:
		void performPaginatedSearch();

		void logoFailed(QString logo);
		void logoLoaded(QString logo, QIcon out);

		void searchRequestFinished();
		void searchRequestFailed(QString reason);

	  private:
		void requestLogo(QString file, QString url);

	  private:
		ModPlatform::ContentType m_contentType;
		QList<IndexedMod> m_mods;
		QStringList m_failedLogos;
		QStringList m_loadingLogos;
		ModLogoMap m_logoMap;

		QString m_currentSearchTerm;
		QString m_mcVersion;
		QString m_loader;
		int m_currentSort = 0;
		int m_nextSearchOffset = 0;
		enum SearchState {
			None,
			CanPossiblyFetchMore,
			ResetRequested,
			Finished
		} m_searchState = None;
		NetJob::Ptr m_jobPtr;
		QByteArray m_response;
	};

} // namespace Modrinth
