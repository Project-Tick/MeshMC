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
 */

#include "MetaCacheSink.h"
#include <QFile>
#include <QFileInfo>
#include "FileSystem.h"
#include "Application.h"

namespace Net
{

	MetaCacheSink::MetaCacheSink(MetaEntryPtr entry, ChecksumValidator* md5sum)
		: Net::FileSink(entry->getFullPath()), m_entry(entry), m_md5Node(md5sum)
	{
		addValidator(md5sum);
	}

	MetaCacheSink::~MetaCacheSink()
	{
		// nil
	}

	JobStatus MetaCacheSink::initCache(QNetworkRequest& request)
	{
		if (!m_entry->isStale()) {
			return Job_Finished;
		}
		// check if file exists, if it does, use its information for the request
		QFile current(m_filename);
		if (current.exists() && current.size() != 0) {
			if (m_entry->getRemoteChangedTimestamp().size()) {
				request.setRawHeader(
					QString("If-Modified-Since").toLatin1(),
					m_entry->getRemoteChangedTimestamp().toLatin1());
			}
			if (m_entry->getETag().size()) {
				request.setRawHeader(QString("If-None-Match").toLatin1(),
									 m_entry->getETag().toLatin1());
			}
		}
		return Job_InProgress;
	}

	JobStatus MetaCacheSink::finalizeCache(QNetworkReply& reply)
	{
		QFileInfo output_file_info(m_filename);
		if (wroteAnyData) {
			m_entry->setMD5Sum(m_md5Node->hash().toHex().constData());
		}
		m_entry->setETag(reply.rawHeader("ETag").constData());
		if (reply.hasRawHeader("Last-Modified")) {
			m_entry->setRemoteChangedTimestamp(
				reply.rawHeader("Last-Modified").constData());
		}
		m_entry->setLocalChangedTimestamp(
			output_file_info.lastModified().toUTC().toMSecsSinceEpoch());
		m_entry->setStale(false);
		APPLICATION->metacache()->updateEntry(m_entry);
		return Job_Finished;
	}

	bool MetaCacheSink::hasLocalData()
	{
		QFileInfo info(m_filename);
		return info.exists() && info.size() != 0;
	}
} // namespace Net
