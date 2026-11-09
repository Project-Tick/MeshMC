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

#include <QString>

class KConfigSettingsObject;

/*!
 * The colour schemes of the application. A scheme is a KDE ".colors" file,
 * which KColorSchemeManager lists (the ones KDE Frameworks has built in, and
 * every file in a "color-schemes" folder of the user's data directory) and
 * activates; the choice is remembered by KColorSchemeManager.
 */
namespace ColorSchemes
{
	/*!
	 * The folder KColorSchemeManager looks for the user's own schemes in:
	 * "color-schemes" in the generic data location.
	 */
	QString userDirectory();

	/*!
	 * Puts the schemes MeshMC ships (":/color-schemes/*.colors") in \p
	 * directory, so that KColorSchemeManager lists them. A file is written
	 * only when it is not there or differs, so an edit by the user to a
	 * shipped scheme lasts only until the next release changes it, and a
	 * start costs nothing.
	 *
	 * \return how many files were written.
	 */
	int installBundled(const QString& directory);

	/*!
	 * The scheme that replaces a theme of earlier versions (the value of the
	 * old "ApplicationTheme" setting). An empty string is "follow the system".
	 * A null string is a theme that has no scheme (a widget style, or a custom
	 * theme), which leaves the scheme as it is.
	 */
	QString idForLegacyTheme(const QString& theme);

	/*!
	 * Whether \p style is the platform's own widget style. It paints with the
	 * colours of the system and ignores the palette of the application in
	 * places, so a colour scheme that is not the system's shows half of the
	 * way on it: dark panels with light buttons.
	 */
	bool isNativeStyle(const QString& style);

	/*!
	 * The widget style that makes a colour scheme show completely, or an
	 * empty string if \p current is fine as it is.
	 *
	 * With a scheme of its own (\p customScheme) a native style is replaced
	 * by Fusion, which follows the palette. With the system's scheme again,
	 * the style that was replaced (\p overridden) is put back, which is
	 * \p systemStyle.
	 */
	QString styleFor(const QString& current, const QString& systemStyle,
					 bool customScheme, bool overridden);

	/*!
	 * Adds a scheme of the user's to \p directory: copies \p file there, if it
	 * is a KDE colour scheme (a ".colors" file with colours in it). One of the
	 * same name is replaced. KColorSchemeManager lists the schemes there are
	 * when the application starts, so this one is listed from the next start.
	 *
	 * \return whether it was added; \p error says why not.
	 */
	bool importScheme(const QString& file, const QString& directory,
					  QString* error);

	/*!
	 * Reads the settings of the theme system of earlier versions out of
	 * \p settings and removes them (ApplicationTheme, BackgroundCat, TheCat
	 * and CatOpacity), so that this happens once.
	 *
	 * \return the old ApplicationTheme, empty if there was none.
	 */
	QString takeLegacySettings(KConfigSettingsObject* settings);
} // namespace ColorSchemes
