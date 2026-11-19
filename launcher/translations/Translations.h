/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-FileContributor: Project Tick
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright 2013-2021 MultiMC Contributors
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

#include <QString>

class SettingsObject;

/*!
 * The language of the application.
 *
 * It is chosen the way KDE applications do it: in the Help menu, with the
 * dialog of KXmlGui (KHelpMenu::menuSwitchLanguage). That dialog writes the
 * choice to the user's klanguageoverridesrc, and the next start of the
 * program finds it there: KXmlGui turns it into LANGUAGE before the
 * application exists, and KI18n looks the application's strings up in the
 * languages that variable names. The change is therefore in effect after a
 * restart, which is also what the dialog tells the user.
 *
 * What the application has to do itself is below.
 */
namespace Translations
{
	/*!
	 * Directory with the catalogs that were installed together with the
	 * program: <dir>/<language>/LC_MESSAGES/meshmc.mo.
	 */
	QString bundledCatalogDir();

	/*!
	 * Tell KI18n where the catalogs are, and make the language the user
	 * chose the one it uses; load Qt's own translation for it. Call once,
	 * after the Application exists.
	 */
	void initialize(SettingsObject* settings);

	/*!
	 * The language keys that earlier versions wrote into the settings are
	 * not all languages the dialog offers (it lists the languages of the
	 * locales Qt knows). This is the key of the language that stands for
	 * the one that was chosen.
	 */
	QString currentLanguageKey(const QString& key);

	/*!
	 * Earlier versions kept the language in the "Language" setting. Move it
	 * to where the language dialog keeps it, unless the user has chosen a
	 * language with that dialog already, and forget the setting.
	 *
	 * @param overridesFile the file the dialog writes (klanguageoverridesrc)
	 * @return the language that was moved, or an empty string
	 */
	QString migrateLegacyLanguage(SettingsObject* settings,
								  const QString& overridesFile);

	/// The file the language dialog writes to.
	QString overridesFilePath();
} // namespace Translations
