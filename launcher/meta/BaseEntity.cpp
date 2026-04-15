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
 * Copyright 2015-2021 MultiMC Contributors
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

#include "BaseEntity.h"

#include "net/Download.h"
#include "net/HttpMetaCache.h"
#include "net/NetJob.h"
#include "Json.h"

#include "BuildConfig.h"
#include "Application.h"

class ParsingValidator : public Net::Validator
{
  public: /* con/des */
	ParsingValidator(Meta::BaseEntity* entity) : m_entity(entity) {};
	virtual ~ParsingValidator() {};

  public: /* methods */
	bool init(QNetworkRequest&) override
	{
		return true;
	}
	bool write(QByteArray& data) override
	{
		this->data.append(data);
		return true;
	}
	bool abort() override
	{
		return true;
	}
	bool validate(QNetworkReply&) override
	{
		auto fname = m_entity->localFilename();
		try {
			auto doc = Json::requireDocument(data, fname);
			auto obj = Json::requireObject(doc, fname);
			m_entity->parse(obj);
			return true;
		} catch (const Exception& e) {
			qWarning() << "Unable to parse response:" << e.cause();
			return false;
		}
	}

  private: /* data */
	QByteArray data;
	Meta::BaseEntity* m_entity;
};

Meta::BaseEntity::~BaseEntity() {}

QUrl Meta::BaseEntity::url() const
{
	return QUrl(BuildConfig.META_URL).resolved(localFilename());
}

bool Meta::BaseEntity::loadLocalFile()
{
	const QString fname = QDir("meta").absoluteFilePath(localFilename());
	if (!QFile::exists(fname)) {
		return false;
	}
	// TODO: check if the file has the expected checksum
	try {
		auto doc = Json::requireDocument(fname, fname);
		auto obj = Json::requireObject(doc, fname);
		parse(obj);
		return true;
	} catch (const Exception& e) {
		qDebug()
			<< QString("Unable to parse file %1: %2").arg(fname, e.cause());
		// just make sure it's gone and we never consider it again.
		QFile::remove(fname);
		return false;
	}
}

void Meta::BaseEntity::load(Net::Mode loadType)
{
	// load local file if nothing is loaded yet
	if (!isLoaded()) {
		if (loadLocalFile()) {
			m_loadStatus = LoadStatus::Local;
		}
	}
	// if we need remote update, run the update task
	if (loadType == Net::Mode::Offline || !shouldStartRemoteUpdate()) {
		return;
	}
	m_updateTask =
		new NetJob(QObject::tr("Download of meta file %1").arg(localFilename()),
				   APPLICATION->network());
	auto url = this->url();
	auto entry =
		APPLICATION->metacache()->resolveEntry("meta", localFilename());
	entry->setStale(true);
	auto dl = Net::Download::makeCached(url, entry);
	/*
	 * The validator parses the file and loads it into the object.
	 * If that fails, the file is not written to storage.
	 */
	dl->addValidator(new ParsingValidator(this));
	m_updateTask->addNetAction(dl);
	m_updateStatus = UpdateStatus::InProgress;
	QObject::connect(m_updateTask.get(), &NetJob::succeeded, [this]() {
		m_loadStatus = LoadStatus::Remote;
		m_updateStatus = UpdateStatus::Succeeded;
	});
	QObject::connect(m_updateTask.get(), &NetJob::failed,
					 [this]() { m_updateStatus = UpdateStatus::Failed; });
	m_updateTask->start();
}

bool Meta::BaseEntity::isLoaded() const
{
	return m_loadStatus > LoadStatus::NotLoaded;
}

bool Meta::BaseEntity::shouldStartRemoteUpdate() const
{
	// TODO: version-locks and offline mode?
	return m_updateStatus != UpdateStatus::InProgress;
}

Task::Ptr Meta::BaseEntity::getCurrentTask()
{
	if (m_updateStatus == UpdateStatus::InProgress) {
		return m_updateTask;
	}
	return nullptr;
}
