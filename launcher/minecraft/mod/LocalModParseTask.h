/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-FileContributor: Project Tick
 * SPDX-License-Identifier: GPL-3.0-or-later WITH LicenseRef-MeshMC-MMCO-Module-Exception-1.0
 *
 *   MeshMC - A Custom Launcher for Minecraft
 *   Copyright (C) 2026 Project Tick
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version, with the additional permission
 *   described in the MeshMC MMCO Module Exception 1.0.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 *   You should have received a copy of the MeshMC MMCO Module Exception 1.0
 *   along with this program.  If not, see <https://projecttick.org/licenses/>.
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
