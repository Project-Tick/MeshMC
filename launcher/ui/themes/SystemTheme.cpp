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

#include "SystemTheme.h"
#include <QApplication>
#include <QStyle>
#include <QStyleFactory>
#include <QDebug>

SystemTheme::SystemTheme()
{
    qDebug() << "Determining System Theme...";
    const auto & style = QApplication::style();
    systemPalette = style->standardPalette();
    QString lowerThemeName = style->objectName();
    qDebug() << "System theme seems to be:" << lowerThemeName;
    QStringList styles = QStyleFactory::keys();
    for(auto &st: styles)
    {
        qDebug() << "Considering theme from theme factory:" << st.toLower();
        if(st.toLower() == lowerThemeName)
        {
            systemTheme = st;
            qDebug() << "System theme has been determined to be:" << systemTheme;
            return;
        }
    }
    // fall back to fusion if we can't find the current theme.
    systemTheme = "Fusion";
    qDebug() << "System theme not found, defaulted to Fusion";
}

void SystemTheme::apply(bool initial)
{
    // if we are applying the system theme as the first theme, just don't touch anything. it's for the better...
    if(initial)
    {
        return;
    }
    ITheme::apply(initial);
}

QString SystemTheme::id()
{
    return "system";
}

QString SystemTheme::name()
{
    return QObject::tr("System");
}

QString SystemTheme::qtTheme()
{
    return systemTheme;
}

QPalette SystemTheme::colorScheme()
{
    return systemPalette;
}

QString SystemTheme::appStyleSheet()
{
    return QString();
}

double SystemTheme::fadeAmount()
{
    return 0.5;
}

QColor SystemTheme::fadeColor()
{
    return QColor(128,128,128);
}

bool SystemTheme::hasStyleSheet()
{
    return false;
}

bool SystemTheme::hasColorScheme()
{
    return true;
}
