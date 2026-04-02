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

#include "ApplicationMessage.h"

#include <QJsonDocument>
#include <QJsonObject>

void ApplicationMessage::parse(const QByteArray& input)
{
	auto doc = QJsonDocument::fromJson(input);
	auto root = doc.object();

	command = root.value("command").toString();
	args.clear();

	auto parsedArgs = root.value("args").toObject();
	for (auto iter = parsedArgs.begin(); iter != parsedArgs.end(); iter++) {
		args[iter.key()] = iter.value().toString();
	}
}

QByteArray ApplicationMessage::serialize()
{
	QJsonObject root;
	root.insert("command", command);
	QJsonObject outArgs;
	for (auto iter = args.begin(); iter != args.end(); iter++) {
		outArgs[iter.key()] = iter.value();
	}
	root.insert("args", outArgs);

	QJsonDocument out;
	out.setObject(root);
	return out.toJson(QJsonDocument::Compact);
}
