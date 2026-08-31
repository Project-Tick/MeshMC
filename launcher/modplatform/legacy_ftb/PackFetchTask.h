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
