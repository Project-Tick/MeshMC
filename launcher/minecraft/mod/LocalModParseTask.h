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
#include <QDebug>
#include <QObject>
#include "Mod.h"
#include "ModDetails.h"

class LocalModParseTask : public QObject, public QRunnable
{
	Q_OBJECT
  public:
	struct Result {
		QString id;
		std::shared_ptr<ModDetails> details;
	};
	using ResultPtr = std::shared_ptr<Result>;
	ResultPtr result() const
	{
		return m_result;
	}

	LocalModParseTask(int token, Mod::ModType type, const QFileInfo& modFile);
	void run();

  signals:
	void finished(int token);

  private:
	void processAsZip();
	void processAsFolder();
	void processAsLitemod();

  private:
	int m_token;
	Mod::ModType m_type;
	QFileInfo m_modFile;
	ResultPtr m_result;
};
