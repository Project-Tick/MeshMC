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


#include "ThemedIcons.h"

#include <QDebug>
#include <QDir>
#include <QStandardPaths>

#include <BreezeIcons>
#include <KIconEngine>
#include <KIconLoader>
#include <KIconTheme>

namespace ThemedIcons
{
	const QHash<QString, QString>& aliases()
	{
		static const QHash<QString, QString> table = {
			{QStringLiteral("about"), QStringLiteral("help-about")},
			{QStringLiteral("accounts"), QStringLiteral("user-identity")},
			{QStringLiteral("appearance"),
			 QStringLiteral("preferences-desktop-theme")},
			{QStringLiteral("backup"), QStringLiteral("backup")},
			{QStringLiteral("bug"), QStringLiteral("tools-report-bug")},
			{QStringLiteral("checkupdate"),
			 QStringLiteral("system-software-update")},
			{QStringLiteral("copy"), QStringLiteral("edit-copy")},
			{QStringLiteral("custom-commands"),
			 QStringLiteral("utilities-terminal")},
			{QStringLiteral("delete"), QStringLiteral("edit-delete")},
			{QStringLiteral("export"), QStringLiteral("document-export")},
			{QStringLiteral("externaltools"),
			 QStringLiteral("applications-development")},
			{QStringLiteral("help"), QStringLiteral("help-contents")},
			{QStringLiteral("instance-settings"), QStringLiteral("configure")},
			{QStringLiteral("launch"), QStringLiteral("media-playback-start")},
			{QStringLiteral("log"), QStringLiteral("text-x-log")},
			{QStringLiteral("new"), QStringLiteral("list-add")},
			{QStringLiteral("news"), QStringLiteral("news-subscribe")},
			{QStringLiteral("notes"), QStringLiteral("view-pim-notes")},
			{QStringLiteral("plugins"), QStringLiteral("plugins")},
			{QStringLiteral("proxy"),
			 QStringLiteral("preferences-system-network-proxy")},
			{QStringLiteral("refresh"), QStringLiteral("view-refresh")},
			{QStringLiteral("rename"), QStringLiteral("edit-rename")},
			{QStringLiteral("screenshots"), QStringLiteral("camera-photo")},
			{QStringLiteral("settings"), QStringLiteral("preferences-system")},
			{QStringLiteral("shortcut"), QStringLiteral("insert-link")},
			{QStringLiteral("tag"), QStringLiteral("tag")},
			{QStringLiteral("viewfolder"), QStringLiteral("folder-open")},
		};
		return table;
	}

	void init()
	{
		// Registers the "breeze" theme of KDE Frameworks. It has to stay the
		// theme of QIcon: KIconLoader follows QIcon::themeName(), and only
		// colours the icons for the palette while that is Breeze. The bundled
		// "multimc" theme is the fallback, for what Breeze does not have.
		BreezeIcons::initIcons();
		QIcon::setThemeName(QStringLiteral("breeze"));
		QIcon::setFallbackThemeName(QStringLiteral("multimc"));

		// QIcon looks for themes in its search paths, KIconTheme in the
		// generic data location: the user's folder has to be in both.
		QDir().mkpath(userDirectory());
		auto paths = QIcon::themeSearchPaths();
		if (!paths.contains(userDirectory())) {
			paths.append(userDirectory());
			QIcon::setThemeSearchPaths(paths);
		}
	}

	namespace
	{
		QPalette s_palette;
		bool s_paletteSet = false;
	} // namespace

	void setPalette(const QPalette& palette)
	{
		s_palette = palette;
		s_paletteSet = true;
		KIconLoader::global()->setCustomPalette(palette);
	}

	QString userDirectory()
	{
		return QStandardPaths::writableLocation(
				   QStandardPaths::GenericDataLocation) +
			   QStringLiteral("/icons");
	}

	void reload()
	{
		// KIconTheme remembers what it found, which is not up to date for a
		// theme that was put in the folder a moment ago.
		KIconTheme::reconfigure();
	}

	QList<Theme> themes()
	{
		QList<Theme> found;
		for (const QString& id : KIconTheme::list()) {
			// hicolor is the freedesktop fallback, not a look to choose
			if (id == QLatin1String("hicolor")) {
				continue;
			}
			const KIconTheme theme(id);
			if (!theme.isValid()) {
				continue;
			}
			const Theme entry{id, theme.name().isEmpty() ? id : theme.name()};
			if (id == QLatin1String("breeze")) {
				found.prepend(entry);
			} else {
				found.append(entry);
			}
		}
		return found;
	}

	QString setTheme(const QString& id)
	{
		auto isThere = [&id] {
			for (const Theme& theme : themes()) {
				if (theme.id == id) {
					return true;
				}
			}
			return false;
		};
		// A theme put in the folder a moment ago is not known yet.
		if (!isThere()) {
			reload();
		}
		const QString target =
			isThere() ? id : QStringLiteral("breeze");
		if (target != id) {
			qDebug() << "There is no icon theme" << id << "; using Breeze";
		}

		// What the theme in use does not have comes from the fallback: the
		// bundled theme (it has the icons of instances, which no other theme
		// has). The bundled theme itself cannot be its own fallback: it has
		// none of Breeze's icons, and they came out empty.
		const QString fallback = target == QLatin1String("multimc")
									 ? QStringLiteral("breeze")
									 : QStringLiteral("multimc");
		const bool changed = QIcon::themeName() != target ||
							 QIcon::fallbackThemeName() != fallback;
		if (changed) {
			QIcon::setFallbackThemeName(fallback);
			QIcon::setThemeName(target);
			KIconTheme::reconfigure();
			KIconLoader::global()->newIconLoader();
			if (s_paletteSet) {
				KIconLoader::global()->setCustomPalette(s_palette);
			}
		}
		return target;
	}

	QString theme()
	{
		return QIcon::themeName();
	}

	namespace
	{
		QIcon breeze(const QString& name)
		{
			return QIcon(new KIconEngine(name, KIconLoader::global()));
		}
	} // namespace

	QIcon icon(const QString& name)
	{
		const QString alias = aliases().value(name);
		// No hasIcon() check: while the icon loader is being made again it
		// can say no for an icon that is there a moment later, and the
		// fallback has no icon for what only Breeze has (accounts, notes...),
		// which is an empty icon for good. The engine looks when it draws.
		if (!alias.isEmpty()) {
			return breeze(alias);
		}
		// The bundled theme, or what Breeze has under that name.
		return QIcon::fromTheme(name);
	}
} // namespace ThemedIcons
