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
