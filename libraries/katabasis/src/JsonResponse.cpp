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

#include "JsonResponse.h"

#include <QByteArray>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>

namespace Katabasis
{

	QVariantMap parseJsonResponse(const QByteArray& data)
	{
		QJsonParseError err;
		QJsonDocument doc = QJsonDocument::fromJson(data, &err);
		if (err.error != QJsonParseError::NoError) {
			qWarning() << "parseTokenResponse: Failed to parse token response "
						  "due to err:"
					   << err.errorString();
			return QVariantMap();
		}

		if (!doc.isObject()) {
			qWarning() << "parseTokenResponse: Token response is not an object";
			return QVariantMap();
		}

		return doc.object().toVariantMap();
	}

} // namespace Katabasis
