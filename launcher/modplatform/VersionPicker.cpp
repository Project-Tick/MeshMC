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

#include "VersionPicker.h"

#include <QDateTime>
#include <QJsonValue>
#include <QStringList>

namespace
{

	/* Loader names as the two providers spell them, lower-cased. Used to
	 * tell a loader apart from a game version in CurseForge's mixed
	 * list, so anything else in there is left alone. */
	const QStringList& knownLoaderNames()
	{
		static const QStringList names = {
			QStringLiteral("neoforge"),      QStringLiteral("forge"),
			QStringLiteral("fabric"),        QStringLiteral("quilt"),
			QStringLiteral("liteloader"),    QStringLiteral("babric"),
			QStringLiteral("bta-babric"),    QStringLiteral("legacy-fabric"),
			QStringLiteral("ornithe"),       QStringLiteral("rift"),
		};
		return names;
	}

	/* Publish dates are RFC 3339 on both providers. Parsed rather than
	 * compared as text, because an entry with a missing or malformed
	 * date has to lose to a real one instead of winning on punctuation. */
	QDateTime parseDate(const QJsonObject& obj, const QString& key)
	{
		return QDateTime::fromString(obj.value(key).toString(), Qt::ISODate);
	}

	/* Whether this entry may run on `loader`.
	 *
	 * An entry that states no loader at all is accepted: plenty of
	 * library mods do not, and refusing them would leave a dependency
	 * unresolvable for no good reason. */
	bool loaderMatches(const QStringList& entryLoaders, const QString& loader)
	{
		if (loader.isEmpty() || entryLoaders.isEmpty()) {
			return true;
		}
		return entryLoaders.contains(loader, Qt::CaseInsensitive);
	}

} // namespace

QJsonObject ModPlatform::newestCurseForgeFile(const QJsonArray& files,
											  const QString& loader)
{
	QJsonObject best;
	QDateTime bestDate;

	for (const auto& fileRaw : files) {
		const auto file = fileRaw.toObject();
		if (file.isEmpty()) {
			continue;
		}

		/* CurseForge puts game versions and loader names in the same
		 * array, so the loader names have to be picked back out of it. */
		QStringList entryLoaders;
		for (const auto& tagRaw : file.value("gameVersions").toArray()) {
			const QString tag = tagRaw.toString();
			if (knownLoaderNames().contains(tag.toLower())) {
				entryLoaders.append(tag);
			}
		}
		if (!loaderMatches(entryLoaders, loader)) {
			continue;
		}

		const QDateTime date = parseDate(file, QStringLiteral("fileDate"));
		if (best.isEmpty() || (date.isValid() && date > bestDate)) {
			best = file;
			bestDate = date;
		}
	}

	return best;
}

QJsonObject ModPlatform::newestModrinthVersion(const QJsonArray& versions,
											   const QString& loader)
{
	QJsonObject best;
	QDateTime bestDate;

	for (const auto& versionRaw : versions) {
		const auto version = versionRaw.toObject();
		if (version.isEmpty()) {
			continue;
		}

		QStringList entryLoaders;
		for (const auto& loaderRaw : version.value("loaders").toArray()) {
			entryLoaders.append(loaderRaw.toString());
		}
		if (!loaderMatches(entryLoaders, loader)) {
			continue;
		}

		const QDateTime date =
			parseDate(version, QStringLiteral("date_published"));
		if (best.isEmpty() || (date.isValid() && date > bestDate)) {
			best = version;
			bestDate = date;
		}
	}

	return best;
}
