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

#include "PropertiesFile.h"

#include <QBuffer>

#include <KConfig>
#include <KConfigGroup>

PropertiesFile::PropertiesFile(const QByteArray& data)
{
	auto buffer = std::make_shared<QBuffer>();
	buffer->setData(data);
	buffer->open(QIODevice::ReadOnly);
	// The configuration reads from the buffer when it is constructed, nothing
	// is written back.
	m_config = std::make_unique<KConfig>(buffer, KConfig::SimpleConfig);
}

PropertiesFile::~PropertiesFile() {}

bool PropertiesFile::contains(const QString& key) const
{
	return m_config->group(QString()).hasKey(key);
}

QString PropertiesFile::get(const QString& key,
							const QString& defaultValue) const
{
	return m_config->group(QString()).readEntry(key, defaultValue);
}
