/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-FileContributor: Project Tick
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 *   MeshMC - A Custom Launcher for Minecraft
 *   Copyright (C) 2026 Project Tick
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "JVisualVM.h"

#include <QDir>
#include <QStandardPaths>

#include "settings/SettingsObject.h"
#include "launch/LaunchTask.h"
#include "BaseInstance.h"

class JVisualVM : public BaseProfiler
{
	Q_OBJECT
  public:
	JVisualVM(SettingsObjectPtr settings, InstancePtr instance,
			  QObject* parent = 0);

  private slots:
	void profilerStarted();
	void profilerFinished(int exit, QProcess::ExitStatus status);

  protected:
	void beginProfilingImpl(shared_qobject_ptr<LaunchTask> process);
};

JVisualVM::JVisualVM(SettingsObjectPtr settings, InstancePtr instance,
					 QObject* parent)
	: BaseProfiler(settings, instance, parent)
{
}

void JVisualVM::profilerStarted()
{
	emit readyToLaunch(tr("JVisualVM started"));
}

void JVisualVM::profilerFinished(int exit, QProcess::ExitStatus status)
{
	if (status == QProcess::CrashExit) {
		emit abortLaunch(tr("Profiler aborted"));
	}
	if (m_profilerProcess) {
		m_profilerProcess->deleteLater();
		m_profilerProcess = 0;
	}
}

void JVisualVM::beginProfilingImpl(shared_qobject_ptr<LaunchTask> process)
{
	QProcess* profiler = new QProcess(this);
	QStringList profilerArgs = {"--openpid", QString::number(process->pid())};
	auto programPath = globalSettings->get("JVisualVMPath").toString();

	profiler->setArguments(profilerArgs);
	profiler->setProgram(programPath);

	connect(profiler, SIGNAL(started()), SLOT(profilerStarted()));
	connect(profiler, SIGNAL(finished(int, QProcess::ExitStatus)),
			SLOT(profilerFinished(int, QProcess::ExitStatus)));

	profiler->start();
	m_profilerProcess = profiler;
}

void JVisualVMFactory::registerSettings(SettingsObjectPtr settings)
{
	QString defaultValue = QStandardPaths::findExecutable("jvisualvm");
	if (defaultValue.isNull()) {
		defaultValue = QStandardPaths::findExecutable("visualvm");
	}
	settings->registerSetting("JVisualVMPath", defaultValue);
	globalSettings = settings;
}

BaseExternalTool* JVisualVMFactory::createTool(InstancePtr instance,
											   QObject* parent)
{
	return new JVisualVM(globalSettings, instance, parent);
}

bool JVisualVMFactory::check(QString* error)
{
	return check(globalSettings->get("JVisualVMPath").toString(), error);
}

bool JVisualVMFactory::check(const QString& path, QString* error)
{
	if (path.isEmpty()) {
		*error = QObject::tr("Empty path");
		return false;
	}
	QFileInfo finfo(path);
	if (!finfo.isExecutable() || !finfo.fileName().contains("visualvm")) {
		*error = QObject::tr("Invalid path to JVisualVM");
		return false;
	}
	return true;
}

#include "JVisualVM.moc"
