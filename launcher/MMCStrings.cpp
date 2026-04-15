/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-FileContributor: Project Tick
 * SPDX-License-Identifier: GPL-3.0-or-later WITH LicenseRef-MeshMC-MMCO-Module-Exception-1.0
 *
 *   MeshMC - A Custom Launcher for Minecraft
 *   Copyright (C) 2026 Project Tick
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version, with the additional permission
 *   described in the MeshMC MMCO Module Exception 1.0.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 *   You should have received a copy of the MeshMC MMCO Module Exception 1.0
 *   along with this program.  If not, see <https://projecttick.org/licenses/>.
 */

#include "MMCStrings.h"

/// TAKEN FROM Qt, because it doesn't expose it intelligently
static inline QChar getNextChar(const QString& s, int location)
{
	return (location < s.length()) ? s.at(location) : QChar();
}

/// TAKEN FROM Qt, because it doesn't expose it intelligently
int Strings::naturalCompare(const QString& s1, const QString& s2,
							Qt::CaseSensitivity cs)
{
	for (int l1 = 0, l2 = 0; l1 <= s1.count() && l2 <= s2.count(); ++l1, ++l2) {
		// skip spaces, tabs and 0's
		QChar c1 = getNextChar(s1, l1);
		while (c1.isSpace())
			c1 = getNextChar(s1, ++l1);
		QChar c2 = getNextChar(s2, l2);
		while (c2.isSpace())
			c2 = getNextChar(s2, ++l2);

		if (c1.isDigit() && c2.isDigit()) {
			while (c1.digitValue() == 0)
				c1 = getNextChar(s1, ++l1);
			while (c2.digitValue() == 0)
				c2 = getNextChar(s2, ++l2);

			int lookAheadLocation1 = l1;
			int lookAheadLocation2 = l2;
			int currentReturnValue = 0;
			// find the last digit, setting currentReturnValue as we go if it
			// isn't equal
			for (QChar lookAhead1 = c1, lookAhead2 = c2;
				 (lookAheadLocation1 <= s1.length() &&
				  lookAheadLocation2 <= s2.length());
				 lookAhead1 = getNextChar(s1, ++lookAheadLocation1),
					   lookAhead2 = getNextChar(s2, ++lookAheadLocation2)) {
				bool is1ADigit = !lookAhead1.isNull() && lookAhead1.isDigit();
				bool is2ADigit = !lookAhead2.isNull() && lookAhead2.isDigit();
				if (!is1ADigit && !is2ADigit)
					break;
				if (!is1ADigit)
					return -1;
				if (!is2ADigit)
					return 1;
				if (currentReturnValue == 0) {
					if (lookAhead1 < lookAhead2) {
						currentReturnValue = -1;
					} else if (lookAhead1 > lookAhead2) {
						currentReturnValue = 1;
					}
				}
			}
			if (currentReturnValue != 0)
				return currentReturnValue;
		}
		if (cs == Qt::CaseInsensitive) {
			if (!c1.isLower())
				c1 = c1.toLower();
			if (!c2.isLower())
				c2 = c2.toLower();
		}
		int r = QString::localeAwareCompare(c1, c2);
		if (r < 0)
			return -1;
		if (r > 0)
			return 1;
	}
	// The two strings are the same (02 == 2) so fall back to the normal sort
	return QString::compare(s1, s2, cs);
}

QString Strings::htmlListPatch(const QString& html)
{
	// QTextBrowser renders <ul>/<ol> lists with too much padding.
	// This function corrects this by adding reasonable CSS margin/padding to
	// the list elements.
	QString patched(html);
	patched.replace(
		QLatin1String("<ul"),
		QLatin1String("<ul style=\"margin-left: 0; padding-left: 20px;\""));
	patched.replace(
		QLatin1String("<ol"),
		QLatin1String("<ol style=\"margin-left: 0; padding-left: 20px;\""));
	patched.replace(QLatin1String("<li"),
					QLatin1String("<li style=\"margin-bottom: 4px;\""));
	return patched;
}

const char* Strings::logColor(QtMsgType type)
{
	switch (type) {
		case QtInfoMsg:
			return "\033[36m"; // cyan
		case QtDebugMsg:
			return "\033[32m"; // green
		case QtWarningMsg:
			return "\033[33m"; // yellow
		case QtCriticalMsg:
			return "\033[31m"; // red
		case QtFatalMsg:
			return "\033[1;31m"; // red again
		default:
			return "";
	}
}

const char* Strings::logColorReset()
{
	return "\033[0m";
}
