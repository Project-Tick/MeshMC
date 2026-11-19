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

#include <QHash>
#include <QIcon>
#include <QList>
#include <QPalette>
#include <QString>

/*!
 * The icons of the application.
 *
 * Icons come from two places. The ones that have a counterpart in the Breeze
 * icon theme of KDE Frameworks are drawn by KIconEngine from Breeze, which
 * colours them for the palette, so they stay readable on a dark colour scheme.
 * The rest (Minecraft, the mod platforms, the instance icons) are MeshMC's own,
 * in the "multimc" icon theme that is bundled in the resources.
 *
 * The code asks for an icon by one of MeshMC's names ("settings", "bug");
 * aliases() says which of them are Breeze's, and under what name.
 *
 * The icon theme is Breeze unless the user chose another: a folder with an
 * index.theme in userDirectory(), which is where KDE Frameworks looks for
 * icon themes (and so a theme installed for KDE is there too). A theme should
 * inherit Breeze, so that what it does not have is still found. Breeze's
 * icons are only coloured for the palette while Breeze is the theme.
 */
namespace ThemedIcons
{
	/*!
	 * MeshMC's icon names that Breeze has an icon for, and what Breeze calls
	 * it.
	 */
	const QHash<QString, QString>& aliases();

	/*!
	 * Registers Breeze (BreezeIcons::initIcons()), which becomes the icon
	 * theme, with the bundled "multimc" theme as its fallback.
	 */
	void init();

	/*! An icon theme that can be chosen. */
	struct Theme {
		QString id;
		QString name;
	};

	/*! Where the user's own icon themes are: "icons" in the generic data location. */
	QString userDirectory();

	/*!
	 * The icon themes there are, Breeze first: the ones KIconTheme finds, which
	 * are the user's, the system's and Breeze. Looked up each time, so a
	 * theme that was added a moment ago is there.
	 */
	QList<Theme> themes();

	/*!
	 * Makes \p id the icon theme, or Breeze if there is no such theme.
	 *
	 * \return the theme that is in use afterwards.
	 */
	QString setTheme(const QString& id);

	/*! The icon theme in use. */
	QString theme();

	/*! Colours the Breeze icons that are made from now on with \p palette. */
	void setPalette(const QPalette& palette);

	/*!
	 * The icon for \p name: Breeze's, drawn by KIconEngine, if it is one of
	 * the aliases, otherwise the icon of that name in the icon theme (the
	 * bundled one, or Breeze's through QIcon). Null when there is none.
	 */
	QIcon icon(const QString& name);
} // namespace ThemedIcons
