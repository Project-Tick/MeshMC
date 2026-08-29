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

#include <QFlags>
#include <QList>
#include <QString>
#include <QStringList>

/*
 * Rendering a folder's contents as a list a person can paste somewhere.
 *
 * Deliberately knows nothing about Mod, the folder models or the sidecar
 * index: the page collects what it knows about each file into an Item and
 * this decides what the text looks like. That keeps the formats testable
 * on their own and means the same code serves mods, resource packs,
 * shader packs and data packs without a single branch on content type.
 */
namespace ContentListExport
{
	enum class Format { Html, Markdown, PlainText, Json, Csv, Custom };

	/* Which of the optional columns to include. The name is always
	 * there, everything else is asked for. */
	enum Field {
		NoFields = 0,
		Authors = 1 << 0,
		Url = 1 << 1,
		Version = 1 << 2,
		FileName = 1 << 3
	};
	Q_DECLARE_FLAGS(Fields, Field)

	struct Item {
		QString name;
		QString version;
		QString url;
		QString fileName;
		/* The mod's own id as declared inside the archive, not the
		 * platform's project id. Only reachable through the custom
		 * template, which is also the only place it is any use. */
		QString modId;
		QStringList authors;
	};

	/* One of the built-in formats. `fields` is ignored by Custom, which
	 * has no way to express it - use the template overload instead. */
	QString render(const QList<Item>& items, Format format, Fields fields);

	/* One line per item, with `{name}`, `{mod_id}`, `{url}`,
	 * `{version}`, `{authors}` and `{filename}` substituted. */
	QString render(const QList<Item>& items, const QString& lineTemplate);

	/* The line a format's own output looks like, offered as the starting
	 * point when the user switches to a custom template. */
	QString exampleLine(Format format);

	/* Suffix to propose in the save dialog. */
	QString fileExtension(Format format);
}

Q_DECLARE_OPERATORS_FOR_FLAGS(ContentListExport::Fields)
