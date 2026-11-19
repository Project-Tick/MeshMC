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


#include "ColorSchemes.h"

#include "settings/KConfigSettingsObject.h"

#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QStandardPaths>

#include <KConfig>
#include <KConfigGroup>

namespace ColorSchemes
{
	QString userDirectory()
	{
		return QStandardPaths::writableLocation(
				   QStandardPaths::GenericDataLocation) +
			   QStringLiteral("/color-schemes");
	}

	int installBundled(const QString& directory)
	{
		QDir dir(directory);
		if (!dir.mkpath(QStringLiteral("."))) {
			qWarning() << "Could not create" << directory;
			return 0;
		}

		int written = 0;
		QDirIterator it(QStringLiteral(":/color-schemes"),
						{QStringLiteral("*.colors")}, QDir::Files);
		while (it.hasNext()) {
			it.next();

			QFile source(it.filePath());
			if (!source.open(QIODevice::ReadOnly)) {
				continue;
			}
			const QByteArray content = source.readAll();

			const QString target = dir.filePath(it.fileName());
			QFile existing(target);
			if (existing.open(QIODevice::ReadOnly) &&
				existing.readAll() == content) {
				continue;
			}
			existing.close();

			QSaveFile out(target);
			if (!out.open(QIODevice::WriteOnly) ||
				out.write(content) != content.size() || !out.commit()) {
				qWarning() << "Could not write" << target;
				continue;
			}
			++written;
		}
		return written;
	}

	bool importScheme(const QString& file, const QString& directory,
					  QString* error)
	{
		const QFileInfo info(file);
		if (!info.isFile() ||
			info.suffix().compare(QLatin1String("colors"),
								  Qt::CaseInsensitive) != 0) {
			*error = QStringLiteral("not a .colors file");
			return false;
		}

		// A scheme has at least the colours of a window.
		KConfig config(file, KConfig::SimpleConfig);
		if (!config.hasGroup(QStringLiteral("Colors:Window"))) {
			*error = QStringLiteral("it has no colours of a window");
			return false;
		}

		QDir dir(directory);
		if (!dir.mkpath(QStringLiteral("."))) {
			*error = QStringLiteral("could not create ") + directory;
			return false;
		}
		const QString target = dir.filePath(info.fileName());
		if (QFileInfo(target).canonicalFilePath() ==
			info.canonicalFilePath()) {
			return true; // it is there already
		}
		QFile::remove(target);
		if (!QFile::copy(file, target)) {
			*error = QStringLiteral("could not copy it to ") + target;
			return false;
		}
		return true;
	}

	QString idForLegacyTheme(const QString& theme)
	{
		if (theme.isEmpty() || theme == QLatin1String("system")) {
			return QString(QLatin1String(""));
		}
		if (theme == QLatin1String("dark")) {
			return QStringLiteral("BreezeDark");
		}
		if (theme == QLatin1String("bright")) {
			return QStringLiteral("BreezeLight");
		}
		if (theme == QLatin1String("green-dark")) {
			return QStringLiteral("MeshMCGreenDark");
		}
		if (theme == QLatin1String("green-light")) {
			return QStringLiteral("MeshMCGreenLight");
		}
		return QString();
	}

	QString takeLegacySettings(KConfigSettingsObject* settings)
	{
		if (!settings) {
			return QString();
		}
		KConfigGroup top = settings->config()->group(QString());
		const QString theme =
			top.readEntry(QStringLiteral("ApplicationTheme"), QString());

		bool removed = false;
		for (const char* key : {"ApplicationTheme", "BackgroundCat", "TheCat",
								"CatOpacity"}) {
			if (top.hasKey(QString::fromLatin1(key))) {
				top.deleteEntry(QString::fromLatin1(key));
				removed = true;
			}
		}
		if (removed) {
			settings->config()->sync();
		}
		return theme;
	}
} // namespace ColorSchemes
