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

#include <QString>
#include <QList>
#include <QDir>
#include <QIcon>
#include <QPalette>
#include <memory>
#include <map>
#include <vector>

class ITheme;

#include "CatPack.h"

struct IconThemeEntry {
	QString id;
	QString name;
	QString family;
	QString variant;
};

class ThemeManager
{
  public:
	ThemeManager();

	void addTheme(std::unique_ptr<ITheme> theme);

	ITheme* getTheme(const QString& id);

	void setApplicationTheme(const QString& id, bool initial);

	void setIconTheme(const QString& name);

	void applyCurrentlySelectedTheme(bool initial = false);

	std::vector<ITheme*> allThemes();

	QStringList families();

	std::vector<ITheme*> themesInFamily(const QString& family);

	QList<IconThemeEntry> iconThemes() const;

	QStringList iconThemeFamilies() const;

	QList<IconThemeEntry> iconThemesInFamily(const QString& family) const;

	QString resolveIconTheme(const QString& family) const;

	QString bestIconThemeForPalette(const QString& currentIconId) const;

	// CatPack API
	QString getCatPack(const QString& catName = QString());
	QList<CatPack*> getValidCatPacks();
	QDir getCatPacksFolder();

  private:
	std::map<QString, std::unique_ptr<ITheme>> m_themes;
	QList<IconThemeEntry> m_iconThemes;
	std::map<QString, std::unique_ptr<CatPack>> m_catPacks;
	QDir m_catPacksFolder;
	QString m_defaultStyle;
	QPalette m_defaultPalette;

	void initIconThemes();
	void initializeCatPacks();
	void addCatPack(std::unique_ptr<CatPack> catPack);
};
