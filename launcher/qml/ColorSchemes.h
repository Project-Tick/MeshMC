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

#include <QAbstractItemModel>
#include <QObject>
#include <QString>

class KColorSchemeManager;

/*
 * The KDE interface's colour scheme away from Plasma -- on macOS, Windows
 * and other Linux desktops -- the way KDE applications get theirs there: a
 * KColorScheme, through KColorSchemeManager, which Kirigami.Theme and the
 * org.kde.desktop controls both draw from. With nothing chosen it follows
 * the system's own light or dark appearance, live (Breeze Light or Breeze
 * Dark); a chosen scheme stays put.
 *
 * On a Plasma session the desktop's platform theme already brings the
 * user's scheme, so nothing here applies (`available` is false).
 *
 * The choice is the UiColorScheme setting (see QmlShell::load()), which a
 * "Colour scheme" picker in Settings can offer from `model` and set through
 * choose(). KColorSchemeManager's own memory of it is turned off, so the
 * launcher's settings stay in one place.
 */
class ColorSchemes : public QObject
{
	Q_OBJECT

	/// Every colour scheme installed (KColorSchemeModel): the first row
	/// follows the system; roles are the scheme's name (display), a preview
	/// (decoration) and its `id`. Null on Plasma.
	Q_PROPERTY(QAbstractItemModel* model READ model CONSTANT)
	/// The chosen scheme's id ("BreezeDark", ...), or empty while the
	/// system's light or dark appearance decides.
	Q_PROPERTY(QString chosen READ chosen NOTIFY chosenChanged)
	/// False on a Plasma session, where the desktop's colours apply.
	Q_PROPERTY(bool available READ available CONSTANT)

  public:
	/* XDG_CURRENT_DESKTOP naming KDE, which Plasma sets for everything it
	 * starts, or KDE_FULL_SESSION. Application asks this too, for the
	 * widget style that goes with the scheme (see its constructor). */
	static bool isPlasmaSession();

	/* "org.kde.desktop" on a Plasma session that has it installed (it comes
	 * with Plasma, as part of qqc2-desktop-style), otherwise empty: the
	 * Kirigami platform plugin QmlShell names to its engine, so Kirigami
	 * follows the user's Plasma settings even under a controls style forced
	 * through QT_QUICK_CONTROLS_STYLE. Found by asking Kirigami for it, the
	 * same lookup Kirigami itself makes. */
	static QString desktopPlugin();

	/* On the offscreen platform only (review snapshots, tests): its theme
	 * names placeholder fonts -- "Sans Serif" 9pt, which no Mac has -- and
	 * leaves every other font to the font database, so Kirigami.Theme's
	 * smallFont (the smallest readable font) came out larger than its
	 * defaultFont (the application's). This makes the application's font
	 * the font database's too, so small text is no larger than body text.
	 * Must run before the engine's first Kirigami.Theme exists: the platform
	 * plugin reads both fonts when each theme is made. Real platforms already
	 * give the system's fonts, and are left alone. */
	static void applyOffscreenFont();

	/* Needs the QGuiApplication: KColorSchemeManager::instance() is made
	 * on first use, and applies the scheme to the application at once. */
	explicit ColorSchemes(QObject* parent = nullptr);

	QAbstractItemModel* model() const;
	QString chosen() const
	{
		return m_chosen;
	}
	bool available() const
	{
		return m_manager != nullptr;
	}

	/// Applies scheme @p id; empty, or an id no installed scheme has,
	/// follows the system again. A no-op on Plasma.
	Q_INVOKABLE void choose(const QString& id);

  signals:
	void chosenChanged();

  private:
	void followSystem();

	KColorSchemeManager* m_manager = nullptr;
	QString m_chosen;
};
