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
 *
 */

#pragma once

#include <QObject>
#include <QList>
#include <QString>

#include "modplatform/ModDownloadTypes.h"
#include "net/NetJob.h"
#include "tasks/Task.h"

class DependencyResolver : public Task
{
	Q_OBJECT

  public:
	explicit DependencyResolver(
		const QList<ModPlatform::SelectedMod>& selectedMods,
		const QString& mcVersion, const QString& loader,
		QObject* parent = nullptr);

	QList<ModPlatform::DependencyInfo> resolvedDependencies() const
	{
		return m_dependencies;
	}
	QList<ModPlatform::UnresolvedDep> unresolvedDependencies() const
	{
		return m_unresolvedDeps;
	}

  protected:
	void executeTask() override;

  private:
	void resolveNextMod();
	void resolveCurseForgeDependencies(const ModPlatform::SelectedMod& mod);
	void resolveModrinthDependencies(const ModPlatform::SelectedMod& mod);
	void onCurseForgeVersionResolved(const ModPlatform::SelectedMod& mod,
									 const QByteArray& data);
	void onModrinthVersionResolved(const ModPlatform::SelectedMod& mod,
								   const QByteArray& data);
	void onDependencyProjectResolved(const QString& platform,
									 const QString& projectId,
									 const QByteArray& data);
	void checkCompletion();

  private:
	void processCFFileDeps(const QJsonObject& fileObj);
	void processMRVersionDeps(const QJsonObject& versionObj);
	void crossResolveFromCurseForge(const QString& projectId);
	void crossResolveFromModrinth(const QString& projectId);
	void executeCrossResolve(const QString& targetPlatform,
							 const QString& projectName,
							 const QString& sourceSlug);

	static QString normalizeName(const QString& name);

  private:
	QList<ModPlatform::SelectedMod> m_selectedMods;
	QList<ModPlatform::DependencyInfo> m_dependencies;
	QList<ModPlatform::UnresolvedDep> m_unresolvedDeps;
	QSet<QString> m_resolvedProjectIds; // avoid duplicates (platform:projectId)
	QSet<QString>
		m_resolvedNames; // avoid cross-platform duplicates (normalized name)
	QString m_mcVersion;
	QString m_loader;
	int m_currentModIndex = 0;
	int m_pendingRequests = 0;
};
