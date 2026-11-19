/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-FileContributor: Project Tick
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (C) 2026 Project Tick
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

#pragma once

#include <memory>

#include <QFileInfo>
#include "ITheme.h"

/*!
 * A theme supplied by the user, loaded from the "themes" folder.
 *
 * Two on-disk shapes are supported, one factory each:
 *
 *  - a folder containing a `theme.json` manifest, optionally alongside a
 *    stylesheet -- see fromManifest()
 *  - a lone `.qss` / `.css` stylesheet that only restyles an existing theme
 *    -- see fromStyleSheet()
 *
 * Whatever a theme does not specify is inherited from a base theme, so a
 * half-written theme still yields a usable look rather than a broken one.
 */
class CustomTheme : public ITheme
{
  public:
	/// Manifest file name looked for inside a theme folder.
	static const QString manifestFileName;
	/// Stylesheet name used when a manifest does not name one itself.
	static const QString defaultStyleSheetName;

	/*!
	 * Loads the theme described by \a manifestFile (a `theme.json`). The id is
	 * the name of the folder holding the manifest, so moving a theme folder
	 * renames the theme.
	 *
	 * The manifest is applied atomically: if any required field is missing or
	 * the file cannot be parsed, nothing from it is used and the result is a
	 * pass-through of \a base that still carries the folder's name. Never
	 * returns null.
	 */
	static std::unique_ptr<CustomTheme> fromManifest(
		ITheme* base, const QFileInfo& manifestFile);

	/*!
	 * Loads \a sheetFile as a theme in its own right. Palette, widget style and
	 * fade settings all come from \a base; only the stylesheet is replaced. The
	 * id is the file name, the display name is the file name without its
	 * extension. Never returns null.
	 */
	static std::unique_ptr<CustomTheme> fromStyleSheet(
		ITheme* base, const QFileInfo& sheetFile);

	/*!
	 * Seeds `themes/<folder>` with an editable manifest and stylesheet copied
	 * from \a base, giving the user a working theme to modify rather than a
	 * blank page. Files that already exist are left untouched.
	 *
	 * \return true when the folder and both files are in place afterwards.
	 */
	static bool writeSkeleton(ITheme* base, const QString& folder);

	~CustomTheme() override = default;

	QString id() override;
	QString name() override;
	QString tooltip() override;
	bool hasStyleSheet() override;
	QString appStyleSheet() override;
	bool hasColorScheme() override;
	QPalette colorScheme() override;
	double fadeAmount() override;
	QColor fadeColor() override;
	QString qtTheme() override;
	QStringList searchPaths() override;

  private:
	/// Seeds every value from \a base; the factories then override selectively.
	CustomTheme(ITheme* base, QString id, QString name);

	QString m_id;
	QString m_name;
	QString m_tooltip;
	QString m_widgets;
	QString m_styleSheet;
	QPalette m_palette;
	QColor m_fadeColor;
	double m_fadeAmount = 0.5;
};
