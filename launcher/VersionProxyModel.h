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
#include <QAbstractProxyModel>
#include "BaseVersionList.h"

#include <Filter.h>

class VersionFilterModel;

class VersionProxyModel : public QAbstractProxyModel
{
	Q_OBJECT
  public:
	enum Column { Name, ParentVersion, Branch, Type, Architecture, Path, Time };
	typedef QHash<BaseVersionList::ModelRoles, std::shared_ptr<Filter>>
		FilterMap;

  public:
	VersionProxyModel(QObject* parent = 0);
	virtual ~VersionProxyModel() {};

	virtual int
	columnCount(const QModelIndex& parent = QModelIndex()) const override;
	virtual int
	rowCount(const QModelIndex& parent = QModelIndex()) const override;
	virtual QModelIndex
	mapFromSource(const QModelIndex& sourceIndex) const override;
	virtual QModelIndex
	mapToSource(const QModelIndex& proxyIndex) const override;
	virtual QModelIndex
	index(int row, int column,
		  const QModelIndex& parent = QModelIndex()) const override;
	virtual QVariant data(const QModelIndex& index,
						  int role = Qt::DisplayRole) const override;
	virtual QVariant headerData(int section, Qt::Orientation orientation,
								int role = Qt::DisplayRole) const override;
	virtual QModelIndex parent(const QModelIndex& child) const override;
	virtual void setSourceModel(QAbstractItemModel* sourceModel) override;

	const FilterMap& filters() const;
	void setFilter(const BaseVersionList::ModelRoles column, Filter* filter);
	void clearFilters();
	QModelIndex getRecommended() const;
	QModelIndex getVersion(const QString& version) const;
	void setCurrentVersion(const QString& version);
  private slots:

	void sourceDataChanged(const QModelIndex& source_top_left,
						   const QModelIndex& source_bottom_right);

	void sourceAboutToBeReset();
	void sourceReset();

	void sourceRowsAboutToBeInserted(const QModelIndex& parent, int first,
									 int last);
	void sourceRowsInserted(const QModelIndex& parent, int first, int last);

	void sourceRowsAboutToBeRemoved(const QModelIndex& parent, int first,
									int last);
	void sourceRowsRemoved(const QModelIndex& parent, int first, int last);

  private:
	QList<Column> m_columns;
	FilterMap m_filters;
	BaseVersionList::RoleList roles;
	VersionFilterModel* filterModel;
	bool hasRecommended = false;
	bool hasLatest = false;
	QString m_currentVersion;
};
