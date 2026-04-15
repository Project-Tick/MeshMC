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

#include "FileSink.h"
#include <QFile>
#include <QFileInfo>
#include "FileSystem.h"

namespace Net
{

	FileSink::FileSink(QString filename) : m_filename(filename)
	{
		// nil
	}

	FileSink::~FileSink()
	{
		// nil
	}

	JobStatus FileSink::init(QNetworkRequest& request)
	{
		auto result = initCache(request);
		if (result != Job_InProgress) {
			return result;
		}
		// create a new save file and open it for writing
		if (!FS::ensureFilePathExists(m_filename)) {
			qCritical() << "Could not create folder for " + m_filename;
			return Job_Failed;
		}
		wroteAnyData = false;
		m_output_file.reset(new QSaveFile(m_filename));
		if (!m_output_file->open(QIODevice::WriteOnly)) {
			qCritical() << "Could not open " + m_filename + " for writing";
			return Job_Failed;
		}

		if (initAllValidators(request))
			return Job_InProgress;
		return Job_Failed;
	}

	JobStatus FileSink::initCache(QNetworkRequest&)
	{
		return Job_InProgress;
	}

	JobStatus FileSink::write(QByteArray& data)
	{
		if (!writeAllValidators(data) ||
			m_output_file->write(data) != data.size()) {
			qCritical() << "Failed writing into " + m_filename;
			m_output_file->cancelWriting();
			m_output_file.reset();
			wroteAnyData = false;
			return Job_Failed;
		}
		wroteAnyData = true;
		return Job_InProgress;
	}

	JobStatus FileSink::abort()
	{
		m_output_file->cancelWriting();
		failAllValidators();
		return Job_Failed;
	}

	JobStatus FileSink::finalize(QNetworkReply& reply)
	{
		bool gotFile = false;
		QVariant statusCodeV =
			reply.attribute(QNetworkRequest::HttpStatusCodeAttribute);
		bool validStatus = false;
		int statusCode = statusCodeV.toInt(&validStatus);
		if (validStatus) {
			// this leaves out 304 Not Modified
			gotFile = statusCode == 200 || statusCode == 203;
		}
		// if we wrote any data to the save file, we try to commit the data to
		// the real file. if it actually got a proper file, we write it even if
		// it was empty
		if (gotFile || wroteAnyData) {
			// ask validators for data consistency
			// we only do this for actual downloads, not 'your data is still the
			// same' cache hits
			if (!finalizeAllValidators(reply))
				return Job_Failed;
			// nothing went wrong...
			if (!m_output_file->commit()) {
				qCritical() << "Failed to commit changes to " << m_filename;
				m_output_file->cancelWriting();
				return Job_Failed;
			}
		}
		// then get rid of the save file
		m_output_file.reset();

		return finalizeCache(reply);
	}

	JobStatus FileSink::finalizeCache(QNetworkReply&)
	{
		return Job_Finished;
	}

	bool FileSink::hasLocalData()
	{
		QFileInfo info(m_filename);
		return info.exists() && info.size() != 0;
	}
} // namespace Net
