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

#include "core/InstancePathProblems.h"

#include <QDir>

#include "FileSystem.h"
#include <KLocalizedString>

QList<InstancePathProblem> instancePathProblems(const QStringList& folders,
												const QString& tempPath)
{
	const QString tempFolderText = i18n(
		"This is a problem: <br/>"
		" - MeshMC will likely be deleted without warning "
		"by the operating system <br/>"
		" - close MeshMC now and extract it to a real "
		"location, not a temporary folder");

	QList<InstancePathProblem> problems;
	for (const QString& folder : folders) {
		if (FS::checkProblemticPathJava(QDir(folder))) {
			problems.append(
				{ folder,
				  i18n("Your instance folder \'%1\' contains \'!\' and this is "
					   "known to cause Java problems!", folder),
				  i18n("You have now two options: <br/>"
					   " - change the instance folder in the settings <br/>"
					   " - move this installation of MeshMC to a different folder") });
		}

		const QString absolute = QDir(folder).absolutePath();
		if (absolute.contains(QLatin1String("Rar$"), Qt::CaseInsensitive)) {
			problems.append(
				{ folder,
				  i18n("Your instance folder \'%1\' contains \'Rar$\' - that "
					   "means you haven't extracted MeshMC archive!", folder),
				  tempFolderText });
		} else if (absolute.startsWith(tempPath) ||
				   absolute.contains(QLatin1String("/TempState/"))) {
			problems.append(
				{ folder,
				  i18n("Your instance folder \'%1\' is in a temporary folder: "
					   "\'%2\'!", folder, tempPath),
				  tempFolderText });
		}
	}
	return problems;
}
