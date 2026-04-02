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

#include "AuthSession.h"
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QStringList>

QString AuthSession::serializeUserProperties()
{
	QJsonObject userAttrs;
	/*
	for (auto key : u.properties.keys())
	{
		auto array = QJsonArray::fromStringList(u.properties.values(key));
		userAttrs.insert(key, array);
	}
	*/
	QJsonDocument value(userAttrs);
	return value.toJson(QJsonDocument::Compact);
}

bool AuthSession::MakeOffline(QString offline_playername)
{
	if (status != PlayableOffline && status != PlayableOnline) {
		return false;
	}
	session = "-";
	player_name = offline_playername;
	status = PlayableOffline;
	return true;
}

void AuthSession::MakeDemo()
{
	player_name = "Player";
	demo = true;
}
