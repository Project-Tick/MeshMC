/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-FileContributor: Project Tick
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright 2013-2021 MultiMC Contributors
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

#include <QAbstractListModel>
#include <memory>

struct Language;

/*!
 * The languages MeshMC is translated into, and the selection of the one in
 * use.
 *
 * The translations are the po/<language>/meshmc.po files of the repository,
 * compiled to .mo catalogs and installed together with the program (see
 * ki18n_install() in the top-level CMakeLists). Nothing is downloaded: a
 * language is listed if its catalog is installed, and the application's
 * strings are looked up by KI18n.
 */
class TranslationsModel : public QAbstractListModel
{
	Q_OBJECT
  public:
	explicit TranslationsModel(QObject* parent = nullptr);
	virtual ~TranslationsModel();

	QVariant data(const QModelIndex& index,
				  int role = Qt::DisplayRole) const override;
	QVariant headerData(int section, Qt::Orientation orientation,
						int role) const override;
	int rowCount(const QModelIndex& parent = QModelIndex()) const override;
	int columnCount(const QModelIndex& parent) const override;

	bool selectLanguage(QString key);
	QModelIndex selectedIndex();
	QString selectedLanguage();

	/*!
	 * Directory with the catalogs that were installed together with the
	 * program: <dir>/<language>/LC_MESSAGES/meshmc.mo.
	 */
	static QString bundledCatalogDir();

  private:
	Language* findLanguage(const QString& key);
	void loadLanguages();

	// hide copy constructor
	TranslationsModel(const TranslationsModel&) = delete;
	// hide assign op
	TranslationsModel& operator=(const TranslationsModel&) = delete;

  private: /* data */
	struct Private;
	std::unique_ptr<Private> d;
};
