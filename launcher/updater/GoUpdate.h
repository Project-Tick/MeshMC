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

#pragma once
#include <QByteArray>
#include <net/NetJob.h>

namespace GoUpdate
{

	/**
	 * A temporary object exchanged between updated checker and the actual
	 * update task
	 */
	struct Status {
		bool updateAvailable = false;

		int newVersionId = -1;
		QString newRepoUrl;

		int currentVersionId = -1;
		QString currentRepoUrl;

		// path to the root of the application
		QString rootPath;
	};

	/**
	 * Struct that describes an entry in a VersionFileEntry's `Sources` list.
	 */
	struct FileSource {
		FileSource(QString type, QString url, QString compression = "")
		{
			this->type = type;
			this->url = url;
			this->compressionType = compression;
		}

		bool operator==(const FileSource& f2) const
		{
			return type == f2.type && url == f2.url &&
				   compressionType == f2.compressionType;
		}

		QString type;
		QString url;
		QString compressionType;
	};
	typedef QList<FileSource> FileSourceList;

	/**
	 * Structure that describes an entry in a GoUpdate version's `Files` list.
	 */
	struct VersionFileEntry {
		QString path;
		int mode;
		FileSourceList sources;
		QString md5;
		bool operator==(const VersionFileEntry& v2) const
		{
			return path == v2.path && mode == v2.mode &&
				   sources == v2.sources && md5 == v2.md5;
		}
	};
	typedef QList<VersionFileEntry> VersionFileList;

	/**
	 * Structure that describes an operation to perform when installing updates.
	 */
	struct Operation {
		static Operation CopyOp(QString from, QString to, int fmode = 0644)
		{
			return Operation{OP_REPLACE, from, to, fmode};
		}
		static Operation DeleteOp(QString file)
		{
			return Operation{OP_DELETE, QString(), file, 0644};
		}

		// FIXME: for some types, some of the other fields are irrelevant!
		bool operator==(const Operation& u2) const
		{
			return type == u2.type && source == u2.source &&
				   destination == u2.destination &&
				   destinationMode == u2.destinationMode;
		}

		//! Specifies the type of operation that this is.
		enum Type {
			OP_REPLACE,
			OP_DELETE,
		} type;

		//! The source file, if any
		QString source;

		//! The destination file.
		QString destination;

		//! The mode to change the destination file to.
		int destinationMode;
	};
	typedef QList<Operation> OperationList;

	/**
	 * Loads the file list from the given version info JSON object into the
	 * given list.
	 */
	bool parseVersionInfo(const QByteArray& data, VersionFileList& list,
						  QString& error);

	/*!
	 * Takes a list of file entries for the current version's files and the new
	 * version's files and populates the downloadList and operationList with
	 * information about how to download and install the update.
	 */
	bool processFileLists(const VersionFileList& currentVersion,
						  const VersionFileList& newVersion,
						  const QString& rootPath, const QString& tempPath,
						  NetJob::Ptr job, OperationList& ops);

} // namespace GoUpdate
Q_DECLARE_METATYPE(GoUpdate::Status)
