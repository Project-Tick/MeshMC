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

#include "PackContents.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QSet>

#include "FileSystem.h"

namespace PackContents
{
	namespace
	{
		/* Bumped only when the meaning of what is already on disk
		 * changes. Readers refuse anything they do not know, because the
		 * list drives deletions and a misread list deletes the wrong
		 * files. */
		const int currentFormatVersion = 1;

		const char* const listFileName = "pack-contents.json";
		const char* const keyFormatVersion = "formatVersion";
		const char* const keyPaths = "paths";

		/* Length of ".disabled", the suffix the launcher and the pack
		 * formats both use to keep a mod on disk without loading it. */
		const int disabledSuffixLength = 9;

		QString disabledSuffix()
		{
			return QStringLiteral(".disabled");
		}

		/* The other name the same file could be sitting under, or an
		 * empty string when there is no such name. */
		QString counterpartOf(const QString& path)
		{
			if (!path.endsWith(disabledSuffix(), Qt::CaseInsensitive)) {
				return path + disabledSuffix();
			}

			const QString stripped =
				path.left(path.size() - disabledSuffixLength);
			/* A bare ".disabled" is not a file the launcher renamed, and
			 * stripping the suffix off one would name the *folder* that
			 * contains it. This list is handed to a delete loop, so that
			 * is not a mistake worth risking for a file name nobody
			 * writes on purpose. */
			if (stripped.isEmpty() || stripped.endsWith(QLatin1Char('/'))) {
				return {};
			}
			return stripped;
		}

		QStringList normalizeAll(const QStringList& paths)
		{
			QStringList out;
			out.reserve(paths.size());
			for (const QString& path : paths) {
				const QString clean = normalizePath(path);
				if (clean.isEmpty()) {
					continue;
				}
				out.append(clean);
			}
			out.removeDuplicates();
			out.sort();
			return out;
		}
	} // namespace

	QString listPath(const QString& instanceRoot)
	{
		return FS::PathCombine(instanceRoot, QLatin1String(listFileName));
	}

	QString normalizePath(const QString& path)
	{
		if (path.isEmpty()) {
			return {};
		}

		/* Written on one platform and read on another in the general
		 * case - an instance directory travels between machines - so the
		 * stored form is always forward slashes. */
		QString clean =
			QString(path).replace(QLatin1Char('\\'), QLatin1Char('/'));
		clean = QDir::cleanPath(clean);

		if (clean.isEmpty() || clean == QLatin1String(".")) {
			return {};
		}
		if (QDir::isAbsolutePath(clean) || clean.startsWith(QLatin1Char('/'))) {
			/* A game-relative list is the whole contract here; an
			 * absolute path in it would point at a file outside the
			 * instance and still be handed to a delete loop. */
			return {};
		}

		/* cleanPath resolves "a/../b" on its own, so a ".." left in the
		 * result is one that climbs above the game directory. */
		const QStringList segments =
			clean.split(QLatin1Char('/'), Qt::SkipEmptyParts);
		if (segments.contains(QLatin1String(".."))) {
			return {};
		}

		return segments.join(QLatin1Char('/'));
	}

	bool write(const QString& instanceRoot,
			   const QStringList& gameRelativePaths)
	{
		const QString path = listPath(instanceRoot);

		QJsonArray paths;
		for (const QString& entry : normalizeAll(gameRelativePaths)) {
			paths.append(entry);
		}

		QJsonObject root;
		root.insert(QLatin1String(keyFormatVersion), currentFormatVersion);
		root.insert(QLatin1String(keyPaths), paths);

		const QByteArray payload =
			QJsonDocument(root).toJson(QJsonDocument::Indented);

		/* QSaveFile because a half-written list is worse than none: the
		 * next update would read a truncated set of paths and conclude
		 * that everything missing from it is the user's own file. */
		QSaveFile file(path);
		if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
			qWarning() << "PackContents: cannot open" << path << "for writing";
			return false;
		}
		if (file.write(payload) != payload.size()) {
			file.cancelWriting();
			qWarning() << "PackContents: short write to" << path;
			return false;
		}
		if (!file.commit()) {
			qWarning() << "PackContents: failed to commit" << path;
			return false;
		}
		return true;
	}

	bool read(const QString& instanceRoot, QStringList& out)
	{
		const QString path = listPath(instanceRoot);

		QFile file(path);
		if (!file.exists()) {
			/* Installed before the launcher recorded this, or by
			 * something else entirely. Not an error, just unknown. */
			return false;
		}
		if (!file.open(QIODevice::ReadOnly)) {
			qWarning() << "PackContents: cannot read" << path;
			return false;
		}
		const QByteArray bytes = file.readAll();
		file.close();

		QJsonParseError error{};
		const QJsonDocument doc = QJsonDocument::fromJson(bytes, &error);
		if (error.error != QJsonParseError::NoError || !doc.isObject()) {
			qWarning() << "PackContents: malformed list at" << path;
			return false;
		}

		const QJsonObject root = doc.object();
		const int version =
			root.value(QLatin1String(keyFormatVersion)).toInt(0);
		if (version != currentFormatVersion) {
			qWarning() << "PackContents: unsupported format version" << version
					   << "at" << path;
			return false;
		}

		const QJsonValue pathsValue = root.value(QLatin1String(keyPaths));
		if (!pathsValue.isArray()) {
			qWarning() << "PackContents: list at" << path
					   << "carries no paths array";
			return false;
		}

		QStringList paths;
		for (const QJsonValue& entry : pathsValue.toArray()) {
			if (!entry.isString()) {
				continue;
			}
			paths.append(entry.toString());
		}

		out = normalizeAll(paths);
		return true;
	}

	QStringList staleEntries(const QStringList& oldPaths,
							 const QStringList& newPaths)
	{
		const QStringList oldClean = normalizeAll(oldPaths);
		const QStringList newClean = normalizeAll(newPaths);

		QSet<QString> shipped(newClean.begin(), newClean.end());

		QStringList stale;
		for (const QString& path : oldClean) {
			if (shipped.contains(path)) {
				continue;
			}
			stale.append(path);

			const QString counterpart = counterpartOf(path);
			if (!counterpart.isEmpty() && !shipped.contains(counterpart)) {
				stale.append(counterpart);
			}
		}

		stale.removeDuplicates();
		stale.sort();
		return stale;
	}
} // namespace PackContents
