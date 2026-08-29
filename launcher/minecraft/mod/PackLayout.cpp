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

#include "PackLayout.h"

#include <QDir>
#include <QSet>
#include <QString>
#include <QStringList>

#include "FileSystem.h"
#include "MMCZip.h"

namespace
{

	/* Zip entry names are not normalised by the spec: writers emit
	 * backslashes, leading "./" and leading "/" more or less at random.
	 * Fold all of that away once so the layout checks below can work with
	 * plain "a/b/c" strings. */
	QStringList normalizedEntries(const QString& zipPath)
	{
		QStringList out;
		const QStringList raw = MMCZip::listEntries(zipPath);
		out.reserve(raw.size());
		for (QString entry : raw) {
			entry.replace(QLatin1Char('\\'), QLatin1Char('/'));
			while (entry.startsWith(QLatin1String("./"))) {
				entry.remove(0, 2);
			}
			while (entry.startsWith(QLatin1Char('/'))) {
				entry.remove(0, 1);
			}
			if (entry.isEmpty()) {
				continue;
			}
			out.append(entry);
		}
		return out;
	}

	/* The first path segment of an entry, or an empty string when the
	 * entry sits at the archive root. Directory entries keep their
	 * trailing slash, so "foo/" yields "foo" just like "foo/bar" does. */
	QString topLevelFolder(const QString& entry)
	{
		const int slash = entry.indexOf(QLatin1Char('/'));
		if (slash <= 0) {
			return QString();
		}
		return entry.left(slash);
	}

	bool anyEntryUnder(const QStringList& entries, const QString& prefix)
	{
		for (const QString& entry : entries) {
			if (entry.startsWith(prefix, Qt::CaseInsensitive) &&
				entry.size() > prefix.size()) {
				return true;
			}
		}
		return false;
	}

	bool hasSubDirectory(const QFileInfo& root, const QString& name)
	{
		return QFileInfo(FS::PathCombine(root.filePath(), name)).isDir();
	}

	bool hasFile(const QFileInfo& root, const QString& name)
	{
		return QFileInfo(FS::PathCombine(root.filePath(), name)).isFile();
	}

	/* Distinct top-level folders present in the archive, used for the
	 * "wrapped in one folder" case. */
	QSet<QString> topLevelFolders(const QStringList& entries)
	{
		QSet<QString> folders;
		for (const QString& entry : entries) {
			const QString top = topLevelFolder(entry);
			if (!top.isEmpty()) {
				folders.insert(top);
			}
		}
		return folders;
	}

} // namespace

namespace PackLayout
{

	bool isShaderPack(const QFileInfo& file)
	{
		if (!file.exists()) {
			return false;
		}

		if (file.isDir()) {
			if (hasSubDirectory(file, QStringLiteral("shaders"))) {
				return true;
			}
			// Wrapped in a single folder, as produced by "download as zip"
			// on a Git forge and then extracted.
			const QDir dir(file.filePath());
			const QStringList children =
				dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
			for (const QString& child : children) {
				if (QFileInfo(FS::PathCombine(dir.path(), child,
											  QStringLiteral("shaders")))
						.isDir()) {
					return true;
				}
			}
			return false;
		}

		const QStringList entries = normalizedEntries(file.filePath());
		if (entries.isEmpty()) {
			return false;
		}

		for (const QString& entry : entries) {
			if (entry.startsWith(QStringLiteral("shaders/"),
								 Qt::CaseInsensitive) ||
				entry.contains(QStringLiteral("/shaders/"),
							   Qt::CaseInsensitive)) {
				return true;
			}
		}
		return false;
	}

	bool isDataPack(const QFileInfo& file)
	{
		if (!file.exists()) {
			return false;
		}

		if (file.isDir()) {
			if (hasFile(file, QStringLiteral("pack.mcmeta")) &&
				hasSubDirectory(file, QStringLiteral("data"))) {
				return true;
			}
			const QDir dir(file.filePath());
			const QStringList children =
				dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
			for (const QString& child : children) {
				const QFileInfo nested(FS::PathCombine(dir.path(), child));
				if (hasFile(nested, QStringLiteral("pack.mcmeta")) &&
					hasSubDirectory(nested, QStringLiteral("data"))) {
					return true;
				}
			}
			return false;
		}

		const QStringList entries = normalizedEntries(file.filePath());
		if (entries.isEmpty()) {
			return false;
		}

		const QSet<QString> entrySet(entries.begin(), entries.end());
		if (entrySet.contains(QStringLiteral("pack.mcmeta")) &&
			anyEntryUnder(entries, QStringLiteral("data/"))) {
			return true;
		}

		for (const QString& top : topLevelFolders(entries)) {
			if (entrySet.contains(top + QStringLiteral("/pack.mcmeta")) &&
				anyEntryUnder(entries, top + QStringLiteral("/data/"))) {
				return true;
			}
		}
		return false;
	}

} // namespace PackLayout
