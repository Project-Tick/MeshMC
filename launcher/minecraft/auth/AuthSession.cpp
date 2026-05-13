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

#include "AuthSession.h"
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QStringList>

QString AuthSession::serializeUserProperties()
{
	/*
	 * If a plugin filled user_properties through MMCO_HOOK_SESSION_FILL,
	 * pass it through as-is — it is expected to already be a valid JSON
	 * object literal. Otherwise emit the empty-object default, which is
	 * what vanilla Mojang flows want.
	 */
	const QString trimmed = user_properties.trimmed();
	if (!trimmed.isEmpty()) {
		QJsonParseError err{};
		auto doc = QJsonDocument::fromJson(trimmed.toUtf8(), &err);
		if (err.error == QJsonParseError::NoError && doc.isObject())
			return doc.toJson(QJsonDocument::Compact);
		/* Malformed → fall through to the empty default so the launch
		 * doesn't break. */
	}
	return QJsonDocument(QJsonObject{}).toJson(QJsonDocument::Compact);
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
