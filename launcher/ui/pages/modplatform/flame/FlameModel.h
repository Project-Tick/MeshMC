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

#include <RWStorage.h>

#include <QAbstractListModel>
#include <QSortFilterProxyModel>
#include <QThreadPool>
#include <QIcon>
#include <QStyledItemDelegate>
#include <QList>
#include <QString>
#include <QStringList>
#include <QMetaType>

#include <functional>
#include <net/NetJob.h>

#include <modplatform/flame/FlamePackIndex.h>

namespace Flame
{

	typedef QMap<QString, QIcon> LogoMap;
	typedef std::function<void(QString)> LogoCallback;

	class ListModel : public QAbstractListModel
	{
		Q_OBJECT

	  public:
		ListModel(QObject* parent);
		virtual ~ListModel();

		int rowCount(const QModelIndex& parent) const override;
		int columnCount(const QModelIndex& parent) const override;
		QVariant data(const QModelIndex& index, int role) const override;
		Qt::ItemFlags flags(const QModelIndex& index) const override;
		bool canFetchMore(const QModelIndex& parent) const override;
		void fetchMore(const QModelIndex& parent) override;

		void getLogo(const QString& logo, const QString& logoUrl,
					 LogoCallback callback);
		void searchWithTerm(const QString& term, const int sort);

	  private slots:
		void performPaginatedSearch();

		void logoFailed(QString logo);
		void logoLoaded(QString logo, QIcon out);

		void searchRequestFinished();
		void searchRequestFailed(QString reason);

	  private:
		void requestLogo(QString file, QString url);

	  private:
		QList<IndexedPack> modpacks;
		QStringList m_failedLogos;
		QStringList m_loadingLogos;
		LogoMap m_logoMap;
		QMap<QString, LogoCallback> waitingCallbacks;

		QString currentSearchTerm;
		int currentSort = 0;
		int nextSearchOffset = 0;
		enum SearchState {
			None,
			CanPossiblyFetchMore,
			ResetRequested,
			Finished
		} searchState = None;
		NetJob::Ptr jobPtr;
		QByteArray response;
	};

} // namespace Flame
