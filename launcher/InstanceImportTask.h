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
 *  This file incorporates work covered by the following copyright and
 *  permission notice:
 *
 * Copyright 2013-2021 MultiMC Contributors
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

#include "InstanceTask.h"
#include "net/NetJob.h"
#include <QUrl>
#include <QVector>
#include <QFuture>
#include <QFutureWatcher>
#include "settings/SettingsObject.h"
#include "QObjectPtr.h"

#include <nonstd/optional>

namespace Flame
{
	class FileResolvingTask;
	struct Manifest;
	struct File;
} // namespace Flame

namespace Modrinth
{
	struct File;
}

class MinecraftInstance;

class InstanceImportTask : public InstanceTask
{
	Q_OBJECT
  public:
	/* Catalogue identifiers for a pack the user picked through the
	 * launcher's own Modrinth / CurseForge browser. The browser
	 * already knows the slug + version, so we hand them over here
	 * instead of trying to recover them from the manifest after
	 * import. Drag-drop imports leave this empty and let
	 * processFlame / processModrinth populate the same fields from
	 * the manifest. */
	struct PackSourceHint {
		QString provider;	   /* "modrinth" / "curseforge" */
		QString packId;		   /* numeric project id as string */
		QString packSlug;	   /* Modrinth slug, empty for CF */
		QString versionId;	   /* version id (Modrinth) / file id (CF) */
		QString versionLabel;  /* human "1.2.3" */
		QString iconUrl;	   /* upstream icon */
		QString sourceUrl;	   /* canonical pack page */
		bool isEmpty() const
		{
			return provider.isEmpty();
		}
	};

	explicit InstanceImportTask(const QUrl sourceUrl);

	/* Optional — set by the browser page right before NewInstanceDialog
	 * adopts the task. The task records these fields into the freshly
	 * created instance's instance.cfg so PackUpdater can read them
	 * back through `instance_setting_get` without sniffing manifests. */
	void setPackSourceHint(const PackSourceHint& hint)
	{
		m_packHint = hint;
	}

  protected:
	//! Entry point for tasks.
	virtual void executeTask() override;

  private:
	void processZipPack();
	void processMeshMC();
	void processFlame();
	void configureFlameInstance(Flame::Manifest& pack);
	void onFlameFileResolutionSucceeded();
	void processModrinth();
	void processTechnic();

  private slots:
	void downloadSucceeded();
	void downloadFailed(QString reason);
	void downloadProgressChanged(qint64 current, qint64 total);
	void detectFinished();
	void extractFinished();
	void extractAborted();

  private: /* data */
	NetJob::Ptr m_filesNetJob;
	shared_qobject_ptr<Flame::FileResolvingTask> m_modIdResolver;
	QUrl m_sourceUrl;
	QString m_archivePath;
	bool m_downloadRequired = false;
	QFuture<nonstd::optional<QStringList>> m_extractFuture;
	QFutureWatcher<nonstd::optional<QStringList>> m_extractFutureWatcher;
	enum class ModpackType {
		Unknown,
		MeshMC,
		Flame,
		Modrinth,
		Technic
	} m_modpackType = ModpackType::Unknown;

	// Holds the raw detection results from the background scan.
	struct DetectResult {
		QString mmcRoot;	  // non-null → MeshMC pack
		QString flameRoot;	  // non-null → Flame/CurseForge pack
		QString modrinthRoot; // non-null → Modrinth pack
		bool technicFound = false;
		QString extractTarget; // dir to pass to extractSubDir
	};
	QFuture<DetectResult> m_detectFuture;
	QFutureWatcher<DetectResult> m_detectFutureWatcher;
	PackSourceHint m_packHint;

	/* Helper: persist the pack source hint into the freshly created
	 * MinecraftInstance's instance.cfg. Called by processFlame and
	 * processModrinth right after they finish setting up the
	 * instance. Centralised here so the key names stay in one
	 * place; BaseInstance pre-registers the same keys so the values
	 * survive a save+reload cycle. */
	void writePackSourceToInstance(MinecraftInstance& instance,
								   const PackSourceHint& hint);

	/* Mod-metadata sidecar writers (declared as free helpers in
	 * the cpp — see writeModrinthModSidecars / writeFlameModSidecars
	 * — to keep this header free of full Flame/Modrinth manifest
	 * type definitions). */
};
