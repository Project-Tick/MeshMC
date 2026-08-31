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

#include <QDialog>
#include <QList>
#include <QTreeWidget>

#include "modplatform/ModDownloadTypes.h"
#include "modplatform/ModUpdateCheckTask.h"

/*
 * ModUpdateDialog
 *
 * Shows the user the set of mods for which a newer compatible version was
 * found, lets them tick which updates to apply, and produces a list of
 * DownloadItem objects ready to be fed into a ContentDownloadTask.
 */
class ModUpdateDialog : public QDialog
{
	Q_OBJECT
  public:
	ModUpdateDialog(const QList<ModUpdateCheckTask::UpdateInfo>& updates,
					QWidget* parent = nullptr);

	/* Returns only the items whose row is checked. */
	QList<ModPlatform::DownloadItem> selectedDownloadItems() const;

  private:
	void setupUi(const QList<ModUpdateCheckTask::UpdateInfo>& updates);

	QTreeWidget* m_tree = nullptr;
	QList<ModUpdateCheckTask::UpdateInfo> m_updates;
};
