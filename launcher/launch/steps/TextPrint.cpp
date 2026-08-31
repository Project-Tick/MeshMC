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

#include "TextPrint.h"

TextPrint::TextPrint(LaunchTask* parent, const QStringList& lines,
					 MessageLevel::Enum level)
	: LaunchStep(parent)
{
	m_lines = lines;
	m_level = level;
}
TextPrint::TextPrint(LaunchTask* parent, const QString& line,
					 MessageLevel::Enum level)
	: LaunchStep(parent)
{
	m_lines.append(line);
	m_level = level;
}

void TextPrint::executeTask()
{
	emit logLines(m_lines, m_level);
	emitSucceeded();
}

bool TextPrint::canAbort() const
{
	return true;
}

bool TextPrint::abort()
{
	emitFailed("Aborted.");
	return true;
}
