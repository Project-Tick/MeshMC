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

#include "VerifyJavaInstall.h"

#include <launch/LaunchTask.h>
#include <minecraft/MinecraftInstance.h>
#include <minecraft/PackProfile.h>
#include <minecraft/VersionFilterData.h>

#ifdef major
    #undef major
#endif
#ifdef minor
    #undef minor
#endif

void VerifyJavaInstall::executeTask() {
    auto m_inst = std::dynamic_pointer_cast<MinecraftInstance>(m_parent->instance());

    auto javaVersion = m_inst->getJavaVersion();
    auto minecraftComponent = m_inst->getPackProfile()->getComponent("net.minecraft");

    // Java 25 requirement
    if (minecraftComponent->getReleaseDateTime() >= g_VersionFilterData.java25BeginsDate) {
        if (javaVersion.major() < 25) {
            emit logLine("This version of Minecraft requires Java 25 or newer. Please configure Java 25 in Settings.",
                         MessageLevel::Fatal);
            emitFailed(tr("This version of Minecraft requires Java 25 or newer. Please configure Java 25 in Settings."));
            return;
        }
    }
    // Java 21 requirement
    else if (minecraftComponent->getReleaseDateTime() >= g_VersionFilterData.java21BeginsDate) {
        if (javaVersion.major() < 21) {
            emit logLine("Minecraft 1.20.5 and above require the use of Java 21",
                         MessageLevel::Fatal);
            emitFailed(tr("Minecraft 1.20.5 and above require the use of Java 21"));
            return;
        }
    }
    // Java 17 requirement
    else if (minecraftComponent->getReleaseDateTime() >= g_VersionFilterData.java17BeginsDate) {
        if (javaVersion.major() < 17) {
            emit logLine("Minecraft 1.18 Pre Release 2 and above require the use of Java 17",
                         MessageLevel::Fatal);
            emitFailed(tr("Minecraft 1.18 Pre Release 2 and above require the use of Java 17"));
            return;
        }
    }
    // Java 16 requirement
    else if (minecraftComponent->getReleaseDateTime() >= g_VersionFilterData.java16BeginsDate) {
        if (javaVersion.major() < 16) {
            emit logLine("Minecraft 21w19a and above require the use of Java 16",
                         MessageLevel::Fatal);
            emitFailed(tr("Minecraft 21w19a and above require the use of Java 16"));
            return;
        }
    }
    // Java 8 requirement
    else if (minecraftComponent->getReleaseDateTime() >= g_VersionFilterData.java8BeginsDate) {
        if (javaVersion.major() < 8) {
            emit logLine("Minecraft 17w13a and above require the use of Java 8",
                         MessageLevel::Fatal);
            emitFailed(tr("Minecraft 17w13a and above require the use of Java 8"));
            return;
        }
    }

    emitSucceeded();
}
