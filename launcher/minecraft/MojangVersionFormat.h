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

#include <minecraft/VersionFile.h>
#include <minecraft/Library.h>
#include <QJsonDocument>
#include <ProblemProvider.h>

class MojangVersionFormat
{
	friend class OneSixVersionFormat;

  protected:
	// does not include libraries
	static void readVersionProperties(const QJsonObject& in, VersionFile* out);
	// does not include libraries
	static void writeVersionProperties(const VersionFile* in, QJsonObject& out);

  public:
	// version files / profile patches
	static VersionFilePtr versionFileFromJson(const QJsonDocument& doc,
											  const QString& filename);
	static QJsonDocument versionFileToJson(const VersionFilePtr& patch);

	// libraries
	static LibraryPtr libraryFromJson(ProblemContainer& problems,
									  const QJsonObject& libObj,
									  const QString& filename);
	static QJsonObject libraryToJson(Library* library);
};
