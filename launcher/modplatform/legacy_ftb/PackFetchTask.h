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

#pragma once

#include "net/NetJob.h"
#include <QTemporaryDir>
#include <QByteArray>
#include <QObject>
#include "PackHelpers.h"

namespace LegacyFTB
{

	class PackFetchTask : public QObject
	{

		Q_OBJECT

	  public:
		PackFetchTask(shared_qobject_ptr<QNetworkAccessManager> network)
			: QObject(nullptr), m_network(network) {};
		virtual ~PackFetchTask() = default;

		void fetch();
		void fetchPrivate(const QStringList& toFetch);

	  private:
		shared_qobject_ptr<QNetworkAccessManager> m_network;
		NetJob::Ptr jobPtr;

		QByteArray publicModpacksXmlFileData;
		QByteArray thirdPartyModpacksXmlFileData;

		bool parseAndAddPacks(QByteArray& data, PackType packType,
							  ModpackList& list);
		ModpackList publicPacks;
		ModpackList thirdPartyPacks;

	  protected slots:
		void fileDownloadFinished();
		void fileDownloadFailed(QString reason);

	  signals:
		void finished(ModpackList publicPacks, ModpackList thirdPartyPacks);
		void failed(QString reason);

		void privateFileDownloadFinished(Modpack modpack);
		void privateFileDownloadFailed(QString reason, QString packCode);
	};

} // namespace LegacyFTB
