/* Copyright 2026 Project Tick
 *
 * This source is subject to the Microsoft Permissive License (MS-PL).
 * Please see the COPYING.md file for more information.
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
