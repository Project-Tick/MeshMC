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

#include "ThemeManager.h"

#include <AppKit/AppKit.h>

void ThemeManager::setTitlebarColorOnMac(WId windowId, QColor color)
{
    if (!windowId) {
        return;
    }

    auto* nativeView = reinterpret_cast<NSView*>(windowId);
    NSWindow* nativeWindow = nativeView.window;

    if (!nativeWindow) {
        return;
    }

    nativeWindow.titlebarAppearsTransparent = YES;
    nativeWindow.backgroundColor = [NSColor colorWithSRGBRed:color.redF()
                                                       green:color.greenF()
                                                        blue:color.blueF()
                                                       alpha:color.alphaF()];
}

void ThemeManager::setTitlebarColorOfAllWindowsOnMac(QColor color)
{
    for (NSWindow* nativeWindow in NSApp.windows) {
        setTitlebarColorOnMac(
            reinterpret_cast<WId>(nativeWindow.contentView),
            color
        );
    }

    stopSettingNewWindowColorsOnMac();

    NSNotificationCenter* notificationCenter =
        NSNotificationCenter.defaultCenter;

    m_windowTitlebarObserver =
        [notificationCenter addObserverForName:NSWindowDidChangeOcclusionStateNotification
                                        object:nil
                                         queue:NSOperationQueue.mainQueue
                                    usingBlock:^(NSNotification* notification) {
                                        NSWindow* nativeWindow =
                                            notification.object;

                                        setTitlebarColorOnMac(
                                            reinterpret_cast<WId>(nativeWindow.contentView),
                                            color
                                        );
                                    }];
}

void ThemeManager::stopSettingNewWindowColorsOnMac()
{
    if (!m_windowTitlebarObserver) {
        return;
    }

    [NSNotificationCenter.defaultCenter removeObserver:m_windowTitlebarObserver];
    m_windowTitlebarObserver = nil;
}
