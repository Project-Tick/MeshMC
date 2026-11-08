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

#include "GlobalSettingsSetup.h"

#include <QHash>
#include <QStringList>

#include <KCoreConfigSkeleton>

#include "settings/GlobalSettings.h"
#include "settings/KConfigSettingsObject.h"

namespace
{
	// What earlier versions called a setting. The first name of a setting is
	// the one in GlobalSettings.kcfg; a value that is found under an older
	// name is read from there, and replaced by the current name the next time
	// the setting is written.
	const QHash<QString, QStringList>& olderNames()
	{
		static const QHash<QString, QStringList> names = {
		{"MacNativeMenuBar", {"UseMacNativeMenuBar"}},
		{"CentralModsDir", {"ModsDir"}},
		{"LaunchMaximized", {"MCWindowMaximize"}},
		{"MinecraftWinWidth", {"MCWindowWidth"}},
		{"MinecraftWinHeight", {"MCWindowHeight"}},
		{"ProxyAddr", {"ProxyHostName"}},
		{"ProxyUser", {"ProxyUsername"}},
		{"ProxyPass", {"ProxyPassword"}},
		{"MinMemAlloc", {"MinMemoryAlloc"}},
		{"MaxMemAlloc", {"MaxMemoryAlloc"}},
		{"PreLaunchCommand", {"PreLaunchCmd"}},
		{"PostExitCommand", {"PostExitCmd"}},
		{"BackupBeforeLaunch", {"plugin.backup_system.BackupBeforeLaunch"}},
		};
		return names;
	}
} // namespace

void registerGlobalSettings(KConfigSettingsObject* settings)
{
	// What the class declares is registered with the settings object...
	{
		GlobalSettings declared(
			KSharedConfig::openConfig(QString(), KConfig::SimpleConfig));
		for (KConfigSkeletonItem* item : declared.items()) {
			const QString id = item->name();
			QStringList names{id};
			names += olderNames().value(id);
			settings->registerSetting(names, item->getDefault());
		}
	}

	// ...and the class itself, made over the file the settings are in, is
	// the skeleton of the settings dialog and the typed accessors: one set of
	// values for both, which a set() updates.
	settings->adoptSkeleton(
		std::make_unique<GlobalSettings>(settings->config()));
}

GlobalSettings* typedGlobalSettings(KConfigSettingsObject* settings)
{
	return static_cast<GlobalSettings*>(settings->skeleton());
}
