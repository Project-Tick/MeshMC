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

#include "LegacyUpgradeTask.h"
#include "settings/INISettingsObject.h"
#include "FileSystem.h"
#include "NullInstance.h"
#include "pathmatcher/RegexpMatcher.h"
#include <QtConcurrentRun>
#include "LegacyInstance.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"
#include "LegacyModList.h"
#include "classparser.h"

LegacyUpgradeTask::LegacyUpgradeTask(InstancePtr origInstance)
{
	m_origInstance = origInstance;
}

void LegacyUpgradeTask::executeTask()
{
	setStatus(tr("Copying instance %1").arg(m_origInstance->name()));

	FS::copy folderCopy(m_origInstance->instanceRoot(), m_stagingPath);
	folderCopy.followSymlinks(true);

	m_copyFuture = QtConcurrent::run(QThreadPool::globalInstance(), folderCopy);
	connect(&m_copyFutureWatcher, &QFutureWatcher<bool>::finished, this,
			&LegacyUpgradeTask::copyFinished);
	connect(&m_copyFutureWatcher, &QFutureWatcher<bool>::canceled, this,
			&LegacyUpgradeTask::copyAborted);
	m_copyFutureWatcher.setFuture(m_copyFuture);
}

static QString decideVersion(const QString& currentVersion,
							 const QString& intendedVersion)
{
	if (intendedVersion != currentVersion) {
		if (!intendedVersion.isEmpty()) {
			return intendedVersion;
		} else if (!currentVersion.isEmpty()) {
			return currentVersion;
		}
	} else {
		if (!intendedVersion.isEmpty()) {
			return intendedVersion;
		}
	}
	return QString();
}

void LegacyUpgradeTask::copyFinished()
{
	auto successful = m_copyFuture.result();
	if (!successful) {
		emitFailed(tr("Instance folder copy failed."));
		return;
	}
	auto legacyInst = std::dynamic_pointer_cast<LegacyInstance>(m_origInstance);

	auto instanceSettings = std::make_shared<INISettingsObject>(
		FS::PathCombine(m_stagingPath, "instance.cfg"));
	instanceSettings->registerSetting("InstanceType", "Legacy");
	instanceSettings->set("InstanceType", "OneSix");
	// NOTE: this scope ensures the instance is fully saved before we
	// emitSucceeded
	{
		MinecraftInstance inst(m_globalSettings, instanceSettings,
							   m_stagingPath);
		inst.setName(m_instName);

		QString preferredVersionNumber = decideVersion(
			legacyInst->currentVersionId(), legacyInst->intendedVersionId());
		if (preferredVersionNumber.isNull()) {
			// try to decide version based on the jar(s?)
			preferredVersionNumber =
				classparser::GetMinecraftJarVersion(legacyInst->baseJar());
			if (preferredVersionNumber.isNull()) {
				preferredVersionNumber = classparser::GetMinecraftJarVersion(
					legacyInst->runnableJar());
				if (preferredVersionNumber.isNull()) {
					emitFailed(tr("Could not decide Minecraft version."));
					return;
				}
			}
		}
		auto components = inst.getPackProfile();
		components->buildingFromScratch();
		components->setComponentVersion("net.minecraft", preferredVersionNumber,
										true);

		QString jarPath = legacyInst->mainJarToPreserve();
		if (!jarPath.isNull()) {
			qDebug() << "Preserving base jar! : " << jarPath;
			// FIXME: handle case when the jar is unreadable?
			// TODO: check the hash, if it's the same as the upstream jar, do
			// not do this
			components->installCustomJar(jarPath);
		}

		auto jarMods = legacyInst->jarModList()->allMods();
		for (auto& jarMod : jarMods) {
			QString modPath = jarMod.absoluteFilePath();
			qDebug() << "jarMod: " << modPath;
			components->installJarMods({modPath});
		}

		// remove all the extra garbage we no longer need
		auto removeAll = [&](const QString& root, const QStringList& things) {
			for (auto& thing : things) {
				auto removePath = FS::PathCombine(root, thing);
				QFileInfo stat(removePath);
				if (stat.isDir()) {
					FS::deletePath(removePath);
				} else {
					QFile::remove(removePath);
				}
			}
		};
		QStringList rootRemovables = {"modlist", "version", "instMods"};
		QStringList mcRemovables = {"bin", "MeshMCLauncher.jar", "icon.png"};
		removeAll(inst.instanceRoot(), rootRemovables);
		removeAll(inst.gameRoot(), mcRemovables);
	}
	emitSucceeded();
}

void LegacyUpgradeTask::copyAborted()
{
	emitFailed(tr("Instance folder copy has been aborted."));
	return;
}
