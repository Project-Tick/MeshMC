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
#include <QRunnable>
#include <QObject>
#include <QDir>
#include <QMap>
#include "Mod.h"
#include <memory>

class ModFolderLoadTask : public QObject, public QRunnable
{
	Q_OBJECT
  public:
	struct Result {
		QMap<QString, Mod> mods;
	};
	using ResultPtr = std::shared_ptr<Result>;
	ResultPtr result() const
	{
		return m_result;
	}

  public:
	ModFolderLoadTask(QDir dir);
	void run();
  signals:
	void succeeded();

  private:
	QDir m_dir;
	ResultPtr m_result;
};
