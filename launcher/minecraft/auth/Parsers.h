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
