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

#include <QString>
#include <QMultiMap>
#include <memory>
#include "QObjectPtr.h"

class MinecraftAccount;
class QNetworkAccessManager;

struct AuthSession {
	bool MakeOffline(QString offline_playername);
	void MakeDemo();

	QString serializeUserProperties();

	enum Status {
		Undetermined,
		RequiresOAuth,
		RequiresPassword,
		RequiresProfileSetup,
		PlayableOffline,
		PlayableOnline,
		GoneOrMigrated
	} status = Undetermined;

	// client token
	QString client_token;
	// account user name
	QString username;
	// combined session ID
	QString session;
	// volatile auth token
	QString access_token;
	// profile name
	QString player_name;
	// profile ID
	QString uuid;
	// 'legacy' or 'mojang', depending on account type
	QString user_type;
	// Did the auth server reply?
	bool auth_server_online = false;
	// Did the user request online mode?
	bool wants_online = true;

	// Is this a demo session?
	bool demo = false;
};

typedef std::shared_ptr<AuthSession> AuthSessionPtr;
