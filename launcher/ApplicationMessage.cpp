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
