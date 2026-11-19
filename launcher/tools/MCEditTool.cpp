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

#include "MCEditTool.h"

#include <QDir>
#include <QProcess>
#include <QUrl>

#include "settings/SettingsObject.h"
#include "BaseInstance.h"
#include "minecraft/MinecraftInstance.h"

MCEditTool::MCEditTool(SettingsObjectPtr settings)
{
	settings->registerSetting("MCEditPath");
	m_settings = settings;
}

void MCEditTool::setPath(QString& path)
{
	m_settings->set("MCEditPath", path);
}

QString MCEditTool::path() const
{
	return m_settings->get("MCEditPath").toString();
}

bool MCEditTool::check(const QString& toolPath, QString& error)
{
	if (toolPath.isEmpty()) {
		error = QObject::tr("Path is empty");
		return false;
	}
	const QDir dir(toolPath);
	if (!dir.exists()) {
		error = QObject::tr("Path does not exist");
		return false;
	}
	if (!dir.exists("mcedit.sh") && !dir.exists("mcedit.py") &&
		!dir.exists("mcedit.exe") && !dir.exists("Contents") &&
		!dir.exists("mcedit2.exe")) {
		error = QObject::tr("Path does not seem to be a MCEdit path");
		return false;
	}
	return true;
}

QString MCEditTool::getProgramPath()
{
#ifdef Q_OS_MACOS
	return path();
#else
	const QString mceditPath = path();
	QDir mceditDir(mceditPath);
#if defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD)
	if (mceditDir.exists("mcedit.sh")) {
		return mceditDir.absoluteFilePath("mcedit.sh");
	} else if (mceditDir.exists("mcedit.py")) {
		return mceditDir.absoluteFilePath("mcedit.py");
	}
	return QString();
#elif defined(Q_OS_WIN32)
	if (mceditDir.exists("mcedit.exe")) {
		return mceditDir.absoluteFilePath("mcedit.exe");
	} else if (mceditDir.exists("mcedit2.exe")) {
		return mceditDir.absoluteFilePath("mcedit2.exe");
	}
	return QString();
#endif
#endif
}
