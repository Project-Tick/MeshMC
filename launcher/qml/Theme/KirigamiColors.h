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

#include <QObject>
#include <QString>

#include <functional>
#include <memory>

#include <Kirigami/Platform/PlatformTheme>

#include "theme/SystemPalette.h"

/*
 * Kirigami's platform theme as the "system" colour scheme's source, on a
 * Plasma session.
 *
 * Kirigami picks its platform integration by the Qt Quick Controls style's
 * name, and MeshMC's own style (MeshMC.Style) has none, so left alone it
 * falls back to its built-in Breeze definition -- the same light Breeze
 * whatever colour scheme the user picked. On Plasma the desktop's own
 * integration, org.kde.desktop, is named instead: QmlShell hands it to its
 * engine, so Kirigami.Theme and Kirigami.Units follow the user's settings as
 * in any other KDE app, and this class reads the same colour sets from it
 * for ThemeService.
 *
 * Anywhere else -- macOS, Windows, a non-KDE Linux desktop -- desktopPlugin()
 * is empty, and ThemeService reads QPalette, which those platforms fill
 * with the OS colours.
 */
class KirigamiColors : public QObject
{
	Q_OBJECT

  public:
	using PlatformTheme = Kirigami::Platform::PlatformTheme;
	/* Makes the theme for one colour set, owned by @p parent; null if it
	 * cannot. */
	using ThemeFactory =
		std::function<PlatformTheme*(PlatformTheme::ColorSet set, QObject* parent)>;

	/* "org.kde.desktop" on a Plasma session that has it installed (it comes
	 * with Plasma, as part of qqc2-desktop-style; MeshMC does not ship it),
	 * otherwise empty. A Plasma session is told by XDG_CURRENT_DESKTOP,
	 * which Plasma sets to KDE for everything it starts, or KDE_FULL_SESSION;
	 * the plugin by asking Kirigami for it, the same lookup Kirigami itself
	 * makes. */
	static QString desktopPlugin();

	/* The desktop plugin's colour sets, or null wherever desktopPlugin() is
	 * empty or the plugin makes no themes. */
	static std::unique_ptr<KirigamiColors> forDesktop();

	explicit KirigamiColors(const ThemeFactory& factory, QObject* parent = nullptr);

	/* Whether every colour set got a theme. */
	bool isValid() const;

	SystemColors colors() const;

  signals:
	/* A colour set changed -- another colour scheme or accent colour was
	 * picked. Sent once per change, however many sets it touched. */
	void changed();

  private:
	void scheduleChanged();

	PlatformTheme* m_window = nullptr;
	PlatformTheme* m_view = nullptr;
	PlatformTheme* m_selection = nullptr;
	PlatformTheme* m_tooltip = nullptr;
	bool m_changePending = false;
};
