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

#pragma once

#include "AccountData.h"

namespace Parsers
{
	bool getDateTime(QJsonValue value, QDateTime& out);
	bool getString(QJsonValue value, QString& out);
	bool getNumber(QJsonValue value, double& out);
	bool getNumber(QJsonValue value, int64_t& out);
	bool getBool(QJsonValue value, bool& out);

	bool parseXTokenResponse(QByteArray& data, Katabasis::Token& output,
							 QString name);
	bool parseMojangResponse(QByteArray& data, Katabasis::Token& output);

	bool parseMinecraftProfile(QByteArray& data, MinecraftProfile& output);
	bool parseMinecraftEntitlements(QByteArray& data,
									MinecraftEntitlement& output);
	bool parseRolloutResponse(QByteArray& data, bool& result);
} // namespace Parsers
