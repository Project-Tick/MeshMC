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

#include "CustomTheme.h"

#include <utility>

#include <QDir>
#include <QObject>
#include <FileSystem.h>
#include <Json.h>

const QString CustomTheme::manifestFileName = QStringLiteral("theme.json");
const QString CustomTheme::defaultStyleSheetName =
	QStringLiteral("themeStyle.css");

namespace
{

/// Palette roles a theme may override, paired with their manifest key.
struct PaletteRole {
	QPalette::ColorRole role;
	const char* key;
};

const PaletteRole paletteRoles[] = {
	{QPalette::Window, "Window"},
	{QPalette::WindowText, "WindowText"},
	{QPalette::Base, "Base"},
	{QPalette::AlternateBase, "AlternateBase"},
	{QPalette::ToolTipBase, "ToolTipBase"},
	{QPalette::ToolTipText, "ToolTipText"},
	{QPalette::Text, "Text"},
	{QPalette::Button, "Button"},
	{QPalette::ButtonText, "ButtonText"},
	{QPalette::BrightText, "BrightText"},
	{QPalette::Link, "Link"},
	{QPalette::Highlight, "Highlight"},
	{QPalette::HighlightedText, "HighlightedText"},
};

/*!
 * Everything a manifest can declare, gathered before any of it is applied.
 * This is what makes loading atomic: a manifest that turns out to be broken
 * half way through cannot leave a theme partially overridden.
 */
struct ManifestData {
	QString name;
	QString widgets;
	QString styleSheetName;
	QString tooltip;
	bool hasColors = false;
	QList<QPair<QPalette::ColorRole, QColor>> colors;
	QColor fadeColor;
	double fadeAmount = 0.5;
};

/// Reads one colour, treating "absent" and "unparseable" alike as "not set".
QColor readColor(const QJsonObject& colors, const QString& key)
{
	const QString value = Json::ensureString(colors, key, QString());
	if (value.isEmpty()) {
		return {};
	}
	QColor color(value);
	if (!color.isValid()) {
		qWarning() << "Colour value" << value << "for" << key
				   << "was not recognised.";
		return {};
	}
	return color;
}

/// \return false when \a path is missing, unparseable, or lacks a required
///         field. \a out is then meaningless and must not be applied.
bool readManifest(const QString& path, ManifestData& out)
{
	const QFileInfo info(path);
	if (!info.exists() || !info.isFile()) {
		qDebug() << "No theme manifest at" << path;
		return false;
	}

	try {
		const QJsonObject root =
			Json::requireDocument(path, "Theme JSON file").object();

		// Required fields first, so an incomplete manifest is rejected before
		// we bother collecting the rest.
		out.name = Json::requireString(root, "name", "Theme name");
		out.widgets = Json::requireString(root, "widgets", "Qt widget theme");
		out.styleSheetName = Json::ensureString(
			root, "qssFilePath", CustomTheme::defaultStyleSheetName);

		if (root.contains("colors")) {
			out.hasColors = true;
			const QJsonObject colors =
				Json::requireObject(root, "colors", "colors object");

			for (const auto& entry : paletteRoles) {
				const QColor color =
					readColor(colors, QLatin1String(entry.key));
				if (color.isValid()) {
					out.colors.append({entry.role, color});
				}
			}

			out.fadeColor = readColor(colors, QStringLiteral("fadeColor"));
			out.fadeAmount =
				Json::ensureDouble(colors, "fadeAmount", 0.5, "fade amount");
		}

		// Optional metadata, shown as the theme's tooltip on the Appearance
		// settings page. Missing fields are skipped rather than left blank.
		QStringList tooltipLines;
		const auto addLine = [&root, &tooltipLines](const QString& key,
													const QString& format) {
			const QString value = Json::ensureString(root, key, QString());
			if (!value.isEmpty()) {
				tooltipLines.append(format.arg(value));
			}
		};
		addLine(QStringLiteral("description"), QStringLiteral("%1"));
		addLine(QStringLiteral("author"), QObject::tr("By %1"));
		addLine(QStringLiteral("license"), QObject::tr("License: %1"));
		out.tooltip = tooltipLines.join(QLatin1Char('\n'));
	} catch (const Exception& e) {
		qWarning() << "Couldn't load theme manifest" << path << ":"
				   << e.cause();
		return false;
	}
	return true;
}

/// Writes a manifest describing \a base, named \a name.
bool writeManifest(const QString& path, ITheme* base, const QString& name)
{
	QJsonObject root;
	root.insert("name", name);
	root.insert("widgets", base->qtTheme());

	const QPalette palette = base->colorScheme();
	QJsonObject colors;
	for (const auto& entry : paletteRoles) {
		colors.insert(QLatin1String(entry.key),
					  palette.color(entry.role).name());
	}
	colors.insert("fadeColor", base->fadeColor().name());
	colors.insert("fadeAmount", base->fadeAmount());
	root.insert("colors", colors);

	try {
		Json::write(root, path);
		return true;
	} catch (const Exception& e) {
		qWarning() << "Couldn't write theme manifest" << path << ":"
				   << e.cause();
		return false;
	}
}

} // namespace

CustomTheme::CustomTheme(ITheme* base, QString id, QString name)
	: m_id(std::move(id)), m_name(std::move(name))
{
	// Seed everything from the base theme. The factories then override only
	// what the theme on disk actually declares, so a sparse -- or broken --
	// theme still yields a usable look.
	m_widgets = base->qtTheme();
	m_styleSheet = base->appStyleSheet();
	m_palette = base->colorScheme();
	m_fadeColor = base->fadeColor();
	m_fadeAmount = base->fadeAmount();
}

std::unique_ptr<CustomTheme> CustomTheme::fromManifest(
	ITheme* base, const QFileInfo& manifestFile)
{
	// The folder carries the identity, not the manifest, so an unreadable
	// theme is still listed and selectable under a recognisable name.
	const QString id = manifestFile.dir().dirName();
	auto theme = std::unique_ptr<CustomTheme>(new CustomTheme(base, id, id));

	qDebug() << "Loading manifest theme" << id;

	ManifestData data;
	if (!readManifest(manifestFile.absoluteFilePath(), data)) {
		qWarning() << "Theme" << id
				   << "was not applied; keeping base theme values.";
		return theme;
	}

	// Commit point. Nothing below can fail, so the manifest is applied whole.
	theme->m_name = data.name;
	theme->m_widgets = data.widgets;
	theme->m_tooltip = data.tooltip;

	if (data.hasColors) {
		for (const auto& [role, color] : data.colors) {
			theme->m_palette.setColor(role, color);
		}
		theme->m_fadeAmount = data.fadeAmount;
		// An omitted fadeColor keeps the base theme's rather than blending
		// against an invalid colour.
		if (data.fadeColor.isValid()) {
			theme->m_fadeColor = data.fadeColor;
		}
	}

	// Derive the Disabled colour group from the Active one.
	theme->m_palette =
		fadeInactive(theme->m_palette, theme->m_fadeAmount, theme->m_fadeColor);

	const QString sheetPath =
		FS::PathCombine(manifestFile.dir().path(), data.styleSheetName);
	if (QFileInfo(sheetPath).isFile()) {
		try {
			// TODO: validate the stylesheet?
			theme->m_styleSheet = QString::fromUtf8(FS::read(sheetPath));
		} catch (const Exception& e) {
			qWarning() << "Couldn't load stylesheet" << sheetPath << ":"
					   << e.cause();
		}
	} else {
		qDebug() << "Theme" << id << "has no stylesheet at" << sheetPath;
	}

	return theme;
}

std::unique_ptr<CustomTheme> CustomTheme::fromStyleSheet(
	ITheme* base, const QFileInfo& sheetFile)
{
	// The extension is part of the id so that midnight.qss and midnight.css
	// can coexist, but it is dropped from the display name.
	auto theme = std::unique_ptr<CustomTheme>(
		new CustomTheme(base, sheetFile.fileName(), sheetFile.baseName()));

	qDebug() << "Loading stylesheet theme" << theme->m_id << "from"
			 << sheetFile.absoluteFilePath();

	try {
		// TODO: validate the stylesheet?
		theme->m_styleSheet =
			QString::fromUtf8(FS::read(sheetFile.absoluteFilePath()));
	} catch (const Exception& e) {
		qWarning() << "Couldn't load stylesheet"
				   << sheetFile.absoluteFilePath() << ":" << e.cause();
	}

	return theme;
}

bool CustomTheme::writeSkeleton(ITheme* base, const QString& folder)
{
	const QString path = FS::PathCombine("themes", folder);
	const QString resourcesPath = FS::PathCombine(path, "resources");

	if (!FS::ensureFolderPathExists(path) ||
		!FS::ensureFolderPathExists(resourcesPath)) {
		qWarning() << "Couldn't create folders for theme" << folder;
		return false;
	}

	// Folder names are lowercase by convention; give the generated theme a
	// presentable display name.
	QString displayName = folder;
	if (!displayName.isEmpty()) {
		displayName[0] = displayName[0].toUpper();
	}

	// Never clobber what the user already has.
	const QString manifestPath = FS::PathCombine(path, manifestFileName);
	if (!QFileInfo::exists(manifestPath)) {
		qDebug() << "Writing starter theme manifest to" << manifestPath;
		if (!writeManifest(manifestPath, base, displayName)) {
			return false;
		}
	}

	const QString sheetPath = FS::PathCombine(path, defaultStyleSheetName);
	if (!QFileInfo::exists(sheetPath)) {
		try {
			FS::write(sheetPath, base->appStyleSheet().toUtf8());
		} catch (const Exception& e) {
			qWarning() << "Couldn't write stylesheet" << sheetPath << ":"
					   << e.cause();
			return false;
		}
	}

	return true;
}

QStringList CustomTheme::searchPaths()
{
	// Only manifest themes have a resources folder, and only when the theme
	// author actually created one. For stylesheet themes m_id is a file name,
	// so this path never exists.
	const QString resourcesPath =
		FS::PathCombine("themes", m_id, "resources");
	if (QFileInfo::exists(resourcesPath)) {
		return {resourcesPath};
	}
	return {};
}

QString CustomTheme::tooltip()
{
	return m_tooltip;
}

QString CustomTheme::id()
{
	return m_id;
}

QString CustomTheme::name()
{
	return m_name;
}

bool CustomTheme::hasColorScheme()
{
	return true;
}

QPalette CustomTheme::colorScheme()
{
	return m_palette;
}

bool CustomTheme::hasStyleSheet()
{
	return true;
}

QString CustomTheme::appStyleSheet()
{
	return m_styleSheet;
}

double CustomTheme::fadeAmount()
{
	return m_fadeAmount;
}

QColor CustomTheme::fadeColor()
{
	return m_fadeColor;
}

QString CustomTheme::qtTheme()
{
	return m_widgets;
}
