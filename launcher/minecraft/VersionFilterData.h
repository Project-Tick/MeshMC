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
#include <QMap>
#include <QString>
#include <QSet>
#include <QDateTime>

struct FMLlib {
	QString filename;
	QString checksum;
};

struct VersionFilterData {
	VersionFilterData();
	// mapping between minecraft versions and FML libraries required
	QMap<QString, QList<FMLlib>> fmlLibsMapping;
	// set of minecraft versions for which using forge installers is blacklisted
	QSet<QString> forgeInstallerBlacklist;
	// no new versions below this date will be accepted from Mojang servers
	QDateTime legacyCutoffDate;
	// Libraries that belong to LWJGL
	QSet<QString> lwjglWhitelist;
	// release date of first version to require Java 8 (17w13a)
	QDateTime java8BeginsDate;
	// release data of first version to require Java 16 (21w19a)
	QDateTime java16BeginsDate;
	// release data of first version to require Java 17 (1.18 Pre Release 2)
	QDateTime java17BeginsDate;
	// release date of first version to require Java 21 (24w14a / 1.20.5)
	QDateTime java21BeginsDate;
	// release date of first version to require Java 25
	QDateTime java25BeginsDate;
};
extern VersionFilterData g_VersionFilterData;
