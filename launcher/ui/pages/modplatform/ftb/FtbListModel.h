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
 *
 *  This file incorporates work covered by the following copyright and
 *  permission notice:
 *
 * Copyright 2020-2021 Jamie Mansfield <jmansfield@cadixdev.org>
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

#include "modplatform/modpacksch/FTBPackManifest.h"
#include "net/NetJob.h"
#include <QIcon>

namespace Ftb
{

	struct Logo {
		QString fullpath;
		NetJob::Ptr downloadJob;
		QIcon result;
		bool failed = false;
	};

	typedef QMap<QString, Logo> LogoMap;
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

		void request();

		void getLogo(const QString& logo, const QString& logoUrl,
					 LogoCallback callback);

	  private slots:
		void requestFinished();
		void requestFailed(QString reason);

		void requestPack();
		void packRequestFinished();
		void packRequestFailed(QString reason);

		void logoFailed(QString logo);
		void logoLoaded(QString logo, bool stale);

	  private:
		void requestLogo(QString file, QString url);

	  private:
		QList<ModpacksCH::Modpack> modpacks;
		LogoMap m_logoMap;

		NetJob::Ptr jobPtr;
		int currentPack;
		QList<int> remainingPacks;
		QByteArray response;
	};

} // namespace Ftb
