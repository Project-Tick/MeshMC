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

#include <QColor>
#include <QObject>
#include <QWindow>
#include <memory>

class QEvent;
class GlobalSettings;
class KConfigSettingsObject;

/*!
 * Brings the look of the application up: the widget style (KStyleManager),
 * the colour scheme (KColorSchemeManager, which also remembers the choice) and
 * the icons (ThemedIcons), and keeps them in step when the palette changes.
 *
 * \p legacySettings is the settings file of earlier versions, where the theme
 * was chosen; what is in it is carried over once, see ColorSchemes. The
 * icon theme is the setting IconTheme of \p settings.
 */
class AppearanceManager : public QObject
{
	Q_OBJECT
  public:
	AppearanceManager(KConfigSettingsObject* legacySettings,
					  GlobalSettings* settings);
	~AppearanceManager() override;

	/*!
	 * Makes \p id the icon theme and remembers it. A theme that is not there
	 * is Breeze.
	 */
	void setIconTheme(const QString& id);

  signals:
	/*! The palette changed, so the icons are coloured differently. */
	void iconsChanged();

  protected:
	bool eventFilter(QObject* watched, QEvent* event) override;

  private:
	void initialize(KConfigSettingsObject* legacySettings);
	GlobalSettings* m_settings;
	void paletteChanged();
	void setTitlebarColorOfAllWindowsOnMac(QColor color);
	void setTitlebarColorOnMac(WId windowId, QColor color);
	void stopSettingNewWindowColorsOnMac();
#ifdef Q_OS_MACOS
	struct MacState;
	std::unique_ptr<MacState> m_macState;
#endif
};
