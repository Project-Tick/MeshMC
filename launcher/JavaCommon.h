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
#include <java/JavaChecker.h>

class QWidget;

/**
 * Common UI bits for the java pages to use.
 */
namespace JavaCommon
{
	bool checkJVMArgs(QString args, QWidget* parent);

	// Show a dialog saying that the Java binary was not usable
	void javaBinaryWasBad(QWidget* parent, JavaCheckResult result);
	// Show a dialog saying that the Java binary was not usable because of bad
	// options
	void javaArgsWereBad(QWidget* parent, JavaCheckResult result);
	// Show a dialog saying that the Java binary was usable
	void javaWasOk(QWidget* parent, JavaCheckResult result);

	class TestCheck : public QObject
	{
		Q_OBJECT
	  public:
		TestCheck(QWidget* parent, QString path, QString args, int minMem,
				  int maxMem, int permGen)
			: m_parent(parent), m_path(path), m_args(args), m_minMem(minMem),
			  m_maxMem(maxMem), m_permGen(permGen)
		{
		}
		virtual ~TestCheck() {};

		void run();

	  signals:
		void finished();

	  private slots:
		void checkFinished(JavaCheckResult result);
		void checkFinishedWithArgs(JavaCheckResult result);

	  private:
		std::shared_ptr<JavaChecker> checker;
		QWidget* m_parent = nullptr;
		QString m_path;
		QString m_args;
		int m_minMem = 0;
		int m_maxMem = 0;
		int m_permGen = 64;
	};
} // namespace JavaCommon
