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
#include <QProcess>
#include <QTimer>
#include <memory>

#include "QObjectPtr.h"

#include "JavaVersion.h"

class JavaChecker;

struct JavaCheckResult {
	QString path;
	QString mojangPlatform;
	QString realPlatform;
	JavaVersion javaVersion;
	QString javaVendor;
	QString outLog;
	QString errorLog;
	bool is_64bit = false;
	int id;
	enum class Validity {
		Errored,
		ReturnedInvalidData,
		Valid
	} validity = Validity::Errored;
};

typedef shared_qobject_ptr<QProcess> QProcessPtr;
typedef shared_qobject_ptr<JavaChecker> JavaCheckerPtr;
class JavaChecker : public QObject
{
	Q_OBJECT
  public:
	explicit JavaChecker(QObject* parent = 0);
	void performCheck();

	QString m_path;
	QString m_args;
	int m_id = 0;
	int m_minMem = 0;
	int m_maxMem = 0;
	int m_permGen = 64;

  signals:
	void checkFinished(JavaCheckResult result);

  private:
	QProcessPtr process;
	QTimer killTimer;
	QString m_stdout;
	QString m_stderr;
  public slots:
	void timeout();
	void finished(int exitcode, QProcess::ExitStatus);
	void error(QProcess::ProcessError);
	void stdoutReady();
	void stderrReady();
};
