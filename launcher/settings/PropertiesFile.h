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

#include <QByteArray>
#include <QString>
#include <memory>

class KConfig;

/*!
 * \brief Read-only access to a flat "key=value" file held in memory.
 *
 * Meant for the small files Forge ships inside its jars, such as
 * fmlversion.properties and forgeversion.properties: no sections, '#'
 * comment lines, values that are plain text. It is parsed by KConfig, so it
 * understands exactly as much of the Java .properties syntax as KConfig does
 * (which is all these files use); it is not a general .properties reader.
 */
class PropertiesFile
{
  public:
	explicit PropertiesFile(const QByteArray& data);
	~PropertiesFile();

	bool contains(const QString& key) const;

	/*!
	 * \return the value of \a key, or \a defaultValue if there is none.
	 */
	QString get(const QString& key,
				const QString& defaultValue = QString()) const;

  private:
	std::unique_ptr<KConfig> m_config;
};
