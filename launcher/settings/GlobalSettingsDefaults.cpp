/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-FileContributor: Project Tick
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright 2013-2021 MultiMC Contributors
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

#include "GlobalSettingsDefaults.h"

#include <QDebug>
#include <QFont>
#include <QFontInfo>
#include <QStandardPaths>

#include <BuildConfig.h>

#include "Sys.h"

namespace
{
	struct Memory {
		int min;
		int max;
	};

	Memory memoryDefaults()
	{
		const uint64_t systemRamMiB = Sys::getSystemRam() / Sys::mebibyte;
		if (systemRamMiB >= 32768) { // 32+ GB
			return {1024, 8192};
		}
		if (systemRamMiB >= 16384) { // 16-32 GB
			return {1024, 6144};
		}
		if (systemRamMiB >= 8192) { // 8-16 GB
			return {512, 4096};
		}
		if (systemRamMiB >= 4096) { // 4-8 GB
			return {512, 2048};
		}
		// <4 GB
		return {512, 1024};
	}
} // namespace

QString GlobalSettingsDefaults::updateChannel()
{
	return BuildConfig.VERSION_CHANNEL;
}

QString GlobalSettingsDefaults::consoleFont()
{
	// Worked out once: it asks the font system, and it is asked for by every
	// reader of the default.
	static const QString font = [] {
		QString defaultMonospace;
#ifdef Q_OS_WIN32
		defaultMonospace = "Courier";
#elif defined(Q_OS_MAC)
		defaultMonospace = "Menlo";
#else
		defaultMonospace = "Monospace";
#endif
		// resolve the font so the default actually matches
		QFont consoleFont;
		consoleFont.setFamily(defaultMonospace);
		consoleFont.setStyleHint(QFont::Monospace);
		consoleFont.setFixedPitch(true);
		QFontInfo consoleFontInfo(consoleFont);
		const QString resolved = consoleFontInfo.family();
		QFont resolvedFont(resolved);
		qDebug() << "Detected default console font:" << resolved
				 << ", substitutions:" << resolvedFont.substitutions().join(',');
		return resolved;
	}();
	return font;
}

int GlobalSettingsDefaults::consoleFontSize()
{
#ifdef Q_OS_WIN32
	return 10;
#else
	return 11;
#endif
}

QString GlobalSettingsDefaults::jvisualvmPath()
{
	QString found = QStandardPaths::findExecutable("jvisualvm");
	if (found.isNull()) {
		found = QStandardPaths::findExecutable("visualvm");
	}
	return found;
}

int GlobalSettingsDefaults::minMemAlloc()
{
	return memoryDefaults().min;
}

int GlobalSettingsDefaults::maxMemAlloc()
{
	return memoryDefaults().max;
}
