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


#include "AppearanceManager.h"

#include "ColorSchemes.h"
#include "ThemedIcons.h"

#include <QApplication>
#include <QDebug>
#include <QEvent>
#include <QPalette>
#include <QPixmapCache>
#include <QStyle>
#include <QTimer>
#include <QWidget>

#include <KColorSchemeManager>
#include <KStyleManager>

#include "settings/GlobalSettings.h"
#include "settings/KConfigSettingsObject.h"

#ifndef Q_OS_MACOS

AppearanceManager::AppearanceManager(KConfigSettingsObject* legacySettings,
									 GlobalSettings* settings)
	: m_settings(settings)
{
	initialize(legacySettings);
}

AppearanceManager::~AppearanceManager() {}

void AppearanceManager::setTitlebarColorOnMac(WId, QColor) {}
void AppearanceManager::setTitlebarColorOfAllWindowsOnMac(QColor) {}
void AppearanceManager::stopSettingNewWindowColorsOnMac() {}

#endif

void AppearanceManager::initialize(KConfigSettingsObject* legacySettings)
{
	// The style of the system, which a colour scheme may take the place of
	// (see paletteChanged()).
	m_systemStyle = QApplication::style()->objectName();

	// The widget style the user chose, if KDE's platform theme is not doing
	// that already.
	KStyleManager::initStyle();

	// The schemes of MeshMC have to be where KColorSchemeManager looks before
	// it is asked, and it restores the scheme that was chosen when it is made.
	ColorSchemes::installBundled(ColorSchemes::userDirectory());
	auto* schemes = KColorSchemeManager::instance();

	const QString legacyTheme =
		ColorSchemes::takeLegacySettings(legacySettings);
	if (!legacyTheme.isEmpty()) {
		const QString id = ColorSchemes::idForLegacyTheme(legacyTheme);
		if (!id.isNull()) {
			qDebug() << "Carrying the theme" << legacyTheme
					 << "over to the colour scheme" << id;
			schemes->activateSchemeId(id);
		} else {
			qDebug() << "The theme" << legacyTheme
					 << "has no colour scheme any more; keeping the current one";
		}
	}

	ThemedIcons::init();
	// An icon theme of earlier versions (pe_dark, flat...) is not there any
	// more, which makes it Breeze.
	const QString wanted = m_settings->iconTheme();
	const QString applied = ThemedIcons::setTheme(wanted);
	if (applied != wanted) {
		m_settings->setIconTheme(applied);
		m_settings->save();
	}
	paletteChanged();

	qApp->installEventFilter(this);
}

void AppearanceManager::setIconTheme(const QString& id)
{
	const QString applied = ThemedIcons::setTheme(id);
	m_settings->setIconTheme(applied);
	m_settings->save();
	// Left to the next turn of the event loop: the icon loader has just been
	// made again, and what asks it for icons should not do so in the middle
	// of the change (in a slot of the combo box that chose the theme).
	QTimer::singleShot(0, this, [this] { refreshLooks(); });
}

bool AppearanceManager::eventFilter(QObject* watched, QEvent* event)
{
	if (watched == qApp && event->type() == QEvent::ApplicationPaletteChange) {
		paletteChanged();
	}
	return QObject::eventFilter(watched, event);
}

void AppearanceManager::paletteChanged()
{
	if (m_applyingStyle) {
		return;
	}

	// A native style (macOS, Windows) paints its own colours, so a colour
	// scheme of the user's shows half of the way on it. Fusion follows the
	// palette; the system's style comes back with the system's scheme.
	const bool custom =
		!KColorSchemeManager::instance()->activeSchemeId().isEmpty();
	const QString style = ColorSchemes::styleFor(
		qApp->style()->objectName(), m_systemStyle, custom, m_styleOverridden);
	if (!style.isEmpty()) {
		m_applyingStyle = true;
		const QPalette keep = qApp->palette();
		QApplication::setStyle(style);
		QApplication::setPalette(keep);
		m_applyingStyle = false;
		m_styleOverridden = custom;
	}

	const QPalette palette = qApp->palette();
	ThemedIcons::setPalette(palette);
	setTitlebarColorOfAllWindowsOnMac(palette.window().color());
	refreshLooks();
}

void AppearanceManager::refreshLooks()
{
	// Icons are drawn when they are needed, from the palette and the icon
	// theme as they are then; what has to happen is that what is on the screen
	// is drawn again, and that nothing is reused from before.
	QPixmapCache::clear();
	for (QWidget* widget : QApplication::allWidgets()) {
		widget->update();
	}
	emit iconsChanged();
}
